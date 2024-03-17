#ifndef STOCK_EXCHANGE_TOOLS_INDEX_REPLICATION_H
#define STOCK_EXCHANGE_TOOLS_INDEX_REPLICATION_H

#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"
#include "stock_index.h"

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
    };

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

    Error AddEntry(
        const CompanySymbol& symbol,
        double weight,
        double cost,
        double commission,
        double value);

    std::vector<Entry> GetEntries(uint64_t cashAmmount);

private:
    std::map<CompanySymbol, Entry> m_entries;
};

#endif // STOCK_EXCHANGE_TOOLS_INDEX_REPLICATION_H
