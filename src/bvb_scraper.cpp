#include "bvb_scraper.h"

#include "scoped_ptr.h"

#include <curl/curl.h>
#include <utility>

using Error      = CBvbScraper::Error;
using ScopedCurl = ScopedPtr<CURL, curl_easy_init, curl_easy_cleanup>;

tl::expected<IndexesNames, Error> CBvbScraper::GetIndexes()
{
    IndexesNames names;

    auto rsp = FetchData("");
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    return names;
}

tl::expected<std::string, Error> CBvbScraper::FetchData(const std::string& url)
{
    ScopedCurl curl;
    if (! curl) {
        return tl::unexpected(Error::CurlInitError);
    }

    return "";
}
