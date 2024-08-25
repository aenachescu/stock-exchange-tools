#ifndef STOCK_EXCHANGE_TOOLS_TRADEVILLE_PORTFOLIO_FILTERS_H
#define STOCK_EXCHANGE_TOOLS_TRADEVILLE_PORTFOLIO_FILTERS_H

#include "stock_index.h"
#include "tradeville.h"

#include <algorithm>
#include <memory>
#include <ranges>
#include <unordered_set>
#include <vector>

class IPortfolioFilter {
public:
    IPortfolioFilter() = default;
    virtual ~IPortfolioFilter() noexcept
    {
    }

    virtual bool Match(const Portfolio::Entry& entry) const noexcept = 0;
};

class PortfolioFilters {
public:
    PortfolioFilters()  = default;
    ~PortfolioFilters() = default;

    void AddFilter(std::unique_ptr<IPortfolioFilter> filter)
    {
        m_filters.emplace_back(std::move(filter));
    }

    void Filter(Portfolio& portfolio) const noexcept
    {
        portfolio.entries.erase(
            std::remove_if(
                portfolio.entries.begin(),
                portfolio.entries.end(),
                [this](const Portfolio::Entry& entry) -> bool {
                    return ! Match(entry);
                }),
            portfolio.entries.end());
    }

private:
    bool Match(const Portfolio::Entry& entry) const noexcept
    {
        for (const auto& filter : m_filters) {
            if (filter->Match(entry) == false) {
                return false;
            }
        }

        return true;
    }

private:
    std::vector<std::unique_ptr<IPortfolioFilter>> m_filters;
};

class PortfolioFilterByAsset : public IPortfolioFilter {
public:
    PortfolioFilterByAsset(bool include, AssetType asset)
        : m_asset(asset), m_include(include)
    {
    }
    ~PortfolioFilterByAsset() = default;

    bool Match(const Portfolio::Entry& entry) const noexcept override
    {
        return m_include == true ? entry.asset == m_asset
                                 : entry.asset != m_asset;
    }

private:
    const AssetType m_asset;
    const bool m_include;
};

class PortfolioFilterByCurrency : public IPortfolioFilter {
public:
    PortfolioFilterByCurrency(bool include, Currency currency)
        : m_currency(currency), m_include(include)
    {
    }
    ~PortfolioFilterByCurrency() = default;

    bool Match(const Portfolio::Entry& entry) const noexcept override
    {
        return m_include == true ? entry.currency == m_currency
                                 : entry.currency != m_currency;
    }

private:
    const Currency m_currency;
    const bool m_include;
};

class PortfolioFilterBySymbol : public IPortfolioFilter {
public:
    PortfolioFilterBySymbol(bool include, const CompanySymbol& symbol)
        : m_symbol(symbol), m_include(include)
    {
    }
    ~PortfolioFilterBySymbol() = default;

    bool Match(const Portfolio::Entry& entry) const noexcept override
    {
        return m_include == true ? entry.symbol == m_symbol
                                 : entry.symbol != m_symbol;
    }

private:
    const CompanySymbol m_symbol;
    const bool m_include;
};

class PortfolioFilterByIndex : public IPortfolioFilter {
public:
    PortfolioFilterByIndex(bool include, const Index& index)
        : m_include(include)
    {
        m_symbols.reserve(index.companies.size());
        for (const auto& company : index.companies) {
            m_symbols.insert(company.symbol);
        }
    }
    ~PortfolioFilterByIndex() = default;

    bool Match(const Portfolio::Entry& entry) const noexcept override
    {
        bool found = (m_symbols.find(entry.symbol) != m_symbols.end());
        return m_include == true ? found : ! found;
    }

private:
    std::unordered_set<CompanySymbol> m_symbols;
    const bool m_include;
};

#endif // STOCK_EXCHANGE_TOOLS_TRADEVILLE_PORTFOLIO_FILTERS_H
