#ifndef STOCK_EXCHANGE_TOOLS_CURRENCY_H
#define STOCK_EXCHANGE_TOOLS_CURRENCY_H

#include <cstdint>
#include <map>

enum class Currency : std::uint8_t
{
    Unknown,
    Usd,
    Eur,
    Ron,
};

using ExchangeRates = std::map<Currency, double>;

#endif // STOCK_EXCHANGE_TOOLS_CURRENCY_H
