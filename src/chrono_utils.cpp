#include "chrono_utils.h"

#include <chrono>

uint64_t get_current_year()
{
    using namespace std::chrono;

    int year = static_cast<int>(
        year_month_day{time_point_cast<days>(system_clock::now())}.year());

    return static_cast<uint64_t>(year);
}
