#include "bvb_scraper.h"

#include <string_view>
#include <utility>

tl::expected<IndexesNames, Error> CBvbScraper::GetIndexes()
{
    static constexpr std::string_view kIndexesMark =
        "<select "
        "name=\"ctl00$ctl00$body$rightColumnPlaceHolder$"
        "IndexProfilesCurrentValues$IndexControlList$ddIndices\"";
    static constexpr std::string_view kIndexesMarkEnd = "</select>";
    static constexpr std::string_view kIndexMark1     = "<option ";
    static constexpr std::string_view kIndexMark2     = ">";
    static constexpr std::string_view kIndexMarkEnd   = "</option>";

    IndexesNames names;

    auto rsp = GetIndicesProfilesPage();
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    std::string& body  = rsp.value().body;
    size_t startPos    = std::string::npos;
    size_t endPos      = std::string::npos;
    size_t indexPos    = std::string::npos;
    size_t indexEndPos = std::string::npos;

    startPos = body.find(kIndexesMark);
    if (startPos == std::string::npos) {
        return tl::unexpected(Error::MarkNotFound);
    }
    startPos += kIndexesMark.size();

    endPos = body.find(kIndexesMarkEnd, startPos);
    if (endPos == std::string::npos) {
        return tl::unexpected(Error::MarkNotFound);
    }

    if (startPos >= endPos) {
        return tl::unexpected(Error::InvalidMarkPosition);
    }

    while (true) {
        indexPos = body.find(kIndexMark1.data(), startPos);
        if (indexPos == std::string::npos || indexPos >= endPos) {
            break;
        }
        indexPos += kIndexMark1.size();

        indexPos = body.find(kIndexMark2.data(), indexPos);
        if (indexPos == std::string::npos || indexPos >= endPos) {
            return tl::unexpected(Error::MarkNotFound);
        }
        indexPos += kIndexMark2.size();

        indexEndPos = body.find(kIndexMarkEnd.data(), indexPos);
        if (indexEndPos == std::string::npos || indexEndPos >= endPos) {
            return tl::unexpected(Error::MarkNotFound);
        }

        names.push_back(body.substr(indexPos, indexEndPos - indexPos));
        startPos = indexEndPos + kIndexesMarkEnd.size();
    }

    return names;
}

tl::expected<HttpResponse, Error> CBvbScraper::SendHttpRequest(
    const char* url,
    const CurlHeaders& headers,
    HttpMethod method,
    HttpVersion version)
{
    Error err = Error::NoError;
    ScopedCurl curl;

    if (! curl) {
        return tl::unexpected(Error::CurlInitError);
    }

#define RETURN_IF_ERROR(func)                                                  \
    err = func;                                                                \
    if (err != Error::NoError) {                                               \
        return tl::unexpected(err);                                            \
    }

    RETURN_IF_ERROR(curl.SetHttpMethod(method));
    RETURN_IF_ERROR(curl.SetHttpVersion(version));
    RETURN_IF_ERROR(curl.SetUrl(url));
    RETURN_IF_ERROR(curl.SetEncoding("gzip"));
    RETURN_IF_ERROR(curl.SetHeaders(headers));

#undef RETURN_IF_ERROR

    return curl.Perform();
}

tl::expected<HttpResponse, Error> CBvbScraper::GetIndicesProfilesPage()
{
    CurlHeaders headers;
    Error err = Error::NoError;

    err = headers.Add({
        "Host: m.bvb.ro",
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 "
        "Firefox/111.0",
        "Accept: "
        "text/html,application/xhtml+xml,application/xml;q=0.9,image/"
        "avif,image/webp,*/*;q=0.8",
        "Accept-Language: en-US,en;q=0.5",
        "Referer: https://www.google.com/",
        "Cookie: MobBVBCulturePref=en-US",
        "Upgrade-Insecure-Requests: 1",
        "Sec-Fetch-Dest: document",
        "Sec-Fetch-Mode: navigate",
        "Sec-Fetch-Site: cross-site",
        "Sec-Fetch-User: ?1",
        "Pragma: no-cache",
        "Cache-Control: no-cache",
    });
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    auto rsp = SendHttpRequest(
        "https://m.bvb.ro/FinancialInstruments/Indices/IndicesProfiles",
        headers);
    if (! rsp) {
        return rsp;
    }

    if (rsp.value().code != 200) {
        return tl::unexpected(Error::UnexpectedResponseCode);
    }

    return std::move(rsp);
}
