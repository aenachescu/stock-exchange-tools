#ifndef STOCK_EXCHANGE_TOOLS_STRING_UTILS_H
#define STOCK_EXCHANGE_TOOLS_STRING_UTILS_H

#include <string>
#include <vector>

std::string double_to_string(
    double d,
    size_t precision   = 2,
    bool useSeparators = false);

std::string u64_to_string(uint64_t val);

std::vector<std::string> split_string(const std::string& str, char delim);

#endif // STOCK_EXCHANGE_TOOLS_STRING_UTILS_H
