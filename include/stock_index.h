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

using Indexes            = std::vector<Index>;
using IndexesPerformance = std::vector<IndexPerformance>;
