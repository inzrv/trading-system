#pragma once

#include "common/queue.h"
#include "common/envelope.h"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <functional>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

namespace binance
{

class WsSource
{
public:
    enum class State
    {
        STOPPED,
        STARTING,
        RUNNING,
        STOPPING,
        FAILED
    };

    using error_handler_t = std::function<void(beast::error_code, std::string_view)>;
    using state_handler_t = std::function<void(State)>;
    using drop_handler_t = std::function<void()>;

    WsSource(net::io_context& io_ctx,
             ssl::context& ssl_ctx,
             std::shared_ptr<IQueue> queue,
             std::string host,
             std::string port,
             std::string target,
             error_handler_t on_error = {},
             state_handler_t on_state = {},
             drop_handler_t on_drop = {});

    void start();
    void stop();
    void restart();

    [[nodiscard]] State state() const noexcept;

private:
    void do_resolve();
    void do_read();
    void do_close();

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec);
    void on_ssl_handshake(beast::error_code ec);
    void on_ws_handshake(beast::error_code ec);
    void on_read(beast::error_code ec, size_t bytes);
    void on_close(beast::error_code ec);

    void publish_message(std::string payload);
    void publish_error(beast::error_code ec, std::string_view where);
    void publish_state(State state);
    void fail(beast::error_code ec, std::string_view where);
    void schedule_reconnect(std::string_view reason);
    void reset_stream();

private:
    net::io_context& m_io_ctx;
    ssl::context& m_ssl_ctx;
    tcp::resolver m_resolver;
    std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> m_ws;
    beast::flat_buffer m_buffer;
    net::steady_timer m_reconnect_timer;

    std::shared_ptr<IQueue> m_queue;

    std::string m_host;
    std::string m_port;
    std::string m_target;

    State m_state{State::STOPPED};
    bool m_restart_requested{false};
    size_t m_reconnect_attempts{0};

    error_handler_t m_on_error;
    state_handler_t m_on_state;
    drop_handler_t m_on_drop;

    static constexpr size_t kMaxReconnectAttempts{10};
    static constexpr std::chrono::milliseconds kReconnectBaseDelay{200};
    static constexpr std::chrono::milliseconds kReconnectMaxDelay{5000};
};

std::string ws_source_state_to_string(WsSource::State state);
} // namespace binance
