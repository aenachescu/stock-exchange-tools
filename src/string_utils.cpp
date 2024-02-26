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
