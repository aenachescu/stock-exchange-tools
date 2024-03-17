#include "cli_utils.h"

#include <iomanip>
#include <iostream>

const char* get_color_str(Color color)
{
    switch (color) {
    case Color::Reset:
        return "\033[0m";
    case Color::Red:
        return "\033[31m";
    case Color::Green:
        return "\033[32m";
    case Color::None:
    default:
        return "";
    }

    return "";
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

void print_table(const ColorizedTable& table)
{
    std::vector<size_t> maxSizes(table[0].size(), 0);
    std::string lineSeparator = "+";
    bool header               = true;

    for (size_t col = 0; col < table[0].size(); col++) {
        for (size_t line = 0; line < table.size(); line++) {
            if (table[line][col].str.size() > maxSizes[col]) {
                maxSizes[col] = table[line][col].str.size();
            }
        }
    }

    for (auto s : maxSizes) {
        lineSeparator += std::string(s + 2, '-') + "+";
    }

    std::cout << lineSeparator << std::endl;
    for (const auto& line : table) {
        for (size_t col = 0; col < line.size(); col++) {
            std::cout << "| " << get_color_str(line[col].color) << std::left
                      << std::setw(maxSizes[col]) << line[col].str
                      << get_color_str(Color::Reset) << ' ';
        }
        std::cout << '|' << std::endl;

        if (header == true) {
            std::cout << lineSeparator << std::endl;
            header = false;
        }
    }
    std::cout << lineSeparator << std::endl;
}
