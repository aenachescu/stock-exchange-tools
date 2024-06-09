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
        m_entries.clear();
        return tl::unexpected(err);
    }

    CalculateDividends(activities);

    return CalculateReplication(cashAmmount);
}

tl::expected<uint64_t, Error> IndexReplication::GetPortfolioValue(
    const Index& index,
    const Portfolio& portfolio)
{
    double value = 0.0;

    for (const auto& company : index.companies) {
        for (const auto& entry : portfolio.entries) {
            if (company.symbol != entry.symbol) {
                continue;
            }

            if (std::holds_alternative<uint64_t>(entry.quantity) == false) {
                return tl::unexpected(Error::InvalidData);
            }

            value += entry.market_price * std::get<uint64_t>(entry.quantity);

            break;
        }
    }

    if (std::floor(value) == value) {
        return static_cast<uint64_t>(value);
    }

    return static_cast<uint64_t>(value) + 1;
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

        Entry& entry    = entryIt->second;
        uint64_t shares = std::get<uint64_t>(activity.quantity);
        double ammount  = std::fabs(activity.cash_ammount);
        double cost     = activity.price * shares;

        entry.actual_cost += cost;
        entry.commission += (ammount - cost);
        entry.shares += shares;
    }

    for (const auto& elem : portfolio.entries) {
        auto entryIt = m_entries.find(elem.symbol);
        if (entryIt == m_entries.end()) {
            continue;
        }

        if (std::holds_alternative<uint64_t>(elem.quantity) == false) {
            return Error::UnexpectedData;
        }

        if (entryIt->second.shares != std::get<uint64_t>(elem.quantity)) {
            return Error::InvalidData;
        }

        Entry& entry = entryIt->second;

        entry.market_price = elem.market_price;
        entry.avg_price    = entry.actual_cost / entry.shares;
        entry.value        = entry.market_price * entryIt->second.shares;
    }

    return Error::NoError;
}

void IndexReplication::CalculateDividends(const Activities& activities)
{
    for (const auto& activity : activities) {
        if (activity.type != ActivityType::Dividend) {
            continue;
        }

        auto entryIt = m_entries.find(activity.transaction_id);
        if (entryIt == m_entries.end()) {
            continue;
        }

        entryIt->second.dividends +=
            (activity.cash_ammount - activity.commission);
    }
}
