#include "cli_utils.h"
#include "config.h"
#include "string_utils.h"
#include "tradeville.h"

#include <chrono>
#include <iostream>
#include <magic_enum.hpp>

uint64_t GetCurrentYear()
{
    using namespace std::chrono;

    int year = static_cast<int>(
        year_month_day{time_point_cast<days>(system_clock::now())}.year());

    return static_cast<uint64_t>(year);
}

std::string quantity_to_string(const Quantity& q)
{
    if (std::holds_alternative<uint64_t>(q) == true) {
        return std::to_string(std::get<uint64_t>(q));
    }

    return double_to_string(std::get<double>(q));
};

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
    uint64_t endYear   = GetCurrentYear();

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

void CmdPrintHelp()
{
    std::cout << "Supported commands:" << std::endl;
    std::cout << "--ptvp - prints the portfolio from tradeville" << std::endl;
    std::cout << "--ptva - prints the activity from tradeville" << std::endl;
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
    } else if (strcmp(argv[1], "--help") == 0) {
        CmdPrintHelp();
        return 0;
    }

    std::cout << "unknown command" << std::endl;
    CmdPrintHelp();

    return -1;
}