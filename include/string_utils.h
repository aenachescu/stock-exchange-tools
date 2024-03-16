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

std::vector<std::string> split_string(
    const std::string& str,
    const std::string& delim);

bool parse_mdy_date(
    const std::string& str,
    uint8_t& month,
    uint8_t& day,
    uint16_t& year);

bool get_year_from_ymd(const std::string& str, uint64_t& year);

bool is_number(const std::string& str);

bool string_contains_ci(const std::string& str1, const std::string& str2);

#endif // STOCK_EXCHANGE_TOOLS_STRING_UTILS_H
