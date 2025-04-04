#ifndef STOCK_EXCHANGE_TOOLS_INDEX_REPLICATION_H
#define STOCK_EXCHANGE_TOOLS_INDEX_REPLICATION_H

#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"
#include "stock_index.h"
#include "tradeville.h"

#include <map>
#include <string>
#include <vector>

class IndexReplication : private noncopyable, private nonmovable {
public:
    struct Entry
    {
        // index data
        CompanySymbol symbol;
        double weight = 0.0;

        // portfolio data
        uint64_t shares     = 0;
        double market_price = 0.0;

        // activity data
        double cost               = 0.0;
        double commission         = 0.0;
        double dividends          = 0.0;
        double estimated_dvd      = 0.0;
        double estimated_net_dvd  = 0.0;
        uint64_t estimated_shares = 0;
        std::chrono::year_month_day ex_date;
        std::chrono::year_month_day record_date;
        std::chrono::year_month_day payment_date;

        // statistics based on portfolio data
        double value                   = 0.0;
        double avg_price               = 0.0;
        double profit_loss             = 0.0;
        double total_return            = 0.0;
        double profit_loss_percentage  = 0.0;
        double total_return_percentage = 0.0;

        // statistics based on index data
        double target_value            = 0.0;
        double delta_cost              = 0.0;
        double delta_value             = 0.0;
        double delta_value_percentage  = 0.0;
        uint64_t target_shares         = 0;
        int64_t delta_shares           = 0;
        double delta_shares_percentage = 0.0;
    };

    using Entries = std::vector<Entry>;

private:
    struct EntryCmp
    {
        bool operator()(const Entry& a, const Entry& b) const
        {
            if (a.weight == b.weight) {
                return a.symbol < b.symbol;
            }
            return a.weight > b.weight;
        }
    };

public:
    IndexReplication()  = default;
    ~IndexReplication() = default;

    tl::expected<Entries, Error> CalculateReplication(uint64_t cashAmount);

    tl::expected<Entries, Error> CalculateReplication(
        const Index& index,
        const Portfolio& portfolio,
        const Activities& activities,
        const DividendActivities& dvdActivities,
        uint64_t cashAmount);

    tl::expected<uint64_t, Error> GetPortfolioValue(
        const Index& index,
        const Portfolio& portfolio);

private:
    Error FillIndexData(const Index& index);
    Error FillPortfolioData(const Portfolio& portfolio);
    Error FillActivityData(const Activities& activities);
    Error FillDividendEstimates(
        const Activities& activities,
        const DividendActivities& dvd);
    void FillPortfolioStatistics();
    void FillIndexStatistics(uint64_t cashAmount);

    DividendActivities::const_iterator FindDividendActivity(
        const DividendActivities& dvdActivities,
        const CompanySymbol& symbol,
        const std::chrono::year_month_day& today);

    Error CalculateEstimateShares(
        const Activities& activities,
        const CompanySymbol& symbol,
        const std::chrono::year_month_day& date,
        uint64_t& shares);

private:
    std::map<CompanySymbol, Entry> m_entries;
};

#endif // STOCK_EXCHANGE_TOOLS_INDEX_REPLICATION_H
