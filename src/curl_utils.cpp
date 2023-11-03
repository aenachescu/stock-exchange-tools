#include "curl_utils.h"

size_t write_cbk(void* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

Error CurlHeaders::Add(const char* header)
{
    curl_slist* tmp = curl_slist_append(m_headers, header);
    if (tmp == NULL) {
        curl_slist_free_all(m_headers);
        m_headers = NULL;
        return Error::CurlAddHeaderError;
    }

    m_headers = tmp;
    return Error::NoError;
}

Error CurlHeaders::Add(std::initializer_list<const char*> list)
{
    for (auto elem : list) {
        if (Add(elem) != Error::NoError) {
            return Error::CurlAddHeaderError;
        }
    }

    return Error::NoError;
}

Error ScopedCurl::SetHttpMethod(HttpMethod method)
{
    if (! m_curl) {
        return Error::InvalidCurlHandle;
    }

    switch (method) {
    case HttpMethod::get:
        return curl_easy_setopt(m_curl.Get(), CURLOPT_HTTPGET, 1L) == CURLE_OK
            ? Error::NoError
            : Error::CurlSetMethodError;

    case HttpMethod::post:
        return curl_easy_setopt(m_curl.Get(), CURLOPT_POST, 1L) == CURLE_OK
            ? Error::NoError
            : Error::CurlSetMethodError;

    case HttpMethod::unknown:
    default:
        break;
    }

    return Error::InvalidHttpMethod;
}

Error ScopedCurl::SetHttpVersion(HttpVersion version)
{
    if (! m_curl) {
        return Error::InvalidCurlHandle;
    }

    switch (version) {
    case HttpVersion::http1_1:
        return curl_easy_setopt(
                   m_curl.Get(),
                   CURLOPT_HTTP_VERSION,
                   (long) CURL_HTTP_VERSION_1_1)
                == CURLE_OK
            ? Error::NoError
            : Error::CurlSetVersionError;

    case HttpVersion::unknown:
    default:
        break;
    }

    return Error::InvalidHttpVersion;
}

Error ScopedCurl::SetUrl(const char* url)
{
    if (! m_curl) {
        return Error::InvalidCurlHandle;
    }

    return curl_easy_setopt(m_curl.Get(), CURLOPT_URL, url) == CURLE_OK
        ? Error::NoError
        : Error::CurlSetUrlError;
}

Error ScopedCurl::SetEncoding(const char* encoding)
{
    if (! m_curl) {
        return Error::InvalidCurlHandle;
    }

    return curl_easy_setopt(m_curl.Get(), CURLOPT_ACCEPT_ENCODING, encoding)
            == CURLE_OK
        ? Error::NoError
        : Error::CurlSetUrlError;
}

Error ScopedCurl::SetHeaders(const CurlHeaders& headers)
{
    if (! m_curl) {
        return Error::InvalidCurlHandle;
    }

    return curl_easy_setopt(m_curl.Get(), CURLOPT_HTTPHEADER, headers.Get())
            == CURLE_OK
        ? Error::NoError
        : Error::CurlSetUrlError;
}

tl::expected<HttpResponse, Error> ScopedCurl::Perform()
{
    if (! m_curl) {
        return tl::unexpected(Error::InvalidCurlHandle);
    }

    CURLcode err;
    HttpResponse rsp;

    rsp.headers.reserve(1024);
    rsp.body.reserve(64 * 1024);

    err = curl_easy_setopt(m_curl.Get(), CURLOPT_WRITEFUNCTION, write_cbk);
    if (err != CURLE_OK) {
        return tl::unexpected(Error::CurlSetoptError);
    }

    err = curl_easy_setopt(m_curl.Get(), CURLOPT_WRITEDATA, &rsp.body);
    if (err != CURLE_OK) {
        return tl::unexpected(Error::CurlSetoptError);
    }

    err = curl_easy_setopt(m_curl.Get(), CURLOPT_HEADERDATA, &rsp.headers);
    if (err != CURLE_OK) {
        return tl::unexpected(Error::CurlSetoptError);
    }

    err = curl_easy_perform(m_curl.Get());
    if (err != CURLE_OK) {
        return tl::unexpected(Error::CurlPerformError);
    }

    err = curl_easy_getinfo(m_curl.Get(), CURLINFO_RESPONSE_CODE, &rsp.code);
    if (err != CURLE_OK) {
        rsp.code = -1;
    }

    return std::move(rsp);
}
