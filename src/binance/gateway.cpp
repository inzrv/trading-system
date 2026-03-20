#include "gateway.h"

#include <iostream>

namespace binance
{
Gateway::Gateway(net::io_context& io_ctx,
                 ssl::context& ssl_ctx,
                 std::shared_ptr<IQueue> queue)
    : m_io_ctx(io_ctx)
    , m_ssl_ctx(ssl_ctx)
    , m_queue(queue)
{
    m_ws_source = std::make_unique<WsSource>(
        m_io_ctx,
        m_ssl_ctx,
        m_queue,
        "stream.binance.com",
        "9443",
        "/ws/btcusdt@depth@100ms",
        [this](beast::error_code ec, std::string_view where) {
            on_ws_error(ec, where);
        },
        [this](WsSource::State state) {
            on_ws_state(state);
        },
        []() {
            std::cerr << "[drop] queue overflow\n";
        }
    );

    m_rest_client = std::make_unique<RestClient>(
        m_io_ctx,
        m_ssl_ctx,
        "api.binance.com",
        "443"
    );
}

std::expected<std::string, GatewayError> Gateway::request_snapshot()
{
    m_state = State::RECOVERING;
    const auto res = m_rest_client->get("/api/v3/depth?symbol=BTCUSDT&limit=5000");
    if (!res) {
        m_state = State::FAILED;
        return std::unexpected(GatewayError::REQUEST_ERROR);
    }

    return *res;
}

void Gateway::start()
{
    if (m_state == State::RUNNING || m_state == State::STARTING) {
        return;
    }

    m_ws_source->start();
}

void Gateway::stop()
{
    if (m_state == State::STOPPED || m_state == State::STOPPING) {
        return;
    }

    m_ws_source->stop();
}

void Gateway::restart()
{
    m_ws_source->restart();
}

void Gateway::on_ws_state(WsSource::State state)
{
    std::lock_guard lock{m_state_mutex};
    switch (state) {
        case WsSource::State::STOPPED:
            m_state = State::STOPPED;
            break;
        case WsSource::State::STARTING:
            m_state = State::STARTING;
            break;
        case WsSource::State::RUNNING:
            if (m_state != State::RECOVERING) {
                m_state = State::RUNNING;
            }
            break;
        case WsSource::State::STOPPING:
            m_state = State::STOPPING;
            break;
        case WsSource::State::FAILED:
            m_state = State::FAILED;
            break;
    }

    m_state_cv.notify_all();
}

void Gateway::on_ws_error(beast::error_code ec, std::string_view where)
{
    std::cerr << "[ws error] " << where << ": " << ec.message() << '\n';
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
