#include "cli_utils.h"
#include "config.h"
#include "string_utils.h"
#include "tradeville.h"

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

void PrintPortfolio(const Portfolio& portfolio)
{
    Table table;
    size_t id = 1;

    auto quantity_to_string = [](const std::variant<uint64_t, double>& q) {
        if (std::holds_alternative<uint64_t>(q) == true) {
            return std::to_string(std::get<uint64_t>(q));
        }
        return double_to_string(std::get<double>(q));
    };

    table.reserve(portfolio.entries.size() + 1);
    table.emplace_back(std::vector<std::string>{
        "#",
        "Account",
        "Symbol",
        "Quantity",
        "Avg price",
        "Market price",
        "Currency",
        "Asset",
    });

    for (const auto& i : portfolio.entries) {
        table.emplace_back(std::vector<std::string>{
            std::to_string(id),
            i.account,
            i.symbol,
            quantity_to_string(i.quantity),
            double_to_string(i.avg_price, 4),
            double_to_string(i.market_price, 4),
            std::string{magic_enum::enum_name(i.currency)},
            std::string{magic_enum::enum_name(i.asset)},
        });
        id++;
    }

    print_table(table);
}

int main()
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

    Tradeville tv(*cfg.GetTradevilleUser(), *cfg.GetTradevillePass());
    auto portfolio = tv.GetPortfolio();
    if (! portfolio) {
        std::cout << "Failed to get portfolio: "
                  << magic_enum::enum_name(portfolio.error()) << std::endl;
        return -1;
    }

    PrintPortfolio(*portfolio);

    return 0;
}