#include "index_replication.h"

#include "chrono_utils.h"

#include <algorithm>
#include <cfenv>
#include <cmath>

using IR = IndexReplication;

uint64_t ceilToU64(double d)
{
    int save_round = std::fegetround();
    std::fesetround(FE_UPWARD);
    uint64_t result = static_cast<uint64_t>(std::llrint(d));
    std::fesetround(save_round);
    return result;
}

tl::expected<IR::Entries, Error> IndexReplication::CalculateReplication(
    uint64_t cashAmount)
{
    if (m_entries.empty()) {
        return tl::unexpected(Error::NoData);
    }

    FillIndexStatistics(cashAmount);

    std::vector<Entry> res;
    res.reserve(m_entries.size());

    for (const auto& it : m_entries) {
        res.push_back(it.second);
    }

    std::sort(res.begin(), res.end(), EntryCmp{});

    return res;
}

tl::expected<IR::Entries, Error> IndexReplication::CalculateReplication(
    const Index& index,
    const Portfolio& portfolio,
    const Activities& activities,
    const DividendActivities& dvdActivities,
    uint64_t cashAmount)
{
    Error err = Error::NoError;

    do {
        err = FillIndexData(index);
        if (err != Error::NoError) {
            break;
        }

        err = FillPortfolioData(portfolio);
        if (err != Error::NoError) {
            break;
        }

        err = FillActivityData(activities);
        if (err != Error::NoError) {
            break;
        }

        FillDividendEstimates(activities, dvdActivities);
        if (err != Error::NoError) {
            break;
        }

        FillPortfolioStatistics();

        return CalculateReplication(cashAmount);
    } while (false);

    m_entries.clear();

    return tl::unexpected(err);
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

    return ceilToU64(value);
}

Error IndexReplication::FillIndexData(const Index& index)
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

Error IndexReplication::FillPortfolioData(const Portfolio& portfolio)
{
    for (const auto& elem : portfolio.entries) {
        auto entryIt = m_entries.find(elem.symbol);
        if (entryIt == m_entries.end()) {
            continue;
        }

        if (std::holds_alternative<uint64_t>(elem.quantity) == false) {
            return Error::UnexpectedData;
        }

        Entry& entry = entryIt->second;

        entry.shares       = std::get<uint64_t>(elem.quantity);
        entry.market_price = elem.market_price;
    }

    return Error::NoError;
}

Error IndexReplication::FillActivityData(const Activities& activities)
{
    std::map<CompanySymbol, uint64_t> sharesPerSymbol;

    for (const auto& it : m_entries) {
        sharesPerSymbol.emplace(it.first, 0ull);
    }

    for (const auto& activity : activities) {
        if (activity.type != ActivityType::Buy &&
            activity.type != ActivityType::AssetTransfer &&
            activity.type != ActivityType::Dividend) {
            continue;
        }

        auto entryIt = m_entries.find(activity.symbol);
        if (entryIt == m_entries.end()) {
            continue;
        }

        Entry& entry = entryIt->second;

        if (activity.type == ActivityType::Buy ||
            activity.type == ActivityType::AssetTransfer) {
            if (std::holds_alternative<uint64_t>(activity.quantity) == false) {
                return Error::UnexpectedData;
            }

            uint64_t shares = std::get<uint64_t>(activity.quantity);
            double amount   = std::fabs(activity.cash_amount);
            double cost     = activity.price * shares;

            entry.cost += cost;
            entry.commission += (amount - cost);

            sharesPerSymbol[activity.symbol] += shares;
        } else if (activity.type == ActivityType::Dividend) {
            entry.dividends += activity.cash_amount;
        }
    }

    for (const auto& it : m_entries) {
        auto sharesIt = sharesPerSymbol.find(it.first);
        if (sharesIt == sharesPerSymbol.end()) {
            return Error::InvalidData;
        }

        if (it.second.shares != sharesIt->second) {
            return Error::InvalidData;
        }

        sharesPerSymbol.erase(sharesIt);
    }

    if (sharesPerSymbol.size() != 0) {
        return Error::InvalidData;
    }

    return Error::NoError;
}

Error IndexReplication::FillDividendEstimates(
    const Activities& activities,
    const DividendActivities& dvd)
{
    Error err        = Error::NoError;
    const auto today = ymd_today();

    for (auto& it : m_entries) {
        Entry& e   = it.second;
        auto dvdIt = FindDividendActivity(dvd, e.symbol, today);
        if (dvdIt == dvd.end()) {
            continue;
        }

        err = CalculateEstimateShares(
            activities,
            e.symbol,
            dvdIt->ex_dvd_date,
            e.estimated_shares);
        if (err != Error::NoError) {
            return err;
        }

        e.estimated_dvd     = e.estimated_shares * dvdIt->dvd_value;
        e.estimated_net_dvd = e.estimated_dvd * 0.92;
        e.ex_date           = dvdIt->ex_dvd_date;
        e.record_date       = dvdIt->record_date;
        e.payment_date      = dvdIt->payment_date;
    }

    return Error::NoError;
}

void IndexReplication::FillPortfolioStatistics()
{
    for (auto& it : m_entries) {
        Entry& e = it.second;

        e.value        = e.shares * e.market_price;
        e.avg_price    = e.cost / e.shares;
        e.profit_loss  = e.value - e.cost;
        e.total_return = e.profit_loss + e.dividends;

        e.profit_loss_percentage  = e.profit_loss / e.cost * 100.0;
        e.total_return_percentage = e.total_return / e.cost * 100.0;
    }
}

void IndexReplication::FillIndexStatistics(uint64_t cashAmount)
{
    for (auto& it : m_entries) {
        Entry& e = it.second;

        e.target_value           = e.weight * cashAmount;
        e.delta_cost             = e.cost - e.target_value;
        e.delta_value            = e.value - e.target_value;
        e.delta_value_percentage = e.delta_value / e.target_value * 100.0;

        e.target_shares = ceilToU64(e.target_value / e.market_price);
        e.delta_shares  = e.shares - e.target_shares;
        e.delta_shares_percentage =
            static_cast<double>(e.delta_shares) / e.target_shares * 100.0;
    }
}

DividendActivities::const_iterator IndexReplication::FindDividendActivity(
    const DividendActivities& dvdActivities,
    const CompanySymbol& symbol,
    const std::chrono::year_month_day& today)
{
    for (auto it = dvdActivities.begin(); it != dvdActivities.end(); ++it) {
        if (it->symbol == symbol && today < it->payment_date) {
            return it;
        }
    }

    return dvdActivities.end();
}

Error IndexReplication::CalculateEstimateShares(
    const Activities& activities,
    const CompanySymbol& symbol,
    const std::chrono::year_month_day& date,
    uint64_t& shares)
{
    shares = 0;

    for (const auto& activity : activities) {
        if (activity.type != ActivityType::Buy &&
            activity.type != ActivityType::AssetTransfer) {
            continue;
        }

        if (activity.ymd >= date) {
            continue;
        }

        if (activity.symbol != symbol) {
            continue;
        }

        if (std::holds_alternative<uint64_t>(activity.quantity) == false) {
            return Error::UnexpectedData;
        }

        shares += std::get<uint64_t>(activity.quantity);
    }

    return Error::NoError;
}
