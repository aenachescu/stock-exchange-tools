#ifndef BVB_SCRAPER_H
#define BVB_SCRAPER_H

#include "curl_utils.h"
#include "error.h"
#include "expected.hpp"
#include "noncopyable.h"
#include "nonmovable.h"
#include "stock_index.h"

class CBvbScraper : private noncopyable, private nonmovable {
public:
    CBvbScraper()  = default;
    ~CBvbScraper() = default;

    tl::expected<IndexesNames, Error> GetIndexes();
    tl::expected<IndexesPerformance, Error> GetIndexesPerformance();
    tl::expected<Index, Error> GetConstituents(const IndexName& name);
    tl::expected<Index, Error> GetAdjustmentsHistory(const IndexName& name);
    tl::expected<IndexTradingData, Error> GetTradingData(const IndexName& name);

private:
    tl::expected<HttpResponse, Error> SendHttpRequest(
        const char* url,
        const CurlHeaders& headers,
        HttpMethod method   = HttpMethod::get,
        HttpVersion version = HttpVersion::http1_1);

    tl::expected<HttpResponse, Error> GetIndicesProfilesPage();
};

#endif // BVB_SCRAPER_H
