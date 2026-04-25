#include "gateway.h"

#include "common/log.h"
#include "metrics/scoped_latency.h"
#include "utils/utils.h"

#include <thread>

namespace binance
{

Gateway::Gateway(Config config,
                 net::io_context& io_ctx,
                 ssl::context& ssl_ctx,
                 std::shared_ptr<IQueue> queue,
                 metrics::Registry& metrics)
    : m_config(std::move(config))
    , m_io_ctx(io_ctx)
    , m_ssl_ctx(ssl_ctx)
    , m_queue(queue)
    , m_metrics(metrics)
{
    const auto depth_stream_host = m_config.depth_stream_conf.host;
    const auto depth_stream_port = std::to_string(m_config.depth_stream_conf.port);
    const auto depth_stream_symbol = to_lower(symbol_to_string(m_config.symbol));
    const auto depth_stream_interval = std::to_string(m_config.depth_stream_conf.interval.count());
    const auto depth_stream_target = std::format("/ws/{}@depth@{}ms", depth_stream_symbol, depth_stream_interval);

    m_ws_source = std::make_unique<WsSource>(
        m_io_ctx,
        m_ssl_ctx,
        m_queue,
        depth_stream_host,
        depth_stream_port,
        depth_stream_target,
        m_metrics,
        [this](beast::error_code ec, std::string_view where) {
            on_ws_error(ec, where);
        },
        [this](WsSource::State state) {
            on_ws_state(state);
        },
        []() {
            log::warn("Gateway", "drop queue overflow");
        }
    );

    const auto orderbook_host = m_config.orderbook_conf.host;
    const auto orderbook_port = std::to_string(m_config.orderbook_conf.port);

    m_rest_client = std::make_unique<RestClient>(
        m_io_ctx,
        m_ssl_ctx,
        orderbook_host,
        orderbook_port
    );
}

std::expected<std::string, GatewayError> Gateway::request_snapshot()
{
    const auto orderbook_symbol = symbol_to_string(m_config.symbol);
    const auto orderbook_limit = std::to_string(m_config.orderbook_conf.limit);
    auto last_error = RestError::UNKNOWN_ERROR;

    const auto target = std::format("/api/v3/depth?symbol={}&limit={}", orderbook_symbol, orderbook_limit);
    for (int attempt = 1; attempt <= kSnapshotMaxAttempts; ++attempt) {
        m_metrics.on_snapshot_request();

        log::debug("Gateway", "requesting snapshot... {} (attempt {}/{})", target, attempt, kSnapshotMaxAttempts);
        const auto res = metrics::measure(m_metrics, &metrics::Registry::observe_snapshot_request, [&] {
            return m_rest_client->get(target);
        });
        if (res) {
            log::debug("Gateway", "snapshot request succeeded");
            return *res;
        }
        last_error = res.error();
        m_metrics.on_snapshot_request_failure();

        if (attempt == kSnapshotMaxAttempts) {
            break;
        }

        const auto backoff = kSnapshotBaseBackoff * (1 << (attempt - 1));
        log::warn("Gateway",
                  "snapshot request failed for {}: {}, retrying in {} ms",
                  target,
                  error_to_string(res.error()),
                  backoff.count());
        std::this_thread::sleep_for(backoff);
    }

    log::error("Gateway",
               "snapshot request failed for {} after {} attempts: {}",
               target,
               kSnapshotMaxAttempts,
               error_to_string(last_error));
    m_state = State::FAILED;
    return std::unexpected(GatewayError::REQUEST_ERROR);
}

void Gateway::start()
{
    if (m_state == State::RUNNING) {
        return;
    }

    log::info("Gateway", "starting...");
    m_ws_source->start();
}

void Gateway::stop()
{
    if (m_state == State::STOPPED) {
        return;
    }

    log::info("Gateway", "stopping...");
    m_ws_source->stop();
}

void Gateway::restart()
{
    log::info("Gateway", "restart requested");
    m_ws_source->restart();
}

void Gateway::on_ws_state(WsSource::State state)
{
    std::lock_guard lock{m_state_mutex};
    log::info("Gateway", "websocket state: {}", ws_source_state_to_string(state));
    switch (state) {
        case WsSource::State::STOPPED:
            m_state = State::STOPPED;
            break;
        case WsSource::State::STARTING:
            break;
        case WsSource::State::RUNNING:
            m_state = State::RUNNING;
            break;
        case WsSource::State::STOPPING:
            break;
        case WsSource::State::FAILED:
            m_state = State::FAILED;
            break;
    }

    m_state_cv.notify_all();
}

void Gateway::on_ws_error(beast::error_code ec, std::string_view where)
{
    log::error("Gateway", "websocket error: {} {}", where, ec.message());
    m_state = State::FAILED;
}

std::expected<void, GatewayError> Gateway::wait_until_running(std::chrono::milliseconds timeout)
{
    std::unique_lock lock{m_state_mutex};
    const bool ok = m_state_cv.wait_for(lock, timeout, [this] {
        return m_state == State::RUNNING || m_state == State::FAILED;
    });

    if (!ok) {
        return std::unexpected(GatewayError::TIMEOUT);
    }

    if (m_state == State::FAILED) {
        return std::unexpected(GatewayError::REQUEST_ERROR);
    }

    return {};
}

} // namespace binance
