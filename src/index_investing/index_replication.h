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
        CompanySymbol symbol;
        double weight      = 0.0;
        double target_cost = 0.0;
        double actual_cost = 0.0;
        double value       = 0.0;
        double commission  = 0.0;
        double delta_cost  = 0.0;
        double delta_value = 0.0;
        double dividends   = 0.0;
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

    tl::expected<Entries, Error> CalculateReplication(uint64_t cashAmmount);

    tl::expected<Entries, Error> CalculateReplication(
        const Index& index,
        const Portfolio& portfolio,
        const Activities& activities,
        uint64_t cashAmmount);

private:
    Error FillEntries(const Index& index);
    Error CalculateCostAndValue(
        const Portfolio& portfolio,
        const Activities& activities);
    void CalculateDividends(const Activities& activities);

private:
    std::map<CompanySymbol, Entry> m_entries;
};

#endif // STOCK_EXCHANGE_TOOLS_INDEX_REPLICATION_H
