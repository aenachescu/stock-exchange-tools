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

    struct IndexesDetails
    {
        IndexesNames names;
        IndexName selected;
    };

    struct RequestData
    {
        std::string eventTarget;
        std::string eventArg;
        std::string eventValidation;
        std::string lastFocus;
        std::string viewState;
        std::string viewStateGenerator;
        std::string viewStateEncrypted;
    };

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
    bool IsValidIndexPerformanceValue(const std::string& name);

    tl::expected<HttpResponse, Error> SendHttpRequest(
        const char* url,
        const CurlHeaders& headers,
        HttpMethod method   = HttpMethod::get,
        HttpVersion version = HttpVersion::http1_1);

    tl::expected<HttpResponse, Error> GetIndicesProfilesPage();
    tl::expected<HttpResponse, Error> SelectIndex(
        const IndexName& name,
        const RequestData& reqData);

    tl::expected<RequestData, Error> ParseRequestDataFromMainPage(
        const std::string& data);
    tl::expected<RequestData, Error> ParseRequestDataFromPostRsp(
        const std::string& data);

    tl::expected<IndexesDetails, Error> ParseIndexesNames(
        const std::string& data);
    tl::expected<IndexesPerformance, Error> ParseIndexesPerformance(
        const std::string& data);
    tl::expected<Index, Error> ParseConstituents(const std::string& data);
};

#endif // BVB_SCRAPER_H
