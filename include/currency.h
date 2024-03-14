#ifndef STOCK_EXCHANGE_TOOLS_CURRENCY_H
#define STOCK_EXCHANGE_TOOLS_CURRENCY_H

#include <map>

enum class Currency
{
    Unknown,
    Ron,
    Usd,
    Eur,
};

using ExchangeRates = std::map<Currency, double>;

#endif // STOCK_EXCHANGE_TOOLS_CURRENCY_H
