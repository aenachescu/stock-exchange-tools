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
    CurlSetoptError,
    CurlPerformError,
    UnexpectedResponseCode,
    InvalidClosedInterval,
    HtmlElementNotFound,
    IncompleteHtmlElement,
    InvalidHtmlElement,
    InvalidHtmlTag,
    InvalidHtmlAttribute,
    NoData,
    InvalidData,
};

#endif // STOCK_EXCHANGE_TOOLS_ERROR_H
