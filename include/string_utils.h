#ifndef STOCK_EXCHANGE_TOOLS_STRING_UTILS_H
#define STOCK_EXCHANGE_TOOLS_STRING_UTILS_H

#include <string>

std::string double_to_string(
    double d,
    size_t precision   = 2,
    bool useSeparators = false);

std::string u64_to_string(uint64_t val);

#endif // STOCK_EXCHANGE_TOOLS_STRING_UTILS_H
