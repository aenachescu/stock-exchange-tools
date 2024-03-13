#include "cli_utils.h"

#include <iomanip>
#include <iostream>

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
