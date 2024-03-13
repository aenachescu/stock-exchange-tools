#ifndef STOCK_EXCHANGE_TOOLS_TRADEVILLE_H
#define STOCK_EXCHANGE_TOOLS_TRADEVILLE_H

#include "asset_type.h"
#include "currency.h"
#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"
#include "websocket_connection.h"

#include <expected.hpp>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// rapidjson
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

struct Portfolio
{
    struct Entry
    {
        std::string account;
        std::string symbol;
        std::variant<uint64_t, double> quantity = 0ull;
        double avg_price                        = 0.0;
        double market_price                     = 0.0;
        Currency currency;
        AssetType asset;
    };

    std::vector<Entry> entries;
};

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
    bool VerifyArrays(
        const rapidjson::Value& doc,
        const std::vector<std::string_view>& arrays);

private:
    std::string m_username;
    std::string m_password;
    WebsocketConnection m_wsConn;
};

#endif // STOCK_EXCHANGE_TOOLS_TRADEVILLE_H
