#include "rest_client.h"

#include "common/log.h"

#include <boost/asio/connect.hpp>

namespace binance
{

RestClient::RestClient(net::io_context& io_ctx,
           ssl::context& ssl_ctx,
           std::string host,
           std::string port)
    : m_io_ctx(io_ctx)
    , m_ssl_ctx(ssl_ctx)
    , m_host(std::move(host))
    , m_port(std::move(port))
{}

std::expected<std::string, RestError> RestClient::get(std::string_view target) const
{
    log::debug("RestClient", "GET {}", target);
    beast::error_code ec;

    tcp::resolver resolver{net::make_strand(m_io_ctx)};
    beast::ssl_stream<beast::tcp_stream> stream{net::make_strand(m_io_ctx), m_ssl_ctx};

    if (!SSL_set_tlsext_host_name(stream.native_handle(), m_host.c_str())) {
        log::error("RestClient", "SSL hostname setup failed");
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    const auto results = resolver.resolve(m_host, m_port, ec);
    if (ec) {
        log::error("RestClient", "resolve failed: {}", ec.message());
        return std::unexpected(RestError::RESOLVE_ERROR);
    }

    beast::get_lowest_layer(stream).connect(results, ec);
    if (ec) {
        log::error("RestClient", "connect failed: {}", ec.message());
        return std::unexpected(RestError::CONNECT_ERROR);
    }

    stream.handshake(ssl::stream_base::client, ec);
    if (ec) {
        log::error("RestClient", "SSL handshake failed: {}", ec.message());
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    http::request<http::empty_body> req{http::verb::get, std::string(target), 11};
    req.set(http::field::host, m_host);
    req.set(http::field::user_agent, "trading-system-rest-client");

    http::write(stream, req, ec);
    if (ec) {
        log::error("RestClient", "HTTP write failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_WRITE_ERROR);
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);
    if (ec) {
        log::error("RestClient", "HTTP read failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_READ_ERROR);
    }

    if (res.result() != http::status::ok) {
        log::warn("RestClient", "bad status: {}", static_cast<int>(res.result()));
        return std::unexpected(RestError::BAD_STATUS);
    }

    stream.shutdown(ec);
    log::debug("RestClient", "GET succeeded: {}", target);

    return std::move(res.body());
}

} // namespace binance
