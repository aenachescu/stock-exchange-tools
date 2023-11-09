#ifndef STOCK_EXCHANGE_TOOLS_CLOSED_INTERVAL_H
#define STOCK_EXCHANGE_TOOLS_CLOSED_INTERVAL_H

#include <cstddef>

class ClosedInterval {
public:
    ClosedInterval()  = default;
    ~ClosedInterval() = default;

    ClosedInterval(size_t lower, size_t upper) : m_lower(lower), m_upper(upper)
    {
    }

    bool Empty() const
    {
        return m_lower > m_upper;
    }

    size_t Size() const
    {
        if (Empty()) {
            return 0;
        }

        return m_upper - m_lower + 1;
    }

    void SetLower(size_t lower)
    {
        m_lower = lower;
    }

    size_t Lower() const
    {
        return m_lower;
    }

    void SetUpper(size_t upper)
    {
        m_upper = upper;
    }

    size_t Upper() const
    {
        return m_upper;
    }

private:
    size_t m_lower = 0;
    size_t m_upper = static_cast<size_t>(-1);
};

#endif // STOCK_EXCHANGE_TOOLS_CLOSED_INTERVAL_H
