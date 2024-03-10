#include "websocket_connection.h"

WebsocketConnection::WebsocketConnection(const std::string& host, uint16_t port)
    : m_sslCtx(ssl::context::tlsv12_client), m_tcpResolver(m_ioCtx),
      m_wss(m_ioCtx, m_sslCtx), m_host(host), m_port(port)
{
}

WebsocketConnection::~WebsocketConnection()
{
    Close();
}

Error WebsocketConnection::Connect(
    const std::string& target,
    const std::string& subprotocol)
{
    boost::system::error_code ec;

    m_wss.set_option(
        websocket::stream_base::decorator([&](websocket::request_type& req) {
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::sec_websocket_protocol, subprotocol);
        }));

    if (! SSL_set_tlsext_host_name(
            m_wss.next_layer().native_handle(),
            m_host.c_str())) {
        return Error::SetSniFailed;
    }

    auto resolverResults =
        m_tcpResolver.resolve(m_host, std::to_string(m_port), ec);
    if (ec) {
        return Error::IpResolverFailed;
    }

    auto endpoint = net::connect(get_lowest_layer(m_wss), resolverResults, ec);
    if (ec) {
        return Error::TcpConnectFailed;
    }

    std::string wsHost = m_host + ":" + std::to_string(endpoint.port());

    m_wss.next_layer().handshake(ssl::stream_base::client, ec);
    if (ec) {
        return Error::SslHandshakeFailed;
    }

    m_wss.handshake(wsHost, target, ec);
    if (ec) {
        return Error::WebsocketHandshakeFailed;
    }

    return Error::NoError;
}

void WebsocketConnection::Close()
{
    if (m_wss.is_open()) {
        boost::system::error_code ec;
        m_wss.close(websocket::close_code::normal, ec);
    }
}

tl::expected<std::string, Error> WebsocketConnection::SendRequest(
    const std::string& req)
{
    boost::system::error_code ec;
    boost::beast::flat_buffer buffer;

    m_wss.write(net::buffer(req), ec);
    if (ec) {
        return tl::unexpected(Error::WebsocketWriteFailed);
    }

    m_wss.read(buffer, ec);
    if (ec) {
        return tl::unexpected(Error::WebsocketReadFailed);
    }

    return std::string(
        reinterpret_cast<char*>(buffer.data().data()),
        buffer.data().size());
}
