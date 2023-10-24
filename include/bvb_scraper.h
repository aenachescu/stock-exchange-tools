#ifndef BVB_SCRAPER_H
#define BVB_SCRAPER_H

#include "expected.hpp"
#include "noncopyable.h"
#include "nonmovable.h"
#include "stock_index.h"

#include <vector>

class CBvbScraper : private noncopyable, private nonmovable {
public:
    enum class Error
    {
        CurlInitError = 1,
    };

public:
    CBvbScraper()  = default;
    ~CBvbScraper() = default;

    tl::expected<IndexesNames, Error> GetIndexes();
    tl::expected<IndexesPerformance, Error> GetIndexesPerformance();
    tl::expected<Index, Error> GetConstituents(const IndexName& name);
    tl::expected<Index, Error> GetAdjustmentsHistory(const IndexName& name);
    tl::expected<IndexTradingData, Error> GetTradingData(const IndexName& name);

private:
    tl::expected<std::string, Error> FetchData(const std::string& url);
};

#endif // BVB_SCRAPER_H
