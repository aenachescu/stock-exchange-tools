#ifndef STOCK_EXCHANGE_TOOLS_WEBSOCKET_CONNECTION_H
#define STOCK_EXCHANGE_TOOLS_WEBSOCKET_CONNECTION_H

#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"

#include <expected.hpp>
#include <memory>
#include <string>

// boost
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace websocket = boost::beast::websocket;
namespace net       = boost::asio;
namespace http      = boost::beast::http;
namespace ssl       = boost::asio::ssl;
using tcp           = boost::asio::ip::tcp;

class WebsocketConnection : private noncopyable, private nonmovable {
public:
    WebsocketConnection(const std::string& host, uint16_t port);
    ~WebsocketConnection();

    Error Connect(const std::string& target, const std::string& subprotocol);
    void Close();

    bool IsConnected()
    {
        return m_wss.is_open();
    }

    tl::expected<std::string, Error> SendRequest(const std::string& req);

private:
    net::io_context m_ioCtx;
    ssl::context m_sslCtx;
    tcp::resolver m_tcpResolver;
    websocket::stream<boost::beast::ssl_stream<tcp::socket>> m_wss;
    std::string m_host;
    uint16_t m_port;
};

#endif // STOCK_EXCHANGE_TOOLS_WEBSOCKET_CONNECTION_H
