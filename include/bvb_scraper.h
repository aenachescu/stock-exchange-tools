#ifndef BVB_SCRAPER_H
#define BVB_SCRAPER_H

#include "curl_utils.h"
#include "error.h"
#include "expected.hpp"
#include "noncopyable.h"
#include "nonmovable.h"
#include "stock_index.h"

class BvbScraper : private noncopyable, private nonmovable {
private:
    friend class BvbScraperTest;

public:
    BvbScraper()  = default;
    ~BvbScraper() = default;

    tl::expected<IndexesNames, Error> GetIndexesNames();
    tl::expected<IndexesPerformance, Error> GetIndexesPerformance();
    tl::expected<Index, Error> GetConstituents(const IndexName& name);
    tl::expected<Index, Error> GetAdjustmentsHistory(const IndexName& name);
    tl::expected<IndexTradingData, Error> GetTradingData(const IndexName& name);

private:
    bool IsValidIndexName(const std::string& name);

    tl::expected<HttpResponse, Error> SendHttpRequest(
        const char* url,
        const CurlHeaders& headers,
        HttpMethod method   = HttpMethod::get,
        HttpVersion version = HttpVersion::http1_1);

    tl::expected<HttpResponse, Error> GetIndicesProfilesPage();

    tl::expected<IndexesNames, Error> ParseIndexesNames(
        const std::string& data);
};

#endif // BVB_SCRAPER_H
