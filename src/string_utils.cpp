#include "string_utils.h"

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
