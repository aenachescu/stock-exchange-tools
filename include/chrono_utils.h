#ifndef STOCK_EXCHANGE_TOOLS_CHRONO_UTILS_H
#define STOCK_EXCHANGE_TOOLS_CHRONO_UTILS_H

#include <chrono>
#include <cstdint>

std::chrono::year_month_day ymd_today();

std::chrono::year_month_day ymd_tomorrow();

uint64_t get_current_year();

#endif // STOCK_EXCHANGE_TOOLS_CHRONO_UTILS_H
