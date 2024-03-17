#include "index_replication.h"

#include <algorithm>

Error IndexReplication::AddEntry(
    const CompanySymbol& symbol,
    double weight,
    double cost,
    double commission,
    double value)
{
    auto res = m_entries.emplace(symbol, Entry{});
    if (res.second == false) {
        return Error::AlreadyExists;
    }

    res.first->second.symbol      = symbol;
    res.first->second.weight      = weight;
    res.first->second.actual_cost = cost;
    res.first->second.commission  = commission;
    res.first->second.value       = value;

    return Error::NoError;
}

std::vector<IndexReplication::Entry> IndexReplication::GetEntries(
    uint64_t cashAmmount)
{
    std::vector<Entry> res;
    res.reserve(m_entries.size());

    for (const auto& it : m_entries) {
        res.push_back(it.second);

        Entry& e      = res.back();
        e.target_cost = e.weight * cashAmmount;
        e.delta_cost  = e.target_cost - e.actual_cost;
        e.delta_value = e.target_cost - e.value;
    }

    std::sort(res.begin(), res.end(), EntryCmp{});

    return res;
}
