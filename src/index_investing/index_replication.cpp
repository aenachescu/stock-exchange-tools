#include "index_replication.h"

#include <algorithm>

using IR = IndexReplication;

tl::expected<IR::Entries, Error> IndexReplication::CalculateReplication(
    uint64_t cashAmmount)
{
    if (m_entries.empty()) {
        return tl::unexpected(Error::NoData);
    }

    std::vector<Entry> res;
    res.reserve(m_entries.size());

    for (const auto& it : m_entries) {
        res.push_back(it.second);

        Entry& e      = res.back();
        e.target_cost = e.weight * cashAmmount;
        e.delta_cost  = e.actual_cost - e.target_cost;
        e.delta_value = e.value - e.target_cost;
    }

    std::sort(res.begin(), res.end(), EntryCmp{});

    return res;
}

tl::expected<IR::Entries, Error> IndexReplication::CalculateReplication(
    const Index& index,
    const Portfolio& portfolio,
    const Activities& activities,
    uint64_t cashAmmount)
{
    Error err = Error::NoError;

    err = FillEntries(index);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = CalculateCostAndValue(portfolio, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    return CalculateReplication(cashAmmount);
}

Error IndexReplication::FillEntries(const Index& index)
{
    m_entries.clear();

    double totalWeight = 0.0;

    for (const auto& company : index.companies) {
        auto res = m_entries.emplace(company.symbol, Entry{});
        if (res.second == false) {
            m_entries.clear();
            return Error::AlreadyExists;
        }

        res.first->second.symbol = company.symbol;
        res.first->second.weight = company.weight / 100.0;

        totalWeight += company.weight;
    }

    using value_type = decltype(m_entries)::value_type;
    auto maxEntry    = std::max_element(
        m_entries.begin(),
        m_entries.end(),
        [](const value_type& a, const value_type& b) {
            return a.second.weight < b.second.weight;
        });

    maxEntry->second.weight += ((100.0 - totalWeight) / 100.0);

    return Error::NoError;
}

Error IndexReplication::CalculateCostAndValue(
    const Portfolio& portfolio,
    const Activities& activities)
{
    std::map<CompanySymbol, uint64_t> quantities;

    for (const auto& it : m_entries) {
        quantities.emplace(it.first, 0ull);
    }

    for (const auto& activity : activities) {
        if (activity.type != ActivityType::Buy &&
            activity.type != ActivityType::AssetTransfer) {
            continue;
        }

        auto entryIt = m_entries.find(activity.symbol);
        if (entryIt == m_entries.end()) {
            continue;
        }

        if (std::holds_alternative<uint64_t>(activity.quantity) == false) {
            return Error::UnexpectedData;
        }

        quantities[activity.symbol] += std::get<uint64_t>(activity.quantity);

        double ammount = std::fabs(activity.cash_ammount);
        double cost    = activity.price * std::get<uint64_t>(activity.quantity);

        entryIt->second.actual_cost += cost;
        entryIt->second.commission += (ammount - cost);
    }

    for (const auto& elem : portfolio.entries) {
        auto entryIt = m_entries.find(elem.symbol);
        if (entryIt == m_entries.end()) {
            continue;
        }

        if (std::holds_alternative<uint64_t>(elem.quantity) == false) {
            return Error::UnexpectedData;
        }

        if (quantities[elem.symbol] != std::get<uint64_t>(elem.quantity)) {
            return Error::InvalidData;
        }

        entryIt->second.value =
            elem.market_price * std::get<uint64_t>(elem.quantity);

        quantities.erase(elem.symbol);
    }

    for (auto it : quantities) {
        if (it.second != 0) {
            return Error::InvalidData;
        }
    }

    return Error::NoError;
}
