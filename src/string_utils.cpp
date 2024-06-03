#include "string_utils.h"

#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>

struct ThousandsSeparator : std::numpunct<char>
{
    char do_thousands_sep() const
    {
        return ',';
    }

    std::string do_grouping() const
    {
        return "\3";
    }
};

template <typename CharType>
struct CharCmp
{
    CharCmp(const std::locale& loc) : m_locale(loc)
    {
    }

    bool operator()(CharType c1, CharType c2)
    {
        return std::toupper(c1, m_locale) == std::toupper(c2, m_locale);
    }

private:
    const std::locale& m_locale;
};

std::string double_to_string(double d, size_t precision, bool useSeparators)
{
    static std::stringstream oss;
    static bool initialized = false;

    if (useSeparators == true) {
        if (initialized == false) {
            oss.imbue(std::locale(std::locale(), new ThousandsSeparator));
            initialized = true;
        }

        oss << std::fixed << std::setprecision(precision) << d;
        std::string res = oss.str();
        oss.str("");

        return res;
    }

    std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << d;
    return stream.str();
}

std::string u64_to_string(uint64_t val)
{
    static std::stringstream oss;
    static bool initialized = false;

    if (initialized == false) {
        oss.imbue(std::locale(std::locale(), new ThousandsSeparator));
        initialized = true;
    }

    oss << val;
    std::string res = oss.str();
    oss.str("");

    return res;
}

std::string quantity_to_string(const Quantity& q)
{
    if (std::holds_alternative<uint64_t>(q) == true) {
        return std::to_string(std::get<uint64_t>(q));
    }

    return double_to_string(std::get<double>(q));
};

std::string date_to_string(
    const std::chrono::year_month_day& date,
    char separator)
{
    std::string res;
    unsigned int day   = static_cast<unsigned int>(date.day());
    unsigned int month = static_cast<unsigned int>(date.month());
    int year           = static_cast<int>(date.year());

    if (month < 10) {
        res += '0';
    }
    res += std::to_string(month);
    res += separator;

    if (day < 10) {
        res += '0';
    }
    res += std::to_string(day);
    res += separator;

    res += std::to_string(year);

    return res;
}

std::vector<std::string> split_string(const std::string& str, char delim)
{
    std::vector<std::string> vec;
    std::string token;
    std::stringstream ss(str);

    while (std::getline(ss, token, delim)) {
        vec.push_back(token);
    }

    return vec;
}

std::vector<std::string> split_string(
    const std::string& str,
    const std::string& delim)
{
    std::vector<std::string> vec;
    size_t pos_start = 0, pos_end;

    while ((pos_end = str.find(delim, pos_start)) != std::string::npos) {
        if (pos_end > pos_start) {
            vec.push_back(str.substr(pos_start, pos_end - pos_start));
        }
        pos_start = pos_end + delim.length();
    }

    vec.push_back(str.substr(pos_start));

    return vec;
}

bool parse_mdy_date(
    const std::string& str,
    uint8_t& month,
    uint8_t& day,
    uint16_t& year)
{
    month = 0;
    day   = 0;
    year  = 0;

    auto validator = [](const std::string& s) -> bool {
        for (char c : s) {
            if (! std::isdigit(c)) {
                return false;
            }
        }
        return true;
    };

    auto setter = [](const std::string& s,
                     auto& value,
                     unsigned long min,
                     unsigned long max) -> bool {
        unsigned long tmp = std::stoul(s);
        if (tmp < min || tmp > max) {
            return false;
        }
        value = static_cast<std::remove_cvref_t<decltype(value)>>(tmp);
        return true;
    };

    std::vector<std::string> tokens = split_string(str, '/');
    if (tokens.size() != 3) {
        return false;
    }

    for (const auto& t : tokens) {
        if (! validator(t)) {
            return false;
        }
    }

    if (! setter(tokens[0], month, 1, 12) || ! setter(tokens[1], day, 1, 31) ||
        ! setter(tokens[2], year, 2000, 3000)) {
        return false;
    }

    return true;
}

bool get_year_from_ymd(const std::string& str, uint64_t& year)
{
    if (str.size() < 5) {
        return false;
    }

    if (str[4] != '-') {
        return false;
    }

    year = std::stoull(str.substr(0, 4));

    return true;
}

bool is_number(const std::string& str)
{
    if (str.empty()) {
        return false;
    }

    if (! std::isdigit(str[0]) || str[0] == '0') {
        return false;
    }

    for (size_t i = 1; i < str.size(); i++) {
        if (! std::isdigit(str[i])) {
            return false;
        }
    }

    return true;
}

bool string_contains_ci(const std::string& str1, const std::string& str2)
{
    std::string::const_iterator it = std::search(
        str1.begin(),
        str1.end(),
        str2.begin(),
        str2.end(),
        CharCmp<std::string::value_type>(std::locale()));

    if (it == str1.end()) {
        return false;
    }

    return true;
}
