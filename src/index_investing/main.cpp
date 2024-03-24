#include "bvb_scraper.h"
#include "chrono_utils.h"
#include "cli_utils.h"
#include "config.h"
#include "index_replication.h"
#include "string_utils.h"
#include "terminal_ui.h"
#include "tradeville.h"

#include <chrono>
#include <iostream>
#include <magic_enum.hpp>

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

int CmdPrintPortfolio(const Config& cfg)
{
    Table table;
    size_t id = 1;
    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());

    auto portfolio = tv.GetPortfolio();
    if (! portfolio) {
        std::cout << "Failed to get portfolio: "
                  << magic_enum::enum_name(portfolio.error()) << std::endl;
        return -1;
    }

    table.reserve(portfolio->entries.size() + 1);
    table.emplace_back(std::vector<std::string>{
        "#",
        "Account",
        "Symbol",
        "Quantity",
        "Avg price",
        "Market price",
        "Cost",
        "Value",
        "P/L",
        "P/L %",
        "Currency",
        "Asset",
    });

    for (const auto& i : portfolio->entries) {
        double quantity = 0.0;
        if (std::holds_alternative<uint64_t>(i.quantity) == true) {
            quantity = std::get<uint64_t>(i.quantity);
        } else {
            quantity = std::get<double>(i.quantity);
        }

        double cost  = i.avg_price * quantity;
        double value = i.market_price * quantity;
        double pl    = value - cost;
        double plp   = pl / cost * 100.0;

        table.emplace_back(std::vector<std::string>{
            std::to_string(id),
            i.account,
            i.symbol,
            quantity_to_string(i.quantity),
            double_to_string(i.avg_price, 4),
            double_to_string(i.market_price, 4),
            double_to_string(cost),
            double_to_string(value),
            double_to_string(pl),
            double_to_string(plp),
            std::string{magic_enum::enum_name(i.currency)},
            std::string{magic_enum::enum_name(i.asset)},
        });
        id++;
    }

    print_table(table);

    std::cout << std::endl;
    std::cout << "Value by asset and currency:" << std::endl;
    auto assetAndCurrencyValue = portfolio->GetValueByAssetAndCurrency();
    for (const auto& asset : assetAndCurrencyValue) {
        std::cout << magic_enum::enum_name(asset.first) << ": ";
        for (const auto& currency : asset.second) {
            std::cout << double_to_string(currency.second) << " "
                      << magic_enum::enum_name(currency.first) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    return 0;
}

int CmdPrintActivity(const Config& cfg)
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
        "Ammount",
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
        table.emplace_back(std::vector<std::string>{
            std::to_string(id),
            i.date,
            std::string{magic_enum::enum_name(i.type)},
            i.symbol,
            quantity_to_string(i.quantity),
            double_to_string(i.price, 4),
            double_to_string(i.commission, 4),
            double_to_string(i.tax, 4),
            double_to_string(i.cash_ammount, 4),
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
        auto res =
            currencyMap.emplace(activity.currency, activity.cash_ammount);
        if (res.second == false) {
            res.first->second += activity.cash_ammount;
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
    uint64_t ammount,
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

        ammount += *portfolioValue;
    }

    auto replication =
        ir.CalculateReplication(*index, *portfolio, *activities, ammount);
    if (! replication) {
        std::cout << "Failed to calculate index replication: "
                  << magic_enum::enum_name(replication.error()) << std::endl;
        return tl::unexpected(replication.error());
    }

    return replication;
}

int CmdPrintIndexReplication(
    const Config& cfg,
    uint64_t ammount,
    bool addPortfolioValue)
{
    ColorizedTable table;
    size_t id                    = 1;
    double sumWeight             = 0.0;
    double sumTargetCost         = 0.0;
    double sumActualCost         = 0.0;
    double sumValue              = 0.0;
    double sumCommission         = 0.0;
    double sumDeltaCost          = 0.0;
    double sumDeltaValue         = 0.0;
    double sumDividends          = 0.0;
    double sumNegativeDeltaCost  = 0.0;
    double sumNegativeDeltaValue = 0.0;
    double sumAllDeltaValues     = 0.0;

    auto get_color = [](double val) -> Color {
        return val < 0.0 ? Color::Red : Color::Green;
    };

    auto replication = GetIndexReplication(cfg, ammount, addPortfolioValue);
    if (! replication) {
        return -1;
    }

    table.reserve(replication->size() + 1);
    table.emplace_back(std::vector<ColorizedString>{
        "#",
        "Symbol",
        "Weight",
        "Target cost",
        "Actual cost",
        "Value",
        "Commission",
        "Delta cost",
        "Delta value",
        "Delta value %",
        "Dvd",
        "P/L",
        "P/L %",
        "Total return",
        "Total return %",
    });

    for (const auto& i : *replication) {
        sumWeight += i.weight;
        sumTargetCost += i.target_cost;
        sumActualCost += i.actual_cost;
        sumValue += i.value;
        sumCommission += i.commission;
        sumDeltaCost += i.delta_cost;
        sumDeltaValue += i.delta_value;
        sumDividends += i.dividends;
        sumAllDeltaValues += std::abs(i.delta_value);

        if (i.delta_cost < 0.0) {
            sumNegativeDeltaCost += i.delta_cost;
        }
        if (i.delta_value < 0.0) {
            sumNegativeDeltaValue += i.delta_value;
        }

        double pl  = i.value - i.actual_cost;
        double plp = pl / i.actual_cost * 100.0;
        double tr  = i.value + i.dividends - i.actual_cost;
        double trp = tr / i.actual_cost * 100.0;
        double dvp = i.delta_value / i.target_cost * 100.0;

        table.emplace_back(std::vector<ColorizedString>{
            std::to_string(id),
            i.symbol,
            double_to_string(i.weight * 100.0),
            double_to_string(i.target_cost),
            double_to_string(i.actual_cost),
            double_to_string(i.value),
            double_to_string(i.commission),
            ColorizedString{
                double_to_string(i.delta_cost),
                get_color(i.delta_cost)},
            ColorizedString{
                double_to_string(i.delta_value),
                get_color(i.delta_value)},
            ColorizedString{double_to_string(dvp), get_color(dvp)},
            double_to_string(i.dividends),
            ColorizedString{double_to_string(pl), get_color(pl)},
            ColorizedString{double_to_string(plp), get_color(plp)},
            ColorizedString{double_to_string(tr), get_color(tr)},
            ColorizedString{double_to_string(trp), get_color(trp)},
        });
        id++;
    }

    double pl  = sumValue - sumActualCost;
    double plp = pl / sumActualCost * 100.0;
    double tr  = sumValue + sumDividends - sumActualCost;
    double trp = tr / sumActualCost * 100.0;
    double dvp = sumAllDeltaValues / sumTargetCost * 100.0;

    table.emplace_back(std::vector<ColorizedString>{
        "",
        "sum",
        double_to_string(sumWeight * 100.0),
        double_to_string(sumTargetCost),
        double_to_string(sumActualCost),
        double_to_string(sumValue),
        double_to_string(sumCommission),
        double_to_string(sumDeltaCost),
        double_to_string(sumDeltaValue),
        double_to_string(dvp),
        double_to_string(sumDividends),
        ColorizedString{double_to_string(pl), get_color(pl)},
        ColorizedString{double_to_string(plp), get_color(plp)},
        ColorizedString{double_to_string(tr), get_color(tr)},
        ColorizedString{double_to_string(trp), get_color(trp)},
    });
    table.emplace_back(std::vector<ColorizedString>{
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
        "",
    });

    print_table(table, {id});

    return 0;
}

void CmdPrintHelp()
{
    std::cout << "Supported commands:" << std::endl;
    std::cout << "--ptvp - prints the portfolio from tradeville" << std::endl;
    std::cout << "--ptva - prints the activity from tradeville" << std::endl;
    std::cout << "--ptvd <start_year> <end_year> - prints the dividend from "
                 "tradeville activity. If only start_year is set then it "
                 "prints the dividends just for that year. If both are set "
                 "then it prints dividends for each year from interval. If no "
                 "one is set then it prints dividends for current year."
              << std::endl;
    std::cout << "--ptvir <cash_ammount> - prints the status of index "
                 "replication based on index adjustment from config file."
                 "If cash_ammount argument is missing then the current value "
                 "of stocks will be used. If the cash_ammount argument starts "
                 "with '+' then that ammount will be added to current value of "
                 "stocks, this method can be used if you want to invest some "
                 "money and you want to replicate a specific index."
              << std::endl;
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
        return CmdPrintPortfolio(cfg);
    } else if (strcmp(argv[1], "--ptva") == 0) {
        return CmdPrintActivity(cfg);
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
        uint64_t ammount       = 0;
        bool addPortfolioValue = false;

        if (argc == 2) {
            addPortfolioValue = true;
        } else if (argc > 2) {
            if (argv[2][0] == '+') {
                ammount           = std::stoull(argv[2] + 1);
                addPortfolioValue = true;
            } else {
                ammount = std::stoull(argv[2]);
            }
        }

        return CmdPrintIndexReplication(cfg, ammount, addPortfolioValue);
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