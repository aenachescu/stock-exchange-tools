#ifndef STOCK_EXCHANGE_TOOLS_TRADEVILLE_H
#define STOCK_EXCHANGE_TOOLS_TRADEVILLE_H

#include "activity_type.h"
#include "asset_type.h"
#include "currency.h"
#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"
#include "websocket_connection.h"

#include <expected.hpp>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// rapidjson
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

using AssetValue            = std::map<AssetType, double>;
using CurrencyValue         = std::map<Currency, double>;
using AssetAndCurrencyValue = std::map<AssetType, CurrencyValue>;
using Quantity              = std::variant<uint64_t, double>;

struct Portfolio
{
    struct Entry
    {
        std::string account;
        std::string symbol;
        Quantity quantity   = 0ull;
        double avg_price    = 0.0;
        double market_price = 0.0;
        Currency currency   = Currency::Unknown;
        AssetType asset     = AssetType::Unknown;
    };

    tl::expected<AssetValue, Error> GetValueByAsset(
        Currency currency,
        const ExchangeRates& rates) const;
    CurrencyValue GetValueByCurrency() const;
    AssetAndCurrencyValue GetValueByAssetAndCurrency() const;

    std::vector<Entry> entries;
};

struct Activity
{
    std::string date;
    std::string symbol;
    std::string note;
    std::string market;
    std::string transaction_id;
    ActivityType type       = ActivityType::Unknown;
    Currency currency       = Currency::Unknown;
    Quantity quantity       = 0ull;
    uint64_t asset_position = 0;
    uint64_t order_id       = 0;
    double price            = 0.0;
    double avg_price        = 0.0;
    double commission       = 0.0;
    double tax              = 0.0;
    double cash_ammount     = 0.0;
    double cash_position    = 0.0;
    double profit           = 0.0;
};

using Activities = std::vector<Activity>;

class Tradeville : private noncopyable, private nonmovable {
private:
    static constexpr const char* kHost   = "api.tradeville.ro";
    static constexpr const char* kTarget = "/";
    static constexpr const char* kProto  = "apitv";
    static constexpr uint16_t kPort      = 443;

public:
    Tradeville(const std::string& user, const std::string& pass)
        : m_username(user), m_password(pass), m_wsConn(kHost, kPort)
    {
    }

    tl::expected<Portfolio, Error> GetPortfolio();
    tl::expected<Activities, Error> GetActivity(
        std::optional<std::string> symbol,
        uint64_t startYear,
        uint64_t endYear);

private:
    Error InitConnection();

    std::string GetLoginRequest();
    bool VerifyLoginResponse(const std::string& data);

    tl::expected<rapidjson::Document, Error> ValidatePortfolioJson(
        const std::string& data);
    tl::expected<Portfolio, Error> ParsePortfolio(
        const rapidjson::Document& doc);
    Error ParsePortfolioAccount(
        const rapidjson::Value& doc,
        Portfolio& portfolio);
    Error ParsePortfolioSymbol(
        const rapidjson::Value& doc,
        Portfolio& portfolio);
    Error ParsePortfolioQuantity(
        const rapidjson::Value& doc,
        Portfolio& portfolio);
    Error ParsePortfolioAvgPrice(
        const rapidjson::Value& doc,
        Portfolio& portfolio);
    Error ParsePortfolioMarketPrice(
        const rapidjson::Value& doc,
        Portfolio& portfolio);
    Error ParsePortfolioAsset(
        const rapidjson::Value& doc,
        Portfolio& portfolio);
    Error ParsePortfolioCurrency(
        const rapidjson::Value& doc,
        Portfolio& portfolio);

    std::string GetActivityRequest(
        const std::optional<std::string>& symbol,
        uint64_t startYear,
        uint64_t endYear);
    tl::expected<rapidjson::Document, Error> ValidateActivityJson(
        const std::string& data,
        const std::optional<std::string>& symbol,
        uint64_t startYear,
        uint64_t endYear);
    tl::expected<Activities, Error> ParseActivity(
        const rapidjson::Document& doc);
    Error ParseActivityDate(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityType(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivitySymbol(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityQuantity(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityPrice(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityCommission(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityCashAmmount(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityCashPosition(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityAssetPosition(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityProfit(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityTransactionId(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityCurrency(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityNote(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityAvgPrice(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityOrderId(
        const rapidjson::Value& doc,
        Activities& activities);
    Error ParseActivityTax(const rapidjson::Value& doc, Activities& activities);
    Error ParseActivityMarket(
        const rapidjson::Value& doc,
        Activities& activities);

    bool VerifyStrField(
        const rapidjson::Value& doc,
        const std::string& name,
        const std::string& value);
    bool VerifyIntField(
        const rapidjson::Value& doc,
        const std::string& name,
        int value);
    bool VerifyBoolField(
        const rapidjson::Value& doc,
        const std::string& name,
        bool value);
    bool VerifyNullField(const rapidjson::Value& doc, const std::string& name);
    bool VerifyArrays(
        const rapidjson::Value& doc,
        const std::vector<std::string_view>& arrays);

private:
    std::string m_username;
    std::string m_password;
    WebsocketConnection m_wsConn;
};

#endif // STOCK_EXCHANGE_TOOLS_TRADEVILLE_H
