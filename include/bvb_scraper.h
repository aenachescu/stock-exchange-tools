#include "expected.hpp"
#include "stock_index.h"

#include <vector>

class CBvbScraper {
public:
    enum class Error
    {
    };

public:
    CBvbScraper()  = default;
    ~CBvbScraper() = default;

    CBvbScraper(const CBvbScraper&) = delete;
    CBvbScraper(CBvbScraper&&)      = delete;
    CBvbScraper& operator=(CBvbScraper&&) = delete;
    CBvbScraper& operator=(const CBvbScraper&) = delete;

    tl::expected<IndexesNames, Error> GetIndexes();
    tl::expected<IndexesPerformance, Error> GetIndexesPerformance();
    tl::expected<Index, Error> GetConstituents(const IndexName& name);
    tl::expected<Index, Error> GetAdjustmentsHistory(const IndexName& name);
    tl::expected<IndexTradingData, Error> GetTradingData(const IndexName& name);

private:
};
