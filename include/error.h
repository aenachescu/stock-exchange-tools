#ifndef STOCK_EXCHANGE_TOOLS_ERROR_H
#define STOCK_EXCHANGE_TOOLS_ERROR_H

#include <cstdint>

enum class Error : uint32_t
{
    NoError = 0,
    InvalidHttpMethod,
    InvalidHttpVersion,
    InvalidCurlHandle,
    InvalidCurlHeaders,
    CurlAddHeaderError,
    CurlInitError,
    CurlSetMethodError,
    CurlSetVersionError,
    CurlSetUrlError,
    CurlSetEncodingError,
    CurlSetHeadersError,
    CurlSetPostDataSizeError,
    CurlSetPostDataError,
    CurlSetoptError,
    CurlEscapeError,
    CurlPerformError,
    TradevilleInvalidAccount,
    TradevilleInvalidSymbol,
    TradevilleInvalidQuantity,
    TradevilleInvalidAvgPrice,
    TradevilleInvalidMarketPrice,
    TradevilleInvalidAsset,
    TradevilleInvalidCurrency,
    UnexpectedResponseCode,
    InvalidClosedInterval,
    HtmlElementNotFound,
    IncompleteHtmlElement,
    InvalidHtmlElement,
    InvalidHtmlTag,
    InvalidHtmlAttribute,
    NoData,
    InvalidData,
    InvalidValue,
    UnexpectedData,
    InvalidArg,
    SetSniFailed,
    IpResolverFailed,
    TcpConnectFailed,
    SslHandshakeFailed,
    WebsocketHandshakeFailed,
    WebsocketWriteFailed,
    WebsocketReadFailed,
    FileNotFound,
};

#endif // STOCK_EXCHANGE_TOOLS_ERROR_H
