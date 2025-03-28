#include "bvb_scraper.h"
#include "chrono_utils.h"
#include "cli_utils.h"
#include "config.h"
#include "index_replication.h"
#include "string_utils.h"
#include "terminal_ui.h"
#include "tradeville.h"
#include "tradeville_activity_filters.h"
#include "tradeville_portfolio_filters.h"

#include <iostream>
#include <magic_enum.hpp>
#include <string_view>
#include <unordered_map>

bool IsValidConfig(const Config& cfg)
{
    if (cfg.GetBroker().value_or("") != "tradeville") {
        std::cout << "unsupported broker" << std::endl;
        return false;
    }

    if (cfg.GetStockExchange().value_or("") != "bvb") {
        std::cout << "unsupported stock exchange" << std::endl;
        return false;
    }

    if (! cfg.GetTradevilleUser() || ! cfg.GetTradevillePass()) {
        std::cout << "tradeville credentials are not set" << std::endl;
        return false;
    }

    if (! cfg.GetTradevilleStartYear()) {
        std::cout << "tradeville start year is not set" << std::endl;
        return false;
    }

    if (! is_number(*cfg.GetTradevilleStartYear())) {
        std::cout << "tradeville start year is not number" << std::endl;
        return false;
    }

    if (! cfg.GetIndexName()) {
        std::cout << "index is not set" << std::endl;
        return false;
    }

    if (! cfg.GetIndexAdjustmentDate() || ! cfg.GetIndexAdjustmentReason()) {
        std::cout << "index adjustment is not set" << std::endl;
        return false;
    }

    return true;
}

tl::expected<Index, Error> GetConfiguredIndex(const Config& cfg)
{
    BvbScraper bvb;

    auto indexes = bvb.LoadAdjustmentsHistoryFromFile(*cfg.GetIndexName());
    if (! indexes) {
        return tl::unexpected(indexes.error());
    }

    for (const auto& i : *indexes) {
        if (i.date == *cfg.GetIndexAdjustmentDate() &&
            i.reason == *cfg.GetIndexAdjustmentReason()) {
            return i;
        }
    }

    return tl::unexpected(Error::InvalidArg);
}

void PrintAssetAndCurrencyValue(
    const AssetAndCurrencyValue& val,
    const char* type)
{
    std::cout << std::endl;
    std::cout << type << " by asset and currency:" << std::endl;

    for (const auto& asset : val) {
        bool first = true;

        std::cout << magic_enum::enum_name(asset.first) << ": ";

        for (const auto& currency : asset.second) {
            if (first == true) {
                first = false;
            } else {
                std::cout << ", ";
            }

            std::cout << double_to_string(currency.second) << " "
                      << magic_enum::enum_name(currency.first);
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;
}

int CmdPrintPortfolio(
    const Config& cfg,
    const Portfolio::SortFields& sortFields,
    const Portfolio::SortEstDvdFields& sortEstDvdFields,
    const PortfolioFilters& filters)
{
    ColorizedTable table, estDvdTable;
    size_t id = 1;
    BvbScraper bvb;
    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());
    uint64_t startYear = std::stoull(*cfg.GetTradevilleStartYear());
    uint64_t endYear   = get_current_year();

    auto dvdActivities = bvb.GetDividendActivities();
    if (! dvdActivities) {
        std::cout << "Failed to get dividend activities from BVB: "
                  << magic_enum::enum_name(dvdActivities.error()) << std::endl;
        return -1;
    }

    auto portfolio = tv.GetPortfolio();
    if (! portfolio) {
        std::cout << "Failed to get portfolio: "
                  << magic_enum::enum_name(portfolio.error()) << std::endl;
        return -1;
    }

    auto activities = tv.GetActivity(std::nullopt, startYear, endYear);
    if (! activities) {
        std::cout << "Failed to get activity: "
                  << magic_enum::enum_name(activities.error()) << std::endl;
        return -1;
    }

    filters.Filter(*portfolio);

    auto err = portfolio->FillStatistics(*activities, *dvdActivities);
    if (err != Error::NoError) {
        std::cout << "Failed to fill portfolio statistics: "
                  << magic_enum::enum_name(err) << std::endl;
        return -1;
    }

    portfolio->Sort(sortFields);
    portfolio->Sort(sortEstDvdFields);

    table.reserve(portfolio->entries.size() + 1);
    table.emplace_back(std::vector<ColorizedString>{
        "#",
        "Account",
        "Symbol",
        "Quantity",
        "Avg price",
        "Market price",
        "Cost",
        "Value",
        "Dvd",
        "P/L",
        "P/L %",
        "TR",
        "TR %",
        "Currency",
        "Asset",
    });

    auto get_color = []<typename T>(T val) -> Color {
        return val < 0 ? Color::Red : Color::Green;
    };
    auto get_percentage = [](double val) -> std::string {
        if (std::isnan(val) || std::isinf(val)) {
            return "-";
        }
        return double_to_string(val);
    };

    for (const auto& i : portfolio->entries) {
        table.emplace_back(std::vector<ColorizedString>{
            std::to_string(id),
            i.account,
            i.symbol,
            quantity_to_string(i.quantity),
            double_to_string(i.avg_price, 4),
            double_to_string(i.market_price, 4),
            double_to_string(i.cost),
            double_to_string(i.value),
            double_to_string(i.dividends),
            ColorizedString{
                double_to_string(i.profit_loss),
                get_color(i.profit_loss)},
            ColorizedString{
                get_percentage(i.profit_loss_percentage),
                get_color(i.profit_loss)},
            ColorizedString{
                double_to_string(i.total_return),
                get_color(i.total_return)},
            ColorizedString{
                get_percentage(i.total_return_percentage),
                get_color(i.total_return)},
            std::string{magic_enum::enum_name(i.currency)},
            std::string{magic_enum::enum_name(i.asset)},
        });
        id++;
    }

    print_table(table);

    if (portfolio->estimated_dividends.empty() == false) {
        const auto today = ymd_today();

        estDvdTable.reserve(portfolio->estimated_dividends.size() + 1);
        estDvdTable.emplace_back(std::vector<ColorizedString>{
            "#",
            "Symbol",
            "Est. dvd",
            "Est. net dvd",
            "Est. shares",
            "Ex date",
            "Record date",
            "Payment date",
        });

        id = 1;

        for (const auto& i : portfolio->estimated_dividends) {
            auto estDvdColor = today < i.ex_date ? Color::Red : Color::Green;

            estDvdTable.emplace_back(std::vector<ColorizedString>{
                std::to_string(id),
                i.symbol,
                ColorizedString{double_to_string(i.estimated_dvd), estDvdColor},
                double_to_string(i.estimated_net_dvd),
                std::to_string(i.estimated_shares),
                date_to_string(i.ex_date),
                date_to_string(i.record_date),
                date_to_string(i.payment_date),
            });

            id++;
        }

        std::cout << std::endl;
        print_table(estDvdTable);
    }

    PrintAssetAndCurrencyValue(portfolio->GetCostByAssetAndCurrency(), "Cost");
    PrintAssetAndCurrencyValue(
        portfolio->GetValueByAssetAndCurrency(),
        "Value");
    PrintAssetAndCurrencyValue(
        portfolio->GetDvdByAssetAndCurrency(),
        "Dividends");
    PrintAssetAndCurrencyValue(
        portfolio->GetProfitByAssetAndCurrency(),
        "Profit");
    PrintAssetAndCurrencyValue(
        portfolio->GetTotalReturnByAssetAndCurrency(),
        "Total return");

    return 0;
}

int CmdPrintActivity(const Config& cfg, const ActivityFilters& filters)
{
    Table table;
    size_t id = 1;
    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());
    uint64_t startYear = std::stoull(*cfg.GetTradevilleStartYear());
    uint64_t endYear   = get_current_year();

    auto activities = tv.GetActivity(std::nullopt, startYear, endYear);
    if (! activities) {
        std::cout << "Failed to get activity: "
                  << magic_enum::enum_name(activities.error()) << std::endl;
        return -1;
    }

    table.reserve(activities->size() + 1);
    table.emplace_back(std::vector<std::string>{
        "#",
        "Date",
        "Type",
        "Symbol",
        "Quantity",
        "Price",
        "Commission",
        "Tax",
        "Amount",
        "Avg price",
        "Currency",
        "Note",
        "Market",
        "Transaction id",
        "Order Id",
        "Asset pos",
        "Cash pos",
        "Profit",
    });

    for (const auto& i : *activities) {
        if (filters.Match(i) == false) {
            continue;
        }

        table.emplace_back(std::vector<std::string>{
            std::to_string(id),
            i.date,
            std::string{magic_enum::enum_name(i.type)},
            i.symbol,
            quantity_to_string(i.quantity),
            double_to_string(i.price, 4),
            double_to_string(i.commission, 4),
            double_to_string(i.tax, 4),
            double_to_string(i.cash_amount, 4),
            double_to_string(i.avg_price, 4),
            std::string{magic_enum::enum_name(i.currency)},
            i.note,
            i.market,
            i.transaction_id,
            std::to_string(i.order_id),
            std::to_string(i.asset_position),
            double_to_string(i.cash_position, 2),
            double_to_string(i.profit, 2),
        });
        id++;
    }

    print_table(table);

    return 0;
}

int CmdPrintDividends(const Config& cfg, uint64_t startYear, uint64_t endYear)
{
    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());
    std::map<uint64_t, std::map<Currency, double>> dividends;
    uint64_t year = 0;

    auto activities = tv.GetActivity(std::nullopt, startYear, endYear);
    if (! activities) {
        std::cout << "Failed to get activity: "
                  << magic_enum::enum_name(activities.error()) << std::endl;
        return -1;
    }

    for (size_t i = startYear; i <= endYear; i++) {
        dividends.emplace(i, std::map<Currency, double>{});
    }

    for (const auto& activity : *activities) {
        if (activity.type != ActivityType::Dividend) {
            continue;
        }

        if (get_year_from_ymd(activity.date, year) == false) {
            std::cout << "Failed to parse year from [" << activity.date << "]"
                      << std::endl;
            return -1;
        }

        if (year < startYear || year > endYear) {
            continue;
        }

        auto& currencyMap = dividends[year];
        auto res = currencyMap.emplace(activity.currency, activity.cash_amount);
        if (res.second == false) {
            res.first->second += activity.cash_amount;
        }
    }

    for (const auto& dvd : dividends) {
        std::cout << dvd.first << ": ";
        for (const auto& entry : dvd.second) {
            std::cout << double_to_string(entry.second)
                      << magic_enum::enum_name(entry.first) << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}

tl::expected<IndexReplication::Entries, Error> GetIndexReplication(
    const Config& cfg,
    uint64_t amount,
    bool addPortfolioValue)
{
    IndexReplication ir;
    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());
    BvbScraper bvb;
    const Index* index = nullptr;
    uint64_t startYear = std::stoull(*cfg.GetTradevilleStartYear());
    uint64_t endYear   = get_current_year();

    auto indexes = bvb.LoadAdjustmentsHistoryFromFile(*cfg.GetIndexName());
    if (! indexes) {
        std::cout << "Failed to load adjustments history: "
                  << magic_enum::enum_name(indexes.error()) << std::endl;
        return tl::unexpected(indexes.error());
    }

    for (const auto& i : *indexes) {
        if (i.date == *cfg.GetIndexAdjustmentDate() &&
            i.reason == *cfg.GetIndexAdjustmentReason()) {
            index = &i;
            break;
        }
    }

    if (index == nullptr) {
        std::cout << "No index adjustments found with specific date and reason"
                  << std::endl;
        return tl::unexpected(Error::InvalidArg);
    }

    auto dvdActivities = bvb.GetDividendActivities();
    if (! dvdActivities) {
        std::cout << "Failed to get dividend activities from BVB: "
                  << magic_enum::enum_name(dvdActivities.error()) << std::endl;
        return tl::unexpected(dvdActivities.error());
    }

    auto portfolio = tv.GetPortfolio();
    if (! portfolio) {
        std::cout << "Failed to get portfolio: "
                  << magic_enum::enum_name(portfolio.error()) << std::endl;
        return tl::unexpected(portfolio.error());
    }

    auto activities = tv.GetActivity(std::nullopt, startYear, endYear);
    if (! activities) {
        std::cout << "Failed to get activity: "
                  << magic_enum::enum_name(activities.error()) << std::endl;
        return tl::unexpected(activities.error());
    }

    if (addPortfolioValue == true) {
        auto portfolioValue = ir.GetPortfolioValue(*index, *portfolio);
        if (! portfolioValue) {
            std::cout << "Failed to get portfolio value: "
                      << magic_enum::enum_name(portfolioValue.error())
                      << std::endl;
            return tl::unexpected(portfolioValue.error());
        }

        amount += *portfolioValue;
    }

    auto replication = ir.CalculateReplication(
        *index,
        *portfolio,
        *activities,
        *dvdActivities,
        amount);
    if (! replication) {
        std::cout << "Failed to calculate index replication: "
                  << magic_enum::enum_name(replication.error()) << std::endl;
        return tl::unexpected(replication.error());
    }

    return replication;
}

int CmdPrintIndexReplication(
    const Config& cfg,
    uint64_t amount,
    bool addPortfolioValue)
{
    ColorizedTable indexReplicationTable;
    ColorizedTable profitTable;
    size_t id                    = 1;
    double sumWeight             = 0.0;
    double sumTargetValue        = 0.0;
    double sumCost               = 0.0;
    double sumValue              = 0.0;
    double sumCommission         = 0.0;
    double sumDeltaCost          = 0.0;
    double sumDeltaValue         = 0.0;
    double sumDividends          = 0.0;
    double sumEstDividends       = 0.0;
    double sumEstNetDividends    = 0.0;
    double sumNegativeDeltaCost  = 0.0;
    double sumNegativeDeltaValue = 0.0;
    double sumAllDeltaValues     = 0.0;
    bool hasEstDvd               = false;
    Color estDvdColor            = Color::Red;
    const auto today             = ymd_today();

    auto get_color = []<typename T>(T val) -> Color {
        return val < 0 ? Color::Red : Color::Green;
    };

    auto replication = GetIndexReplication(cfg, amount, addPortfolioValue);
    if (! replication) {
        return -1;
    }

    indexReplicationTable.reserve(replication->size() + 2);
    indexReplicationTable.emplace_back(std::vector<ColorizedString>{
        "#",
        "Symbol",
        "Weight",
        "Avg price",
        "Market price",
        "Target value",
        "Cost",
        "Value",
        "Commission",
        "Delta cost",
        "Delta value",
        "Delta value %",
        "Shares",
        "Target shares",
        "Delta shares",
        "Delta shares %",
    });

    profitTable.reserve(replication->size() + 1);
    profitTable.emplace_back(std::vector<ColorizedString>{
        "#",
        "Symbol",
        "Cost",
        "Value",
        "Dvd",
        "P/L",
        "P/L %",
        "Total return",
        "Total return %",
        "Est. dvd",
        "Est. net dvd",
        "Est. shares",
        "Ex date",
        "Record date",
        "Payment date",
    });

    for (const auto& i : *replication) {
        sumWeight += i.weight;
        sumTargetValue += i.target_value;
        sumCost += i.cost;
        sumValue += i.value;
        sumCommission += i.commission;
        sumDeltaCost += i.delta_cost;
        sumDeltaValue += i.delta_value;
        sumDividends += i.dividends;
        sumEstDividends += i.estimated_dvd;
        sumEstNetDividends += i.estimated_net_dvd;
        sumAllDeltaValues += std::abs(i.delta_value);

        if (i.delta_cost < 0.0) {
            sumNegativeDeltaCost += i.delta_cost;
        }
        if (i.delta_value < 0.0) {
            sumNegativeDeltaValue += i.delta_value;
        }

        if (i.estimated_shares != 0) {
            hasEstDvd   = true;
            estDvdColor = today < i.ex_date ? Color::Red : Color::Green;
        } else {
            hasEstDvd = false;
        }
        hasEstDvd = i.estimated_shares != 0;

        indexReplicationTable.emplace_back(std::vector<ColorizedString>{
            std::to_string(id),
            i.symbol,
            double_to_string(i.weight * 100.0),
            double_to_string(i.avg_price, 6),
            double_to_string(i.market_price, 6),
            double_to_string(i.target_value),
            double_to_string(i.cost),
            double_to_string(i.value),
            double_to_string(i.commission),
            ColorizedString{
                double_to_string(i.delta_cost),
                get_color(i.delta_cost)},
            ColorizedString{
                double_to_string(i.delta_value),
                get_color(i.delta_value)},
            ColorizedString{
                double_to_string(i.delta_value_percentage),
                get_color(i.delta_value_percentage)},
            std::to_string(i.shares),
            std::to_string(i.target_shares),
            ColorizedString{
                double_to_string(i.delta_shares),
                get_color(i.delta_shares)},
            ColorizedString{
                double_to_string(i.delta_shares_percentage),
                get_color(i.delta_shares_percentage)},
        });

        profitTable.emplace_back(std::vector<ColorizedString>{
            std::to_string(id),
            i.symbol,
            double_to_string(i.cost),
            double_to_string(i.value),
            double_to_string(i.dividends),
            ColorizedString{
                double_to_string(i.profit_loss),
                get_color(i.profit_loss)},
            ColorizedString{
                double_to_string(i.profit_loss_percentage),
                get_color(i.profit_loss_percentage)},
            ColorizedString{
                double_to_string(i.total_return),
                get_color(i.total_return)},
            ColorizedString{
                double_to_string(i.total_return_percentage),
                get_color(i.total_return_percentage)},
            hasEstDvd
                ? ColorizedString{double_to_string(i.estimated_dvd), estDvdColor}
                : "-",
            hasEstDvd ? double_to_string(i.estimated_net_dvd) : "-",
            hasEstDvd ? std::to_string(i.estimated_shares) : "-",
            hasEstDvd ? date_to_string(i.ex_date) : "-",
            hasEstDvd ? date_to_string(i.record_date) : "-",
            hasEstDvd ? date_to_string(i.payment_date) : "-",
        });

        id++;
    }

    double pl  = sumValue - sumCost;
    double plp = pl / sumCost * 100.0;
    double tr  = sumValue + sumDividends - sumCost;
    double trp = tr / sumCost * 100.0;
    double dvp = sumAllDeltaValues / sumTargetValue * 100.0;

    indexReplicationTable.emplace_back(std::vector<ColorizedString>{
        "",
        "sum",
        double_to_string(sumWeight * 100.0),
        "",
        "",
        double_to_string(sumTargetValue),
        double_to_string(sumCost),
        double_to_string(sumValue),
        double_to_string(sumCommission),
        double_to_string(sumDeltaCost),
        double_to_string(sumDeltaValue),
        double_to_string(dvp),
        "",
        "",
        "",
        "",
    });
    indexReplicationTable.emplace_back(std::vector<ColorizedString>{
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        double_to_string(sumNegativeDeltaCost),
        double_to_string(sumNegativeDeltaValue),
        "",
        "",
        "",
        "",
        "",
    });

    profitTable.emplace_back(std::vector<ColorizedString>{
        "",
        "sum",
        double_to_string(sumCost),
        double_to_string(sumValue),
        double_to_string(sumDividends),
        ColorizedString{double_to_string(pl), get_color(pl)},
        ColorizedString{double_to_string(plp), get_color(plp)},
        ColorizedString{double_to_string(tr), get_color(tr)},
        ColorizedString{double_to_string(trp), get_color(trp)},
        double_to_string(sumEstDividends),
        double_to_string(sumEstNetDividends),
        "",
        "",
        "",
        "",
    });

    print_table(indexReplicationTable, {id});
    print_table(profitTable, {id});

    return 0;
}

int CmdSaveTradevilleActivity(const Config& cfg, uint64_t year)
{
    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());

    Error err = tv.SaveActivityToFile(year);
    if (err != Error::NoError) {
        std::cout << "Failed to save Tradeville activity to file: "
                  << magic_enum::enum_name(err) << std::endl;
        return -1;
    }

    std::cout << "Successfully saved Tradeville activity to file!" << std::endl;

    return 0;
}

int CmdSaveTradevillePortfolio(const Config& cfg)
{
    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());

    Error err = tv.SavePortfolioToFile();
    if (err != Error::NoError) {
        std::cout << "Failed to save Tradeville portfolio to file: "
                  << magic_enum::enum_name(err) << std::endl;
        return -1;
    }

    std::cout << "Successfully saved Tradeville portfolio to file!"
              << std::endl;

    return 0;
}

void CmdPrintHelp()
{
    std::cout << "Supported commands:" << std::endl;
    std::cout << "--ptvp - prints the portfolio from tradeville. Use --sort "
                 "q,ap,mp,c,v,d,p,pp,tr,trp,ccy,a in order to sort portfolio "
                 "entries. Use --sort-est-dvd d,s,ed,rd,pd in order to sort "
                 "estimated dividends entries. If the sort field stats with "
                 "capital letter then the field will be sorted in descending "
                 "order, otherwise ascending order. For example, `--sort "
                 "a,ccy,Tr` will sort the portfolio by asset type and currency "
                 "in ascending order and by total return in descending order. "
                 "Use --asset, --symbol, --currency, --index in order to "
                 "filter the portfolio."
              << std::endl;
    std::cout << "--ptva - prints the activity from tradeville. Use --type, "
                 "--year, --symbol, --currency in order to filter the activity."
              << std::endl;
    std::cout << "--ptvd <start_year> <end_year> - prints the dividend from "
                 "tradeville activity. If only start_year is set then it "
                 "prints the dividends just for that year. If both are set "
                 "then it prints dividends for each year from interval. If no "
                 "one is set then it prints dividends for current year."
              << std::endl;
    std::cout << "--ptvir <cash_amount> - prints the status of index "
                 "replication based on index adjustment from config file."
                 "If cash_amount argument is missing then the current value "
                 "of stocks will be used. If the cash_amount argument starts "
                 "with '+' then that amount will be added to current value of "
                 "stocks, this method can be used if you want to invest some "
                 "money and you want to replicate a specific index."
              << std::endl;
    std::cout << "--stva <year> - save the activity from tradeville to file."
              << std::endl;
    std::cout << "--stvp - save the portfolio from tradeville to file."
              << std::endl;
}

bool ParseActivityFilters(
    char* argv[],
    int argc,
    int start,
    ActivityFilters& filters)
{
    for (int i = start; i < argc; i++) {
        if (strcmp(argv[i], "--year") == 0) {
            if (i + 1 >= argc) {
                std::cout << "no year provided" << std::endl;
                return false;
            }

            uint64_t year = std::stoull(argv[i + 1]);
            filters.AddFilter(std::make_unique<ActivityFilterByYear>(year));
            i++;
            continue;
        }

        if (strcmp(argv[i], "--type") == 0) {
            if (i + 1 >= argc) {
                std::cout << "no type provided" << std::endl;
                return false;
            }

            auto type = magic_enum::enum_cast<ActivityType>(
                argv[i + 1],
                magic_enum::case_insensitive);
            if (type == std::nullopt) {
                std::cout << "invalid type" << std::endl;
                return false;
            }

            filters.AddFilter(std::make_unique<ActivityFilterByType>(*type));
            i++;
            continue;
        }

        if (strcmp(argv[i], "--symbol") == 0) {
            if (i + 1 >= argc) {
                std::cout << "no symbol provided" << std::endl;
                return false;
            }

            filters.AddFilter(
                std::make_unique<ActivityFilterBySymbol>(argv[i + 1]));
            i++;
            continue;
        }

        if (strcmp(argv[i], "--currency") == 0) {
            if (i + 1 >= argc) {
                std::cout << "no currency provided" << std::endl;
                return false;
            }

            auto currency = magic_enum::enum_cast<Currency>(
                argv[i + 1],
                magic_enum::case_insensitive);
            if (currency == std::nullopt) {
                std::cout << "invalid currency" << std::endl;
                return false;
            }

            filters.AddFilter(
                std::make_unique<ActivityFilterByCurrency>(*currency));
            i++;
            continue;
        }

        std::cout << "invalid filter" << std::endl;
        return false;
    }

    return true;
}

bool ParsePortfolioSortBy(Portfolio::SortFields& fields, const std::string& str)
{
    static const std::unordered_map<std::string_view, Portfolio::SortBy>
        kFields = {
            {"q", Portfolio::SortBy::Quantity},
            {"ap", Portfolio::SortBy::AvgPrice},
            {"mp", Portfolio::SortBy::MarketPrice},
            {"c", Portfolio::SortBy::Cost},
            {"v", Portfolio::SortBy::Value},
            {"d", Portfolio::SortBy::Dvd},
            {"p", Portfolio::SortBy::Profit},
            {"pp", Portfolio::SortBy::ProfitPercentage},
            {"tr", Portfolio::SortBy::TotalReturn},
            {"trp", Portfolio::SortBy::TotalReturnPercentage},
            {"ccy", Portfolio::SortBy::Currency},
            {"a", Portfolio::SortBy::Asset},
        };

    auto tokens = split_string(str, ',');
    fields.reserve(tokens.size());

    for (auto& token : tokens) {
        if (token.empty() == true) {
            std::cout << "empty SortBy token" << std::endl;
            return false;
        }

        auto sortOrder = std::islower(token[0])
            ? Portfolio::SortOrder::Ascending
            : Portfolio::SortOrder::Descending;

        std::transform(
            token.begin(),
            token.end(),
            token.begin(),
            [](unsigned char c) { return std::tolower(c); });

        auto it = kFields.find(token);
        if (it == kFields.end()) {
            std::cout << "unrecognized SortBy token: " << token << std::endl;
            return false;
        }

        fields.push_back(std::make_pair(it->second, sortOrder));
    }

    return true;
}

bool ParsePortfolioSortEstDvdBy(
    Portfolio::SortEstDvdFields& fields,
    const std::string& str)
{
    static const std::unordered_map<std::string_view, Portfolio::SortEstDvdBy>
        kFields = {
            {"d", Portfolio::SortEstDvdBy::Dividend},
            {"s", Portfolio::SortEstDvdBy::Shares},
            {"ed", Portfolio::SortEstDvdBy::ExDate},
            {"rd", Portfolio::SortEstDvdBy::RecordDate},
            {"pd", Portfolio::SortEstDvdBy::PaymentDate},
        };

    auto tokens = split_string(str, ',');
    fields.reserve(tokens.size());

    for (auto& token : tokens) {
        if (token.empty() == true) {
            std::cout << "empty SortEstDvdBy token" << std::endl;
            return false;
        }

        auto sortOrder = std::islower(token[0])
            ? Portfolio::SortOrder::Ascending
            : Portfolio::SortOrder::Descending;

        std::transform(
            token.begin(),
            token.end(),
            token.begin(),
            [](unsigned char c) { return std::tolower(c); });

        auto it = kFields.find(token);
        if (it == kFields.end()) {
            std::cout << "unrecognized SortEstDvdBy token: " << token
                      << std::endl;
            return false;
        }

        fields.push_back(std::make_pair(it->second, sortOrder));
    }

    return true;
}

bool ParsePtvpCommand(
    const Config& cfg,
    char* argv[],
    int argc,
    int start,
    Portfolio::SortFields& sortFields,
    Portfolio::SortEstDvdFields& sortEstDvdFields,
    PortfolioFilters& filters)
{
    bool res = true;

    for (int i = start; i < argc; i++) {
        std::string param(argv[i]);
        std::transform(
            param.begin(),
            param.end(),
            param.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (param == "--sort") {
            if (i + 1 >= argc) {
                std::cout << "no sort fields provided!" << std::endl;
                return false;
            }

            res = ParsePortfolioSortBy(sortFields, argv[i + 1]);
            if (res == false) {
                return res;
            }

            i++;
            continue;
        }

        if (param == "--sort-est-dvd") {
            if (i + 1 >= argc) {
                std::cout << "no sort est dvd fields provided!" << std::endl;
                return false;
            }

            res = ParsePortfolioSortEstDvdBy(sortEstDvdFields, argv[i + 1]);
            if (res == false) {
                return res;
            }

            i++;
            continue;
        }

        if (param == "--asset") {
            if (i + 1 >= argc) {
                std::cout << "no asset type provided!" << std::endl;
                return false;
            }

            auto assetType = magic_enum::enum_cast<AssetType>(
                argv[i + 1],
                magic_enum::case_insensitive);
            if (assetType == std::nullopt) {
                std::cout << "invalid asset type!" << std::endl;
                return false;
            }

            filters.AddFilter(std::make_unique<PortfolioFilterByAsset>(
                argv[i][2] == 'a',
                *assetType));

            i++;
            continue;
        }

        if (param == "--currency") {
            if (i + 1 >= argc) {
                std::cout << "no currency provided!" << std::endl;
                return false;
            }

            auto currency = magic_enum::enum_cast<Currency>(
                argv[i + 1],
                magic_enum::case_insensitive);
            if (currency == std::nullopt) {
                std::cout << "invalid currency!" << std::endl;
                return false;
            }

            filters.AddFilter(std::make_unique<PortfolioFilterByCurrency>(
                argv[i][2] == 'c',
                *currency));

            i++;
            continue;
        }

        if (param == "--symbol") {
            if (i + 1 >= argc) {
                std::cout << "no symbol provided!" << std::endl;
                return false;
            }

            filters.AddFilter(std::make_unique<PortfolioFilterBySymbol>(
                argv[i][2] == 's',
                argv[i + 1]));

            i++;
            continue;
        }

        if (param == "--index") {
            auto index = GetConfiguredIndex(cfg);
            if (! index) {
                std::cout << "failed to get configured index: "
                          << magic_enum::enum_name(index.error()) << std::endl;
            }

            filters.AddFilter(std::make_unique<PortfolioFilterByIndex>(
                argv[i][2] == 'i',
                *index));

            continue;
        }

        std::cout << "unknown parameter for ptvp command: " << argv[i]
                  << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[])
{
    Config cfg;
    Error err = cfg.LoadConfig();
    if (err != Error::NoError) {
        std::cout << "load config: " << magic_enum::enum_name(err) << std::endl;
        return -1;
    }

    if (! IsValidConfig(cfg)) {
        return -1;
    }

    if (argc < 2) {
        std::cout << "no command" << std::endl;
        CmdPrintHelp();
        return -1;
    }

    if (strcmp(argv[1], "--ptvp") == 0) {
        Portfolio::SortFields sortFields;
        Portfolio::SortEstDvdFields sortEstDvdFields;
        PortfolioFilters filters;

        bool res = ParsePtvpCommand(
            cfg,
            argv,
            argc,
            2,
            sortFields,
            sortEstDvdFields,
            filters);
        if (res == false) {
            return -1;
        }

        return CmdPrintPortfolio(cfg, sortFields, sortEstDvdFields, filters);
    } else if (strcmp(argv[1], "--ptva") == 0) {
        ActivityFilters filters;
        if (ParseActivityFilters(argv, argc, 2, filters) == false) {
            return -1;
        }

        return CmdPrintActivity(cfg, filters);
    } else if (strcmp(argv[1], "--ptvd") == 0) {
        uint64_t startYear, endYear;
        if (argc == 2) {
            startYear = endYear = get_current_year();
        } else if (argc == 3) {
            startYear = endYear = std::stoull(argv[2]);
        } else {
            startYear = std::stoull(argv[2]);
            endYear   = std::stoull(argv[3]);
        }

        return CmdPrintDividends(cfg, startYear, endYear);
    } else if (strcmp(argv[1], "--ptvir") == 0) {
        uint64_t amount        = 0;
        bool addPortfolioValue = false;

        if (argc == 2) {
            addPortfolioValue = true;
        } else if (argc > 2) {
            if (argv[2][0] == '+') {
                amount            = std::stoull(argv[2] + 1);
                addPortfolioValue = true;
            } else {
                amount = std::stoull(argv[2]);
            }
        }

        return CmdPrintIndexReplication(cfg, amount, addPortfolioValue);
    } else if (strcmp(argv[1], "--stva") == 0) {
        if (argc != 3) {
            std::cout << "no year provided" << std::endl;
            return -1;
        }

        uint64_t year = std::stoull(argv[2]);
        return CmdSaveTradevilleActivity(cfg, year);
    } else if (strcmp(argv[1], "--stvp") == 0) {
        return CmdSaveTradevillePortfolio(cfg);
    } else if (strcmp(argv[1], "--ui") == 0) {
        TerminalUi tui(cfg);

        Error err = tui.Init();
        if (err != Error::NoError) {
            std::cout << "Failed to init TerminalUI: "
                      << magic_enum::enum_name(err) << std::endl;
            return -1;
        }

        tui.Run();

        return 0;
    } else if (strcmp(argv[1], "--help") == 0) {
        CmdPrintHelp();
        return 0;
    }

    std::cout << "unknown command" << std::endl;
    CmdPrintHelp();

    return -1;
}