#include "bvb_scraper.h"
#include "string_utils.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <magic_enum.hpp>
#include <set>
#include <sstream>

using Table = std::vector<std::vector<std::string>>;

void print_table(const Table& table)
{
    std::vector<size_t> maxSizes(table[0].size(), 0);
    std::string lineSeparator = "+";
    bool header               = true;

    for (size_t col = 0; col < table[0].size(); col++) {
        for (size_t line = 0; line < table.size(); line++) {
            if (table[line][col].size() > maxSizes[col]) {
                maxSizes[col] = table[line][col].size();
            }
        }
    }

    for (auto s : maxSizes) {
        lineSeparator += std::string(s + 2, '-') + "+";
    }

    std::cout << lineSeparator << std::endl;
    for (const auto& line : table) {
        for (size_t col = 0; col < line.size(); col++) {
            std::cout << "| " << std::left << std::setw(maxSizes[col])
                      << line[col] << ' ';
        }
        std::cout << '|' << std::endl;

        if (header == true) {
            std::cout << lineSeparator << std::endl;
            header = false;
        }
    }
    std::cout << lineSeparator << std::endl;
}

int cmd_print_indexes()
{
    BvbScraper bvbScraper;
    Table table;
    size_t id = 1;

    auto r = bvbScraper.GetIndexesNames();
    if (! r) {
        std::cout << "failed to get indexes: "
                  << magic_enum::enum_name(r.error()) << std::endl;
        return -1;
    }

    table.reserve(r.value().size() + 1);
    table.emplace_back(std::vector<std::string>{"#", "Index"});

    for (const auto& i : r.value()) {
        table.emplace_back(std::vector<std::string>{std::to_string(id), i});
        id++;
    }

    print_table(table);

    return 0;
}

int cmd_print_indexes_performance()
{
    BvbScraper bvbScraper;
    Table table;
    size_t id = 1;

    auto r = bvbScraper.GetIndexesPerformance();
    if (! r) {
        std::cout << "failed to get indexes performance: "
                  << magic_enum::enum_name(r.error()) << std::endl;
        return -1;
    }

    table.reserve(r.value().size() + 1);
    table.emplace_back(std::vector<std::string>{
        "#",
        "Index",
        "today (%)",
        "1 week (%)",
        "1 month (%)",
        "6 months (%)",
        "1 year (%)",
        "YTD (%)",
    });

    for (const auto& i : r.value()) {
        table.emplace_back(std::vector<std::string>{
            std::to_string(id),
            i.name,
            double_to_string(i.today),
            double_to_string(i.one_week),
            double_to_string(i.one_month),
            double_to_string(i.six_months),
            double_to_string(i.one_year),
            double_to_string(i.year_to_date),
        });
        id++;
    }

    print_table(table);

    return 0;
}

int cmd_print_index_constituents(const IndexName& indexName)
{
    BvbScraper bvbScraper;
    Table table;
    size_t id = 1;
    IndexesNames names;

    if (indexName == "--all") {
        auto r = bvbScraper.GetIndexesNames();
        if (! r) {
            std::cout << "failed to get indexes names: "
                      << magic_enum::enum_name(r.error()) << std::endl;
            return -1;
        }

        names = r.value();
    } else {
        names.push_back(indexName);
    }

    for (const auto& name : names) {
        auto r = bvbScraper.GetConstituents(name);
        if (! r) {
            std::cout << "failed to get " << name
                      << " constituents: " << magic_enum::enum_name(r.error())
                      << std::endl;
            return -1;
        }

        std::sort(
            r.value().companies.begin(),
            r.value().companies.end(),
            [](Company a, Company b) { return a.weight > b.weight; });

        id = 1;
        table.clear();
        table.reserve(r.value().companies.size() + 1);
        table.emplace_back(std::vector<std::string>{
            "#",
            "Symbol",
            "Company",
            "Shares",
            "Price",
            "FF",
            "FR",
            "FC",
            "FL",
            "Weight (%)",
        });

        for (const auto& i : r.value().companies) {
            table.emplace_back(std::vector<std::string>{
                std::to_string(id),
                i.symbol,
                i.name,
                u64_to_string(i.shares),
                double_to_string(i.reference_price, 4),
                double_to_string(i.free_float_factor),
                double_to_string(i.representation_factor, 6),
                double_to_string(i.price_correction_factor, 6),
                double_to_string(i.liquidity_factor),
                double_to_string(i.weight),
            });
            id++;
        }

        std::cout << "Index name: " << r.value().name << std::endl;
        std::cout << "Date: " << r.value().date << std::endl;
        std::cout << "Reason: " << r.value().reason << std::endl;
        print_table(table);
        std::cout << std::endl;
    }

    return 0;
}

int cmd_print_adjustments_history(const IndexName& indexName, size_t count)
{
    BvbScraper bvbScraper;
    Table table;
    size_t id = 1;
    IndexesNames names;

    if (indexName == "--all") {
        auto r = bvbScraper.GetIndexesNames();
        if (! r) {
            std::cout << "failed to get indexes names: "
                      << magic_enum::enum_name(r.error()) << std::endl;
            return -1;
        }

        names = r.value();
    } else {
        names.push_back(indexName);
    }

    for (const auto& name : names) {
        auto r = bvbScraper.GetAdjustmentsHistory(name);
        if (! r) {
            std::cout << "failed to get " << name << " adjustments history: "
                      << magic_enum::enum_name(r.error()) << std::endl;
            return -1;
        }

        for (size_t i = 0; i < r.value().size() && i < count; i++) {
            Index& index = r.value()[i];
            std::sort(
                index.companies.begin(),
                index.companies.end(),
                [](Company a, Company b) { return a.weight > b.weight; });

            id = 1;
            table.clear();
            table.reserve(index.companies.size() + 1);
            table.emplace_back(std::vector<std::string>{
                "#",
                "Symbol",
                "Company",
                "Shares",
                "Price",
                "FF",
                "FR",
                "FC",
                "FL",
                "Weight (%)",
            });

            for (const auto& i : index.companies) {
                table.emplace_back(std::vector<std::string>{
                    std::to_string(id),
                    i.symbol,
                    i.name,
                    u64_to_string(i.shares),
                    double_to_string(i.reference_price, 4),
                    double_to_string(i.free_float_factor),
                    double_to_string(i.representation_factor, 6),
                    double_to_string(i.price_correction_factor, 6),
                    double_to_string(i.liquidity_factor),
                    double_to_string(i.weight),
                });
                id++;
            }

            std::cout << "Index name: " << index.name << std::endl;
            std::cout << "Date: " << index.date << std::endl;
            std::cout << "Reason: " << index.reason << std::endl;
            print_table(table);
        }
        std::cout << std::endl;
    }

    return 0;
}

int cmd_print_trading_data(const IndexName& indexName)
{
    BvbScraper bvbScraper;
    Table table;
    size_t id = 1;
    IndexesNames names;

    if (indexName == "--all") {
        auto r = bvbScraper.GetIndexesNames();
        if (! r) {
            std::cout << "failed to get indexes names: "
                      << magic_enum::enum_name(r.error()) << std::endl;
            return -1;
        }

        names = r.value();
    } else {
        names.push_back(indexName);
    }

    for (const auto& name : names) {
        auto r = bvbScraper.GetTradingData(name);
        if (! r) {
            std::cout << "failed to get " << name
                      << " trading data: " << magic_enum::enum_name(r.error())
                      << std::endl;
            return -1;
        }

        std::sort(
            r.value().companies.begin(),
            r.value().companies.end(),
            [](CompanyTradingData a, CompanyTradingData b) {
                return a.weight > b.weight;
            });

        id = 1;
        table.clear();
        table.reserve(r.value().companies.size() + 1);
        table.emplace_back(std::vector<std::string>{
            "#",
            "Symbol",
            "Price",
            "Variation (%)",
            "Trades",
            "Volume",
            "Value",
            "Low",
            "High",
            "Weight (%)",
        });

        for (const auto& i : r.value().companies) {
            table.emplace_back(std::vector<std::string>{
                std::to_string(id),
                i.symbol,
                double_to_string(i.price, 4),
                double_to_string(i.variation),
                u64_to_string(i.trades),
                u64_to_string(i.volume),
                double_to_string(i.value, 2, true),
                double_to_string(i.lowest_price, 4),
                double_to_string(i.highest_price, 4),
                double_to_string(i.weight),
            });
            id++;
        }

        std::cout << "Index name: " << r.value().name << std::endl;
        std::cout << "Date: " << r.value().date << std::endl;
        print_table(table);
        std::cout << std::endl;
    }

    return 0;
}

int cmd_save_adjustments_history(const IndexName& indexName)
{
    BvbScraper bvbScraper;
    IndexesNames names;

    if (indexName == "--all") {
        auto r = bvbScraper.GetIndexesNames();
        if (! r) {
            std::cout << "failed to get indexes names: "
                      << magic_enum::enum_name(r.error()) << std::endl;
            return -1;
        }

        names = r.value();
    } else {
        names.push_back(indexName);
    }

    for (const auto& name : names) {
        auto r = bvbScraper.GetAdjustmentsHistory(name);
        if (! r) {
            std::cout << "failed to get " << name << " adjustments history: "
                      << magic_enum::enum_name(r.error()) << std::endl;
            continue;
        }

        Error err = bvbScraper.SaveAdjustmentsHistoryToFile(name, r.value());
        if (err != Error::NoError) {
            std::cout << "failed to save " << name
                      << " adjustments history: " << magic_enum::enum_name(err)
                      << std::endl;
            continue;
        }

        std::cout << "saved " << name << " adjustments history" << std::endl;
    }

    return 0;
}

int cmd_load_adjustments_history(const IndexName& indexName)
{
    BvbScraper bvbScraper;
    Table table;
    size_t id = 1;

    auto r = bvbScraper.LoadAdjustmentsHistoryFromFile(indexName);
    if (! r) {
        std::cout << "failed to load " << indexName << " adjustments history: "
                  << magic_enum::enum_name(r.error()) << std::endl;
        return -1;
    }

    for (size_t i = 0; i < r.value().size(); i++) {
        Index& index = r.value()[i];
        std::sort(
            index.companies.begin(),
            index.companies.end(),
            [](Company a, Company b) { return a.weight > b.weight; });

        id = 1;
        table.clear();
        table.reserve(index.companies.size() + 1);
        table.emplace_back(std::vector<std::string>{
            "#",
            "Symbol",
            "Company",
            "Shares",
            "Price",
            "FF",
            "FR",
            "FC",
            "FL",
            "Weight (%)",
        });

        for (const auto& i : index.companies) {
            table.emplace_back(std::vector<std::string>{
                std::to_string(id),
                i.symbol,
                i.name,
                u64_to_string(i.shares),
                double_to_string(i.reference_price, 4),
                double_to_string(i.free_float_factor),
                double_to_string(i.representation_factor, 6),
                double_to_string(i.price_correction_factor, 6),
                double_to_string(i.liquidity_factor),
                double_to_string(i.weight),
            });
            id++;
        }

        std::cout << "Index name: " << index.name << std::endl;
        std::cout << "Date: " << index.date << std::endl;
        std::cout << "Reason: " << index.reason << std::endl;
        print_table(table);
    }
    std::cout << std::endl;

    return 0;
}

int cmd_update_adjustments_history(const IndexName& indexName)
{
    BvbScraper bvbScraper;
    IndexesNames names;
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day   = 0;

    if (indexName == "--all") {
        auto r = bvbScraper.GetIndexesNames();
        if (! r) {
            std::cout << "failed to get indexes names: "
                      << magic_enum::enum_name(r.error()) << std::endl;
            return -1;
        }

        names = r.value();
    } else {
        names.push_back(indexName);
    }

    for (const auto& name : names) {
        std::set<ComparableIndex, IndexComparator> mergedHistory;
        Indexes indexes;

        auto fileHistory = bvbScraper.LoadAdjustmentsHistoryFromFile(name);
        if (! fileHistory) {
            std::cout << "failed to load " << name << " adjustments history: "
                      << magic_enum::enum_name(fileHistory.error())
                      << std::endl;
            continue;
        }

        auto siteHistory = bvbScraper.GetAdjustmentsHistory(name);
        if (! siteHistory) {
            std::cout << "failed to get " << name << " adjustments history: "
                      << magic_enum::enum_name(siteHistory.error())
                      << std::endl;
            continue;
        }

        for (const auto& entry : fileHistory.value()) {
            if (! parse_mdy_date(entry.date, month, day, year)) {
                std::cout << "failed to parse index date [" << entry.date << "]"
                          << std::endl;
                return -1;
            }
            mergedHistory.emplace(ComparableIndex(entry, year, month, day));
        }

        for (const auto& entry : siteHistory.value()) {
            if (! parse_mdy_date(entry.date, month, day, year)) {
                std::cout << "failed to parse index date [" << entry.date << "]"
                          << std::endl;
                return -1;
            }
            mergedHistory.emplace(ComparableIndex(entry, year, month, day));
        }

        std::transform(
            mergedHistory.rbegin(),
            mergedHistory.rend(),
            std::back_inserter(indexes),
            [](const ComparableIndex& ci) { return ci.index; });

        Error err = bvbScraper.SaveAdjustmentsHistoryToFile(name, indexes);
        if (err != Error::NoError) {
            std::cout << "failed to save " << name
                      << " adjustments history: " << magic_enum::enum_name(err)
                      << std::endl;
            continue;
        }

        std::cout << "updated " << name << " adjustments history" << std::endl;
    }

    return 0;
}

void cmd_print_help()
{
    std::cout << "Supported commands:" << std::endl;
    std::cout << "--pi - prints names of BVB indices" << std::endl;
    std::cout << "--pip - prints performance of BVB indices" << std::endl;
    std::cout << "--pic <index_name> - prints constituents of a BVB index. Use "
                 "--all for index name in order to print constituents for all "
                 "BVB indices."
              << std::endl;
    std::cout << "--pah <index_name> <n> - prints adjustments history of a BVB "
                 "index. Use --all for index name in order to print "
                 "adjustments history for all BVB indices. N can be used to "
                 "limit how many adjustments to be printed, by default all "
                 "adjustments are printed."
              << std::endl;
    std::cout << "--ptd <index_name> - prints trading data for a BVB index. "
                 "Use --all for index name in order to print trading data for "
                 "all BVB indices."
              << std::endl;
    std::cout << "--sah <index_name> - saves adjustments history for a BVB "
                 "index. Use --all for index name in order to save adjustments "
                 "history for all BVB indices."
              << std::endl;
    std::cout << "--lah <index_name> - loads adjustments history from file for "
                 "a BVB index and prints them."
              << std::endl;
    std::cout << "--uah <index_name> - updates adjustments history for a BVB "
                 "index by merging the adjustments history from file with the "
                 "adjustments history from BVB site. Use --all for index name "
                 "in order to update adjustments history for all BVB indices."
              << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "no command" << std::endl;
        return -1;
    }

    if (strcmp(argv[1], "--pi") == 0) {
        return cmd_print_indexes();
    } else if (strcmp(argv[1], "--pip") == 0) {
        return cmd_print_indexes_performance();
    } else if (strcmp(argv[1], "--pic") == 0) {
        if (argc < 3) {
            std::cout << "no index name" << std::endl;
            return -1;
        }
        return cmd_print_index_constituents(argv[2]);
    } else if (strcmp(argv[1], "--pah") == 0) {
        size_t count = std::numeric_limits<size_t>::max();

        if (argc < 3) {
            std::cout << "no index name" << std::endl;
            return -1;
        }

        if (argc == 4) {
            std::istringstream iss(argv[3]);
            iss >> count;
        }

        return cmd_print_adjustments_history(argv[2], count);
    } else if (strcmp(argv[1], "--ptd") == 0) {
        if (argc < 3) {
            std::cout << "no index name" << std::endl;
            return -1;
        }
        return cmd_print_trading_data(argv[2]);
    } else if (strcmp(argv[1], "--sah") == 0) {
        if (argc < 3) {
            std::cout << "no index name" << std::endl;
            return -1;
        }

        return cmd_save_adjustments_history(argv[2]);
    } else if (strcmp(argv[1], "--lah") == 0) {
        if (argc < 3) {
            std::cout << "no index name" << std::endl;
            return -1;
        }

        return cmd_load_adjustments_history(argv[2]);
    } else if (strcmp(argv[1], "--uah") == 0) {
        if (argc < 3) {
            std::cout << "no index name" << std::endl;
            return -1;
        }

        return cmd_update_adjustments_history(argv[2]);
    } else if (strcmp(argv[1], "--help") == 0) {
        cmd_print_help();
        return 0;
    }

    std::cout << "unknown command" << std::endl;
    cmd_print_help();
    return -1;
}
