#include "chrono_utils.h"

std::chrono::year_month_day ymd_today()
{
    return std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(
        std::chrono::system_clock::now())};
}

std::chrono::year_month_day ymd_tomorrow()
{
    auto days =
        std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    ++days;
    return std::chrono::year_month_day{days};
}

uint64_t get_current_year()
{
    int year = static_cast<int>(ymd_today().year());
    return static_cast<uint64_t>(year);
}
