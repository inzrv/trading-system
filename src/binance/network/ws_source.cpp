#include "ws_source.h"

#include "common/log.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <utility>

namespace binance
{

std::string ws_source_state_to_string(WsSource::State state)
{
    switch (state) {
        case WsSource::State::STOPPED: return "STOPPED";
        case WsSource::State::STARTING: return "STARTING";
        case WsSource::State::RUNNING: return "RUNNING";
        case WsSource::State::STOPPING: return "STOPPING";
        case WsSource::State::FAILED: return "FAILED";
    }
    return "UNKNOWN";
}

WsSource::WsSource(net::io_context& io_ctx,
                   ssl::context& ssl_ctx,
                   std::shared_ptr<IQueue> queue,
                   std::string host,
                   std::string port,
                   std::string target,
                   metrics::Registry& metrics,
                   replay::MarketDataRecorder* market_data_recorder,
                   error_handler_t on_error,
                   state_handler_t on_state,
                   drop_handler_t on_drop)
    : m_io_ctx(io_ctx)
    , m_ssl_ctx(ssl_ctx)
    , m_resolver(net::make_strand(io_ctx))
    , m_ws(std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(net::make_strand(io_ctx), ssl_ctx))
    , m_reconnect_timer(io_ctx)
    , m_queue(std::move(queue))
    , m_metrics(metrics)
    , m_market_data_recorder(market_data_recorder)
    , m_host(std::move(host))
    , m_port(std::move(port))
    , m_target(std::move(target))
    , m_on_error(std::move(on_error))
    , m_on_state(std::move(on_state))
    , m_on_drop(std::move(on_drop))
{
}

void WsSource::start()
{
    m_reconnect_timer.cancel();
    if (m_state == State::STARTING || m_state == State::RUNNING) {
        return;
    }

    log::info("WsSource", "starting... host={} target={}", m_host, m_target);
    m_restart_requested = false;
    reset_stream();
    publish_state(State::STARTING);
    do_resolve();
}

void WsSource::stop()
{
    log::info("WsSource", "stopping...");
    m_restart_requested = false;
    m_reconnect_timer.cancel();

    if (m_state == State::STOPPED || m_state == State::STOPPING) {
        return;
    }

    publish_state(State::STOPPING);
    do_close();
}

void WsSource::restart()
{
    log::info("WsSource", "restart requested");
    m_restart_requested = true;

    if (m_state == State::STOPPED || m_state == State::FAILED) {
        reset_stream();
        start();
        return;
    }

    if (m_state == State::STOPPING) {
        return;
    }

    publish_state(State::STOPPING);
    do_close();
}

WsSource::State WsSource::state() const noexcept
{
    return m_state;
}

void WsSource::reset_stream()
{
    m_buffer.consume(m_buffer.size());
    m_resolver.cancel();

    if (m_ws) {
        beast::error_code ec;
        beast::get_lowest_layer(*m_ws).close(ec);
    }

    m_ws = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(
        net::make_strand(m_io_ctx),
        m_ssl_ctx);
}

void WsSource::do_resolve()
{
    m_resolver.async_resolve(
        m_host,
        m_port,
        beast::bind_front_handler(&WsSource::on_resolve, this));
}

void WsSource::do_read()
{
    m_ws->async_read(
        m_buffer,
        beast::bind_front_handler(&WsSource::on_read, this));
}

void WsSource::do_close()
{
    if (!beast::get_lowest_layer(*m_ws).is_open()) {
        on_close({});
        return;
    }

    m_ws->async_close(
        websocket::close_code::normal,
        beast::bind_front_handler(&WsSource::on_close, this));
}

void WsSource::on_resolve(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        log::error("WsSource", "resolve failed: {}", ec.message());
        fail(ec, "resolve");
        return;
    }

    log::debug("WsSource", "resolved host");
    net::async_connect(
        beast::get_lowest_layer(*m_ws),
        results,
        [this](beast::error_code ec, const tcp::endpoint&) {
            on_connect(ec);
        });
}

void WsSource::on_connect(beast::error_code ec)
{
    if (ec) {
        log::error("WsSource", "connect failed: {}", ec.message());
        fail(ec, "connect");
        return;
    }

    log::debug("WsSource", "connected to host");
    if (!SSL_set_tlsext_host_name(m_ws->next_layer().native_handle(), m_host.c_str())) {
        const auto ssl_error = static_cast<int>(::ERR_get_error());
        beast::error_code sni_ec{ssl_error, net::error::get_ssl_category()};
        log::error("WsSource", "failed to set SNI host={}: {}", m_host, sni_ec.message());
        fail(sni_ec, "sni");
        return;
    }

    m_ws->next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(&WsSource::on_ssl_handshake, this));
}

void WsSource::on_ssl_handshake(beast::error_code ec)
{
    if (ec) {
        log::error("WsSource", "SSL handshake failed: {}", ec.message());
        fail(ec, "ssl_handshake");
        return;
    }

    log::debug("WsSource", "SSL handshake succeeded");
    m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    m_ws->set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(beast::http::field::user_agent, "binance-ws-source");
        }));

    const std::string host_header = m_host + ":" + m_port;
    m_ws->async_handshake(
        host_header,
        m_target,
        beast::bind_front_handler(&WsSource::on_ws_handshake, this));
}

void WsSource::on_ws_handshake(beast::error_code ec)
{
    if (ec) {
        log::error("WsSource", "websocket handshake failed: {}", ec.message());
        fail(ec, "ws_handshake");
        return;
    }

    log::info("WsSource", "websocket handshake succeeded");
    m_reconnect_attempts = 0;
    publish_state(State::RUNNING);
    do_read();
}

void WsSource::on_read(beast::error_code ec, size_t /*bytes*/)
{
    if (ec) {
        log::error("WsSource", "read failed: {}", ec.message());
        fail(ec, "read");
        return;
    }

    publish_message(beast::buffers_to_string(m_buffer.data()));
    m_buffer.consume(m_buffer.size());

    if (m_state == State::RUNNING) {
        do_read();
    }
}

void WsSource::on_close(beast::error_code ec)
{
    m_buffer.consume(m_buffer.size());

    const bool close_failed = ec && ec != websocket::error::closed;

    if (close_failed) {
        log::warn("WsSource", "close failed: {}", ec.message());
        publish_error(ec, "close");
        publish_state(State::FAILED);
        schedule_reconnect("close_failed");
    } else {
        log::info("WsSource", "closed cleanly");
        publish_state(State::STOPPED);
    }

    reset_stream();

    if (m_restart_requested) {
        m_restart_requested = false;
        start();
    }
}

void WsSource::publish_message(std::string payload)
{
    m_metrics.on_ws_message();
    if (m_market_data_recorder) {
        m_market_data_recorder->record_update(m_host, payload);
    }

    if (!m_queue) {
        return;
    }

    const bool pushed = m_queue->try_push(InputEnvelope{
        .ingress_time = latency_clock::now(),
        .source = m_host,
        .payload = std::move(payload)
    });

    if (!pushed && m_on_drop) {
        m_on_drop();
    }
}

void WsSource::publish_error(beast::error_code ec, std::string_view where)
{
    if (m_on_error) {
        m_on_error(ec, where);
    }
}

void WsSource::publish_state(State state)
{
    m_state = state;

    if (m_on_state) {
        m_on_state(state);
    }
}

void WsSource::fail(beast::error_code ec, std::string_view where)
{
    log::error("WsSource", " failure at {}: {}", where, ec.message());
    publish_error(ec, where);
    publish_state(State::FAILED);
    reset_stream();
    schedule_reconnect(where);
}

void WsSource::schedule_reconnect(std::string_view reason)
{
    if (m_state == State::STOPPING || m_state == State::STOPPED) {
        return;
    }

    if (m_reconnect_attempts >= kMaxReconnectAttempts) {
        log::error("WsSource",
                   "auto-reconnect disabled after {} attempts (last reason: {})",
                   m_reconnect_attempts,
                   reason);
        return;
    }

    m_metrics.on_ws_reconnect();

    const auto attempt = m_reconnect_attempts++;
    const auto multiplier = static_cast<int64_t>(1ull << std::min<size_t>(attempt, 5));
    const auto exp_delay = std::chrono::milliseconds{kReconnectBaseDelay.count() * multiplier};
    const auto delay = std::min(exp_delay, kReconnectMaxDelay);

    log::warn("WsSource",
              "scheduling auto-reconnect attempt {}/{} in {} ms (reason: {})",
              m_reconnect_attempts,
              kMaxReconnectAttempts,
              delay.count(),
              reason);

    m_reconnect_timer.expires_after(delay);
    m_reconnect_timer.async_wait([this](beast::error_code ec) {
        if (ec) {
            return;
        }

        if (m_state == State::STOPPING || m_state == State::RUNNING) {
            return;
        }

        start();
    });
}

} // namespace binance
