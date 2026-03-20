#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>

#include <expected>
#include <string>
#include <string_view>

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

namespace binance
{

enum class RestError
{
    RESOLVE_ERROR,
    CONNECT_ERROR,
    SSL_HANDSHAKE_ERROR,
    HTTP_WRITE_ERROR,
    HTTP_READ_ERROR,
    BAD_STATUS,
    SHUTDOWN_ERROR
};

class RestClient
{
public:
    RestClient(net::io_context& io_ctx,
               ssl::context& ssl_ctx,
               std::string host,
               std::string port);

    std::expected<std::string, RestError> get(std::string_view target) const;

private:
    net::io_context& m_io_ctx;
    ssl::context& m_ssl_ctx;
    std::string m_host;
    std::string m_port;
};

} // namespace binance
