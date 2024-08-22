#ifndef STOCK_EXCHANGE_TOOLS_TRADEVILLE_ACTIVITY_FILTERS_H
#define STOCK_EXCHANGE_TOOLS_TRADEVILLE_ACTIVITY_FILTERS_H

#include "tradeville.h"

#include <memory>
#include <vector>

class IActivityFilter {
public:
    IActivityFilter() = default;
    virtual ~IActivityFilter() noexcept
    {
    }

    virtual bool Match(const Activity& activity) const noexcept = 0;
};

class ActivityFilters {
public:
    ActivityFilters()  = default;
    ~ActivityFilters() = default;

    void AddFilter(std::unique_ptr<IActivityFilter> filter)
    {
        m_filters.emplace_back(std::move(filter));
    }

    bool Match(const Activity& activity) const noexcept
    {
        for (const auto& filter : m_filters) {
            if (filter->Match(activity) == false) {
                return false;
            }
        }

        return true;
    }

private:
    std::vector<std::unique_ptr<IActivityFilter>> m_filters;
};

class ActivityFilterByYear : public IActivityFilter {
public:
    ActivityFilterByYear(uint64_t year) : m_year(year)
    {
    }
    ~ActivityFilterByYear() = default;

    bool Match(const Activity& activity) const noexcept override
    {
        size_t pos = activity.date.find('-');
        if (pos == std::string::npos) {
            return false;
        }

        return std::stoull(activity.date.substr(0, pos)) == m_year;
    }

private:
    const uint64_t m_year;
};

class ActivityFilterByType : public IActivityFilter {
public:
    ActivityFilterByType(ActivityType type) : m_type(type)
    {
    }
    ~ActivityFilterByType() = default;

    bool Match(const Activity& activity) const noexcept override
    {
        return activity.type == m_type;
    }

private:
    const ActivityType m_type;
};

class ActivityFilterBySymbol : public IActivityFilter {
public:
    ActivityFilterBySymbol(const std::string& symbol) : m_symbol(symbol)
    {
    }
    ~ActivityFilterBySymbol() = default;

    bool Match(const Activity& activity) const noexcept override
    {
        return activity.symbol == m_symbol;
    }

private:
    const std::string m_symbol;
};

class ActivityFilterByCurrency : public IActivityFilter {
public:
    ActivityFilterByCurrency(Currency currency) : m_currency(currency)
    {
    }
    ~ActivityFilterByCurrency() = default;

    bool Match(const Activity& activity) const noexcept override
    {
        return activity.currency == m_currency;
    }

private:
    const Currency m_currency;
};

#endif // STOCK_EXCHANGE_TOOLS_TRADEVILLE_ACTIVITY_FILTERS_H
