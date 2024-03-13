#ifndef STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_CONFIG_H
#define STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_CONFIG_H

#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"

#include <functional>
#include <map>
#include <optional>
#include <string>

class Config : private noncopyable, private nonmovable {
private:
    static constexpr const char* kFileName = "index_investing_tool.conf";

    using Setter = std::function<void(Config&, const std::string&)>;

public:
    Config();
    ~Config() = default;

    Error LoadConfig();

    const std::optional<std::string>& GetBroker() const
    {
        return m_broker;
    }

    const std::optional<std::string>& GetTradevilleUser() const
    {
        return m_tradevilleUser;
    }

    const std::optional<std::string>& GetTradevillePass() const
    {
        return m_tradevillePass;
    }

    const std::optional<std::string>& GetTradevilleStartYear() const
    {
        return m_tradevilleStartYear;
    }

    const std::optional<std::string>& GetStockExchange() const
    {
        return m_stockExchange;
    }

    const std::optional<std::string>& GetIndexName() const
    {
        return m_indexName;
    }

    const std::optional<std::string>& GetIndexAdjustmentDate() const
    {
        return m_indexAdjustmentDate;
    }

    const std::optional<std::string>& GetIndexAdjustmentReason() const
    {
        return m_indexAdjustmentReason;
    }

private:
    std::optional<std::string> m_broker;
    std::optional<std::string> m_tradevilleUser;
    std::optional<std::string> m_tradevillePass;
    std::optional<std::string> m_tradevilleStartYear;
    std::optional<std::string> m_stockExchange;
    std::optional<std::string> m_indexName;
    std::optional<std::string> m_indexAdjustmentDate;
    std::optional<std::string> m_indexAdjustmentReason;

    std::map<std::string, Setter> m_setters;
};

#endif // STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_CONFIG_H
