#ifndef STOCK_EXCHANGE_TOOLS_STOCK_INDEX_H
#define STOCK_EXCHANGE_TOOLS_STOCK_INDEX_H

#include <chrono>
#include <string>
#include <vector>

using IndexName     = std::string;
using CompanyName   = std::string;
using CompanySymbol = std::string;
using IndexesNames  = std::vector<IndexName>;

struct Company
{
    CompanySymbol symbol;
    CompanyName name;
    uint64_t shares                = 0;
    double reference_price         = 0.0;
    double free_float_factor       = 0.0;
    double representation_factor   = 0.0;
    double price_correction_factor = 0.0;
    double liquidity_factor        = 1.0;
    double weight                  = 0.0; // percentage
};

struct CompanyTradingData
{
    CompanySymbol symbol;
    double price         = 0.0;
    double variation     = 0.0; // percentage
    uint64_t trades      = 0;
    uint64_t volume      = 0;
    double value         = 0.0;
    double lowest_price  = 0.0;
    double highest_price = 0.0;
    double weight        = 0.0; // percentage
};

struct Index
{
    IndexName name;
    std::string date;
    std::string reason;
    std::vector<Company> companies;
};

struct IndexTradingData
{
    IndexName name;
    std::string date;
    std::vector<CompanyTradingData> companies;
};

struct IndexPerformance
{
    IndexName name;
    double today        = 0.0; // percentage
    double one_week     = 0.0; // percentage
    double one_month    = 0.0; // percentage
    double six_months   = 0.0; // percentage
    double one_year     = 0.0; // percentage
    double year_to_date = 0.0; // percentage
};

struct DividendActivity
{
    CompanySymbol symbol;
    CompanyName name;
    double dvd_value       = 0.0;
    double dvd_total_value = 0.0;
    double dvd_yield       = 0.0; // percentage
    uint16_t year          = 0;
    std::chrono::year_month_day ex_dvd_date;
    std::chrono::year_month_day record_date;
    std::chrono::year_month_day payment_date;
};

struct ComparableIndex
{
    const Index& index;
    uint16_t year;
    uint8_t month;
    uint8_t day;

    ComparableIndex(const Index& i, uint16_t y, uint8_t m, uint8_t d)
        : index(i), year(y), month(m), day(d)
    {
    }
};

struct IndexComparator
{
    bool operator()(const ComparableIndex& a, const ComparableIndex& b) const
    {
        if (a.year == b.year) {
            if (a.month == b.month) {
                if (a.day == b.day) {
                    return a.index.reason < b.index.reason;
                }
                return a.day < b.day;
            }
            return a.month < b.month;
        }
        return a.year < b.year;
    }
};

using Indexes            = std::vector<Index>;
using IndexesPerformance = std::vector<IndexPerformance>;
using DividendActivities = std::vector<DividendActivity>;

#endif // STOCK_EXCHANGE_TOOLS_STOCK_INDEX_H
