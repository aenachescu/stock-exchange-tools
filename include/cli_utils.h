#ifndef STOCK_EXCHANGE_TOOLS_CLI_UTILS_H
#define STOCK_EXCHANGE_TOOLS_CLI_UTILS_H

#include <string>
#include <vector>

enum class Color
{
    None,
    Reset,
    Red,
    Green,
};

struct ColorizedString
{
    std::string str;
    Color color = Color::None;

    ColorizedString()  = default;
    ~ColorizedString() = default;

    ColorizedString(const char* s) : str(s)
    {
    }

    ColorizedString(std::string&& s) : str(std::move(s))
    {
    }

    ColorizedString(const std::string& s) : str(s)
    {
    }

    ColorizedString(const char* s, Color c) : str(s), color(c)
    {
    }

    ColorizedString(std::string&& s, Color c) : str(std::move(s)), color(c)
    {
    }

    ColorizedString(const std::string& s, Color c) : str(s), color(c)
    {
    }
};

using Table          = std::vector<std::vector<std::string>>;
using ColorizedTable = std::vector<std::vector<ColorizedString>>;

void print_table(const Table& table);

void print_table(const ColorizedTable& table);

#endif // STOCK_EXCHANGE_TOOLS_CLI_UTILS_H
