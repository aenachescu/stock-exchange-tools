#include "bvb_scraper.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <magic_enum.hpp>
#include <sstream>

using Table = std::vector<std::vector<std::string>>;

std::string double_to_string(double d, size_t precision = 2)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << d;
    return stream.str();
}

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
        "YTD (%)"});

    for (const auto& i : r.value()) {
        table.emplace_back(std::vector<std::string>{
            std::to_string(id),
            i.name,
            double_to_string(i.today),
            double_to_string(i.one_week),
            double_to_string(i.one_month),
            double_to_string(i.six_months),
            double_to_string(i.one_year),
            double_to_string(i.year_to_date)});
        id++;
    }

    print_table(table);

    return 0;
}

int cmd_print_index_constituents(const IndexName& name)
{
    std::cout << "processing " << name << std::endl;
    return 0;
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
    }

    std::cout << "unknown command" << std::endl;
    return -1;
}
