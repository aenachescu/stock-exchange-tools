#ifndef CURL_UTILS_H
#define CURL_UTILS_H

#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"
#include "scoped_ptr.h"

#include <curl/curl.h>
#include <expected.hpp>
#include <string>
#include <utility>
#include <vector>

enum class HttpMethod
{
    unknown,
    get,
    post
};

enum class HttpVersion
{
    unknown,
    http1_1,
};

using PostData = std::vector<std::pair<std::string, std::string>>;

struct HttpResponse
{
    long code = 0;
    std::string headers;
    std::string body;
};

class CurlHeaders : private noncopyable, private nonmovable {
public:
    CurlHeaders() = default;
    ~CurlHeaders()
    {
        curl_slist_free_all(m_headers);
    }

    curl_slist* Get() const
    {
        return m_headers;
    }

    Error Add(const char* header);
    Error Add(std::initializer_list<const char*> list);

private:
    mutable curl_slist* m_headers = NULL;
};

class ScopedCurl : private noncopyable, private nonmovable {
public:
    ScopedCurl()  = default;
    ~ScopedCurl() = default;

    operator bool() const
    {
        return m_curl;
    }

    Error SetHttpMethod(HttpMethod method);
    Error SetHttpVersion(HttpVersion version);
    Error SetUrl(const char* url);
    Error SetEncoding(const char* encoding);
    Error SetHeaders(const CurlHeaders& headers);
    Error SetPostData(const PostData& data);

    tl::expected<HttpResponse, Error> Perform();

private:
    ScopedPtr<CURL, curl_easy_init, curl_easy_cleanup> m_curl;
};

#endif // CURL_UTILS_H
