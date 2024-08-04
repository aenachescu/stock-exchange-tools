#include "tradeville.h"

#include "string_utils.h"

#include <fstream>
#include <iomanip>
#include <magic_enum.hpp>

tl::expected<AssetValue, Error> Portfolio::GetValueByAsset(
    Currency currency,
    const ExchangeRates& rates) const
{
    AssetValue res;

    for (const auto& e : entries) {
        double val = e.value;

        if (e.currency != currency) {
            auto rateIt = rates.find(e.currency);
            if (rateIt == rates.end()) {
                return tl::unexpected(Error::InvalidArg);
            }

            val *= rateIt->second;
        }

        auto it = res.emplace(e.asset, val);
        if (it.second == false) {
            it.first->second += val;
        }
    }

    return res;
}

CurrencyValue Portfolio::GetValueByCurrency() const
{
    CurrencyValue res;

    for (const auto& e : entries) {
        auto it = res.emplace(e.currency, e.value);
        if (it.second == false) {
            it.first->second += e.value;
        }
    }

    return res;
}

AssetAndCurrencyValue Portfolio::GetValueByAssetAndCurrency() const
{
    AssetAndCurrencyValue res;

    for (const auto& e : entries) {
        auto assetIt    = res.emplace(e.asset, CurrencyValue{});
        auto currencyIt = assetIt.first->second.emplace(e.currency, e.value);
        if (currencyIt.second == false) {
            currencyIt.first->second += e.value;
        }
    }

    return res;
}

Error Portfolio::FillStatistics(const Activities& activities)
{
    FillDividends(activities);

    for (auto& entry : entries) {
        if (entry.asset == AssetType::Money) {
            entry.avg_price = 1.0;
        }

        double quantity = 0.0;
        if (std::holds_alternative<uint64_t>(entry.quantity) == true) {
            quantity = std::get<uint64_t>(entry.quantity);
        } else {
            quantity = std::get<double>(entry.quantity);
        }

        entry.cost         = entry.avg_price * quantity;
        entry.value        = entry.market_price * quantity;
        entry.profit_loss  = entry.value - entry.cost;
        entry.total_return = entry.profit_loss + entry.dividends;

        entry.profit_loss_percentage  = entry.profit_loss / entry.cost * 100.0;
        entry.total_return_percentage = entry.total_return / entry.cost * 100.0;
    }

    return Error::NoError;
}

void Portfolio::FillDividends(const Activities& activities)
{
    for (const auto& activity : activities) {
        if (activity.type != ActivityType::Dividend) {
            continue;
        }

        for (auto& entry : entries) {
            if (entry.symbol == activity.symbol) {
                entry.dividends += activity.cash_ammount;
                break;
            }
        }
    }
}

tl::expected<Portfolio, Error> Tradeville::GetPortfolio()
{
    static constexpr const char* kRequest =
        "{ \"cmd\": \"Portfolio\", \"prm\": { \"data\": \"null\" } } ";

    Error err = InitConnection();
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    auto rsp = m_wsConn.SendRequest(kRequest);
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    auto json = ValidatePortfolioJson(*rsp);
    if (! json) {
        return tl::unexpected(json.error());
    }

    return ParsePortfolio(*json);
}

tl::expected<Activities, Error> Tradeville::GetActivity(
    std::optional<std::string> symbol,
    uint64_t startYear,
    uint64_t endYear)
{
    Error err = InitConnection();
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    std::string req = GetActivityRequest(symbol, startYear, endYear);
    auto rsp        = m_wsConn.SendRequest(req);
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    auto json = ValidateActivityJson(*rsp, symbol, startYear, endYear);
    if (! json) {
        return tl::unexpected(json.error());
    }

    return ParseActivity(*json);
}

Error Tradeville::SavePortfolioToFile()
{
    static constexpr const char* kRequest =
        "{ \"cmd\": \"Portfolio\", \"prm\": { \"data\": \"null\" } } ";

    Error err = InitConnection();
    if (err != Error::NoError) {
        return err;
    }

    auto rsp = m_wsConn.SendRequest(kRequest);
    if (! rsp) {
        return rsp.error();
    }

    auto json = ValidatePortfolioJson(*rsp);
    if (! json) {
        return json.error();
    }

    std::ofstream file("tradeville_portfolio.txt");
    if (! file) {
        return Error::FileNotFound;
    }

    file << *rsp;
    file.close();

    return Error::NoError;
}

Error Tradeville::SaveActivityToFile(uint64_t year)
{
    Error err = InitConnection();
    if (err != Error::NoError) {
        return err;
    }

    std::string req = GetActivityRequest(std::nullopt, year, year);
    auto rsp        = m_wsConn.SendRequest(req);
    if (! rsp) {
        return rsp.error();
    }

    auto json = ValidateActivityJson(*rsp, std::nullopt, year, year);
    if (! json) {
        return json.error();
    }

    std::string fileName = "tradeville_activity_";
    fileName += std::to_string(year);
    fileName += ".txt";

    std::ofstream file(fileName);
    if (! file) {
        return Error::FileNotFound;
    }

    file << *rsp;
    file.close();

    return Error::NoError;
}

Error Tradeville::InitConnection()
{
    if (m_wsConn.IsConnected() == true) {
        return Error::NoError;
    }

    Error err = m_wsConn.Connect(kTarget, kProto);
    if (err != Error::NoError) {
        return err;
    }

    std::string loginReq = GetLoginRequest();
    auto rsp             = m_wsConn.SendRequest(loginReq);
    if (! rsp) {
        return rsp.error();
    }

    if (VerifyLoginResponse(*rsp) == false) {
        return Error::InvalidArg;
    }

    return Error::NoError;
}

std::string Tradeville::GetLoginRequest()
{
    rapidjson::StringBuffer strBuff;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuff);
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value prm(rapidjson::kObjectType);

    doc.SetObject();

    prm.AddMember(
        "coduser",
        rapidjson::Value{}.SetString(m_username.c_str(), allocator),
        allocator);
    prm.AddMember(
        "parola",
        rapidjson::Value{}.SetString(m_password.c_str(), allocator),
        allocator);
    prm.AddMember("demo", false, allocator);

    doc.AddMember("cmd", "login", allocator);
    doc.AddMember("prm", prm, allocator);

    doc.Accept(writer);

    return std::string(strBuff.GetString(), strBuff.GetSize());
}

bool Tradeville::VerifyLoginResponse(const std::string& data)
{
    rapidjson::Document doc;
    doc.Parse(data.c_str(), data.size());

    if (doc.IsObject() == false) {
        return false;
    }

    if (VerifyStrField(doc, "cmd", "login") == false) {
        return false;
    }

    if (VerifyIntField(doc, "OK", 1) == false) {
        return false;
    }

    rapidjson::Value::ConstMemberIterator prmIt = doc.FindMember("prm");
    if (prmIt == doc.MemberEnd()) {
        return false;
    }

    if (prmIt->value.IsObject() == false) {
        return false;
    }

    if (VerifyStrField(prmIt->value, "coduser", m_username) == false) {
        return false;
    }

    if (VerifyStrField(prmIt->value, "parola", m_password) == false) {
        return false;
    }

    if (VerifyBoolField(prmIt->value, "demo", false) == false) {
        return false;
    }

    return true;
}

tl::expected<rapidjson::Document, Error> Tradeville::ValidatePortfolioJson(
    const std::string& data)
{
    static std::vector<std::string_view> kDataArrays = {
        "Account",
        "Symbol",
        "Quantity",
        "AvgPrice",
        "MarketPrice",
        "PType",
        "Ccy",
    };

    rapidjson::Document doc;
    doc.Parse(data.c_str(), data.size());

    if (doc.IsObject() == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (VerifyStrField(doc, "cmd", "Portfolio") == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    rapidjson::Value::ConstMemberIterator prmIt = doc.FindMember("prm");
    if (prmIt == doc.MemberEnd()) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (prmIt->value.IsObject() == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (VerifyStrField(prmIt->value, "data", "null") == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    rapidjson::Value::ConstMemberIterator dataIt = doc.FindMember("data");
    if (dataIt == doc.MemberEnd()) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (dataIt->value.IsObject() == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (! VerifyArrays(dataIt->value, kDataArrays)) {
        return tl::unexpected(Error::UnexpectedData);
    }

    return doc;
}

tl::expected<Portfolio, Error> Tradeville::ParsePortfolio(
    const rapidjson::Document& doc)
{
    Portfolio portfolio;
    rapidjson::Value::ConstMemberIterator dataIt = doc.FindMember("data");
    Error err                                    = Error::NoError;

    portfolio.entries.resize(
        dataIt->value.FindMember("Account")->value.GetArray().Size());

    err = ParsePortfolioAccount(dataIt->value, portfolio);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParsePortfolioSymbol(dataIt->value, portfolio);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParsePortfolioAsset(dataIt->value, portfolio);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParsePortfolioCurrency(dataIt->value, portfolio);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParsePortfolioQuantity(dataIt->value, portfolio);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParsePortfolioAvgPrice(dataIt->value, portfolio);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParsePortfolioMarketPrice(dataIt->value, portfolio);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    return portfolio;
}

Error Tradeville::ParsePortfolioAccount(
    const rapidjson::Value& doc,
    Portfolio& portfolio)
{
    auto accountArray = doc.FindMember("Account")->value.GetArray();

    for (size_t i = 0; i < portfolio.entries.size(); i++) {
        if (accountArray[i].IsString() == false) {
            return Error::TradevilleInvalidAccount;
        }
        portfolio.entries[i].account = accountArray[i].GetString();
    }

    return Error::NoError;
}

Error Tradeville::ParsePortfolioSymbol(
    const rapidjson::Value& doc,
    Portfolio& portfolio)
{
    auto symbolArray = doc.FindMember("Symbol")->value.GetArray();

    for (size_t i = 0; i < portfolio.entries.size(); i++) {
        if (symbolArray[i].IsString() == false) {
            return Error::TradevilleInvalidSymbol;
        }
        portfolio.entries[i].symbol = symbolArray[i].GetString();
    }

    return Error::NoError;
}

Error Tradeville::ParsePortfolioQuantity(
    const rapidjson::Value& doc,
    Portfolio& portfolio)
{
    auto quantityArray = doc.FindMember("Quantity")->value.GetArray();

    for (size_t i = 0; i < portfolio.entries.size(); i++) {
        Portfolio::Entry& e = portfolio.entries[i];

        if (e.asset == AssetType::Stock || e.asset == AssetType::Bonds) {
            if (quantityArray[i].IsUint64() == false) {
                return Error::TradevilleInvalidQuantity;
            }
            e.quantity = quantityArray[i].GetUint64();
        } else if (e.asset == AssetType::Money) {
            if (quantityArray[i].IsUint64() == true) {
                e.quantity = static_cast<double>(quantityArray[i].GetUint64());
            } else if (quantityArray[i].IsDouble() == true) {
                e.quantity = quantityArray[i].GetDouble();
            } else {
                return Error::TradevilleInvalidQuantity;
            }
        } else {
            return Error::TradevilleInvalidQuantity;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParsePortfolioAvgPrice(
    const rapidjson::Value& doc,
    Portfolio& portfolio)
{
    auto avgPriceArray = doc.FindMember("AvgPrice")->value.GetArray();

    for (size_t i = 0; i < portfolio.entries.size(); i++) {
        Portfolio::Entry& e = portfolio.entries[i];

        if (e.asset == AssetType::Stock || e.asset == AssetType::Bonds) {
            if (avgPriceArray[i].IsUint64() == true) {
                e.avg_price = static_cast<double>(avgPriceArray[i].GetUint64());
            } else if (avgPriceArray[i].IsDouble() == true) {
                e.avg_price = avgPriceArray[i].GetDouble();
            } else {
                return Error::TradevilleInvalidAvgPrice;
            }
        } else if (e.asset == AssetType::Money) {
            if (avgPriceArray[i].IsUint64() == false ||
                avgPriceArray[i].GetUint64() != 0) {
                return Error::TradevilleInvalidAvgPrice;
            }
            e.avg_price = 0.0;
        } else {
            return Error::TradevilleInvalidAvgPrice;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParsePortfolioMarketPrice(
    const rapidjson::Value& doc,
    Portfolio& portfolio)
{
    auto marketPriceArray = doc.FindMember("MarketPrice")->value.GetArray();

    for (size_t i = 0; i < portfolio.entries.size(); i++) {
        Portfolio::Entry& e = portfolio.entries[i];

        if (e.asset == AssetType::Stock || e.asset == AssetType::Bonds) {
            if (marketPriceArray[i].IsUint64() == true) {
                e.market_price =
                    static_cast<double>(marketPriceArray[i].GetUint64());
            } else if (marketPriceArray[i].IsDouble() == true) {
                e.market_price = marketPriceArray[i].GetDouble();
            } else {
                return Error::TradevilleInvalidMarketPrice;
            }
        } else if (e.asset == AssetType::Money) {
            if (marketPriceArray[i].IsUint64() == false ||
                marketPriceArray[i].GetUint64() != 1) {
                return Error::TradevilleInvalidMarketPrice;
            }
            e.market_price = 1.0;
        } else {
            return Error::TradevilleInvalidMarketPrice;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParsePortfolioAsset(
    const rapidjson::Value& doc,
    Portfolio& portfolio)
{
    auto assetArray = doc.FindMember("PType")->value.GetArray();

    for (size_t i = 0; i < portfolio.entries.size(); i++) {
        if (assetArray[i].IsString() == false) {
            return Error::TradevilleInvalidAsset;
        }

        Portfolio::Entry& e = portfolio.entries[i];
        std::string type    = assetArray[i].GetString();

        if (type == "A") {
            if (e.account.ends_with("-RE") == true) {
                e.asset = AssetType::Bonds;
            } else {
                e.asset = AssetType::Stock;
            }
        } else if (type == "B") {
            e.asset = AssetType::Money;
        } else {
            return Error::TradevilleInvalidAsset;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParsePortfolioCurrency(
    const rapidjson::Value& doc,
    Portfolio& portfolio)
{
    auto currencyArray = doc.FindMember("Ccy")->value.GetArray();

    for (size_t i = 0; i < portfolio.entries.size(); i++) {
        if (currencyArray[i].IsString() == false) {
            return Error::TradevilleInvalidCurrency;
        }

        Portfolio::Entry& e  = portfolio.entries[i];
        std::string currency = currencyArray[i].GetString();

        if (currency == "RON") {
            e.currency = Currency::Ron;
        } else if (currency == "USD") {
            e.currency = Currency::Usd;
        } else if (currency == "EUR") {
            e.currency = Currency::Eur;
        } else {
            return Error::TradevilleInvalidCurrency;
        }
    }

    return Error::NoError;
}

std::string Tradeville::GetActivityRequest(
    const std::optional<std::string>& symbol,
    uint64_t startYear,
    uint64_t endYear)
{
    rapidjson::StringBuffer strBuff;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuff);
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    rapidjson::Value prm(rapidjson::kObjectType);
    std::string dstart = "1jan" + std::to_string(startYear % 100);
    std::string dend   = "31dec" + std::to_string(endYear % 100);

    doc.SetObject();

    if (symbol.has_value() == true) {
        prm.AddMember(
            "symbol",
            rapidjson::Value{}.SetString(symbol->c_str(), allocator),
            allocator);
    } else {
        prm.AddMember("symbol", rapidjson::Value{}.SetNull(), allocator);
    }

    prm.AddMember(
        "dstart",
        rapidjson::Value{}.SetString(dstart.c_str(), allocator),
        allocator);
    prm.AddMember(
        "dend",
        rapidjson::Value{}.SetString(dend.c_str(), allocator),
        allocator);

    doc.AddMember("cmd", "Activity", allocator);
    doc.AddMember("prm", prm, allocator);

    doc.Accept(writer);

    return std::string(strBuff.GetString(), strBuff.GetSize());
}

tl::expected<rapidjson::Document, Error> Tradeville::ValidateActivityJson(
    const std::string& data,
    const std::optional<std::string>& symbol,
    uint64_t startYear,
    uint64_t endYear)
{
    static std::vector<std::string_view> kDataArrays = {
        "Date",
        "OpType",
        "Symbol",
        "Quantity",
        "Price",
        "Comission",
        "Ammount",
        "CashPos",
        "InstrPos",
        "Profit",
        "TranzNo",
        "Ccy",
        "Obs",
        "AvgPrice",
        "OrderId",
        "Tax",
        "Market",
    };

    rapidjson::Document doc;
    std::string dstart = "1jan" + std::to_string(startYear % 100);
    std::string dend   = "31dec" + std::to_string(endYear % 100);

    doc.Parse(data.c_str(), data.size());

    if (doc.IsObject() == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (VerifyStrField(doc, "cmd", "Activity") == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    rapidjson::Value::ConstMemberIterator prmIt = doc.FindMember("prm");
    if (prmIt == doc.MemberEnd()) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (prmIt->value.IsObject() == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (symbol) {
        if (VerifyStrField(prmIt->value, "symbol", symbol->c_str()) == false) {
            return tl::unexpected(Error::UnexpectedData);
        }
    } else {
        if (VerifyNullField(prmIt->value, "symbol") == false) {
            return tl::unexpected(Error::UnexpectedData);
        }
    }

    if (VerifyStrField(prmIt->value, "dstart", dstart.c_str()) == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (VerifyStrField(prmIt->value, "dend", dend.c_str()) == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    rapidjson::Value::ConstMemberIterator dataIt = doc.FindMember("data");
    if (dataIt == doc.MemberEnd()) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (dataIt->value.IsObject() == false) {
        return tl::unexpected(Error::UnexpectedData);
    }

    if (! VerifyArrays(dataIt->value, kDataArrays)) {
        return tl::unexpected(Error::UnexpectedData);
    }

    return doc;
}

tl::expected<Activities, Error> Tradeville::ParseActivity(
    const rapidjson::Document& doc)
{
    Activities activities;
    rapidjson::Value::ConstMemberIterator dataIt = doc.FindMember("data");
    Error err                                    = Error::NoError;

    activities.resize(
        dataIt->value.FindMember("Date")->value.GetArray().Size());

    err = ParseActivityDate(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityNote(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivitySymbol(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityType(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityQuantity(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityPrice(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityCommission(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityCashAmmount(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityCashPosition(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityAssetPosition(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityProfit(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityTransactionId(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityCurrency(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityAvgPrice(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityOrderId(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityTax(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    err = ParseActivityMarket(dataIt->value, activities);
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    return activities;
}

Error Tradeville::ParseActivityDate(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto dateArray = doc.FindMember("Date")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (dateArray[i].IsString() == false) {
            return Error::TradevilleInvalidDate;
        }
        activities[i].date = dateArray[i].GetString();

        // set ymd field
        {
            std::tm tm;
            std::istringstream iss(activities[i].date);

            iss >> std::get_time(&tm, "%Y-%m-%d");
            if (iss.fail() == true) {
                return Error::TradevilleInvalidDate;
            }

            activities[i].ymd = std::chrono::year_month_day(
                std::chrono::year(tm.tm_year + 1900),
                std::chrono::month(tm.tm_mon + 1),
                std::chrono::day(tm.tm_mday));
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityType(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto typeArray = doc.FindMember("OpType")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (typeArray[i].IsString() == false) {
            return Error::TradevilleInvalidActivityType;
        }
        if (std::string_view{"Buy"} == typeArray[i].GetString()) {
            activities[i].type = ActivityType::Buy;
        } else if (std::string_view{"Sell"} == typeArray[i].GetString()) {
            activities[i].type = ActivityType::Sell;
        } else if (std::string_view{"X"} == typeArray[i].GetString()) {
            activities[i].type = ActivityType::Tax;
        } else if (std::string_view{"In"} == typeArray[i].GetString()) {
            if (string_contains_ci(activities[i].note, "dividend") ||
                string_contains_ci(activities[i].note, "plata cupon")) {
                activities[i].type = ActivityType::Dividend;
                continue;
            }

            auto currency = magic_enum::enum_cast<Currency>(
                activities[i].symbol,
                magic_enum::case_insensitive);
            if (! currency || currency == Currency::Unknown) {
                activities[i].type = ActivityType::AssetTransfer;
            } else {
                activities[i].type = ActivityType::Deposit;
            }
        } else if (std::string_view{"Out"} == typeArray[i].GetString()) {
            activities[i].type = ActivityType::Out;
        } else {
            return Error::TradevilleInvalidActivityType;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivitySymbol(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto symbolArray = doc.FindMember("Symbol")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (symbolArray[i].IsString() == false) {
            return Error::TradevilleInvalidSymbol;
        }
        activities[i].symbol = symbolArray[i].GetString();
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityQuantity(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto quantityArray = doc.FindMember("Quantity")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (activities[i].type == ActivityType::Buy ||
            activities[i].type == ActivityType::Sell ||
            activities[i].type == ActivityType::AssetTransfer) {
            if (quantityArray[i].IsUint64() == false) {
                return Error::TradevilleInvalidQuantity;
            }
            activities[i].quantity = quantityArray[i].GetUint64();
        } else if (
            activities[i].type == ActivityType::Deposit ||
            activities[i].type == ActivityType::Dividend ||
            activities[i].type == ActivityType::Out) {
            if (quantityArray[i].IsUint64() == true) {
                activities[i].quantity = quantityArray[i].GetUint64();
            } else if (quantityArray[i].IsDouble() == true) {
                activities[i].quantity = quantityArray[i].GetDouble();
            } else {
                return Error::TradevilleInvalidQuantity;
            }
        } else if (activities[i].type == ActivityType::Tax) {
            if (quantityArray[i].IsUint64() == false ||
                quantityArray[i].GetUint64() != 0) {
                return Error::TradevilleInvalidQuantity;
            }
            activities[i].quantity = 0ull;
        } else {
            return Error::TradevilleInvalidActivityType;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityPrice(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto priceArray = doc.FindMember("Price")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (activities[i].type == ActivityType::Buy ||
            activities[i].type == ActivityType::Sell ||
            activities[i].type == ActivityType::AssetTransfer) {
            if (priceArray[i].IsUint64() == true) {
                activities[i].price =
                    static_cast<double>(priceArray[i].GetUint64());
            } else if (priceArray[i].IsDouble() == true) {
                activities[i].price = priceArray[i].GetDouble();
            } else if (priceArray[i].IsNull() == true) {
                activities[i].price = 0.0;
            } else {
                return Error::TradevilleInvalidPrice;
            }
        } else {
            if (priceArray[i].IsNull() == false) {
                return Error::TradevilleInvalidPrice;
            }
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityCommission(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto commissionArray = doc.FindMember("Comission")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (commissionArray[i].IsUint64() == true) {
            activities[i].commission =
                static_cast<double>(commissionArray[i].GetUint64());
        } else if (commissionArray[i].IsDouble() == true) {
            activities[i].commission = commissionArray[i].GetDouble();
        } else if (commissionArray[i].IsNull() == false) {
            return Error::TradevilleInvalidCommission;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityCashAmmount(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto ammountArray = doc.FindMember("Ammount")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (ammountArray[i].IsUint64() == true) {
            activities[i].cash_ammount =
                static_cast<double>(ammountArray[i].GetUint64());
        } else if (ammountArray[i].IsInt64() == true) {
            activities[i].cash_ammount =
                static_cast<double>(ammountArray[i].GetInt64());
        } else if (ammountArray[i].IsDouble() == true) {
            activities[i].cash_ammount = ammountArray[i].GetDouble();
        } else {
            return Error::TradevilleInvalidCashAmmount;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityCashPosition(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto cashArray = doc.FindMember("CashPos")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (cashArray[i].IsUint64() == true) {
            activities[i].cash_position =
                static_cast<double>(cashArray[i].GetUint64());
        } else if (cashArray[i].IsDouble() == true) {
            activities[i].cash_position = cashArray[i].GetDouble();
        } else {
            return Error::TradevilleInvalidCashPosition;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityAssetPosition(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto assetArray = doc.FindMember("InstrPos")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (assetArray[i].IsUint64() == true) {
            activities[i].asset_position = assetArray[i].GetUint64();
        } else if (assetArray[i].IsNull() == false) {
            return Error::TradevilleInvalidAssetPosition;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityProfit(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto profitArray = doc.FindMember("Profit")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (profitArray[i].IsUint64() == true) {
            activities[i].profit =
                static_cast<double>(profitArray[i].GetUint64());
        } else if (profitArray[i].IsDouble() == true) {
            activities[i].profit = profitArray[i].GetDouble();
        } else if (profitArray[i].IsNull() == false) {
            return Error::TradevilleInvalidProfit;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityTransactionId(
    const rapidjson::Value& doc,
    Activities& activities)
{
    static constexpr std::string_view kDvdNote = "Dividend net ";

    auto transactionArray = doc.FindMember("TranzNo")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (transactionArray[i].IsString() == false) {
            return Error::TradevilleInvalidTransactionId;
        }
        activities[i].transaction_id = transactionArray[i].GetString();

        if (activities[i].type == ActivityType::Dividend) {
            if (activities[i].transaction_id.empty() == false) {
                activities[i].symbol = activities[i].transaction_id;
                continue;
            }

            if (activities[i].note.starts_with(kDvdNote) == true) {
                activities[i].symbol =
                    activities[i].note.substr(kDvdNote.size());
                continue;
            }

            return Error::TradevilleInvalidSymbol;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityCurrency(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto currencyArray = doc.FindMember("Ccy")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (currencyArray[i].IsString() == false) {
            return Error::TradevilleInvalidCurrency;
        }

        std::string currency = currencyArray[i].GetString();

        if (currency == "RON") {
            activities[i].currency = Currency::Ron;
        } else if (currency == "USD") {
            activities[i].currency = Currency::Usd;
        } else if (currency == "EUR") {
            activities[i].currency = Currency::Eur;
        } else {
            return Error::TradevilleInvalidCurrency;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityNote(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto noteArray = doc.FindMember("Obs")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (noteArray[i].IsNull() == true) {
            continue;
        }
        if (noteArray[i].IsString() == false) {
            return Error::TradevilleInvalidNote;
        }
        activities[i].note = noteArray[i].GetString();
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityAvgPrice(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto avgPriceArray = doc.FindMember("AvgPrice")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (avgPriceArray[i].IsUint64() == true) {
            activities[i].avg_price =
                static_cast<double>(avgPriceArray[i].GetUint64());
        } else if (avgPriceArray[i].IsDouble() == true) {
            activities[i].avg_price = avgPriceArray[i].GetDouble();
        } else if (avgPriceArray[i].IsNull() == false) {
            return Error::TradevilleInvalidAvgPrice;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityOrderId(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto orderIdArray = doc.FindMember("OrderId")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (orderIdArray[i].IsUint64() == true) {
            activities[i].order_id = orderIdArray[i].GetUint64();
        } else if (orderIdArray[i].IsNull() == false) {
            return Error::TradevilleInvalidOrderId;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityTax(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto taxArray = doc.FindMember("Tax")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (taxArray[i].IsUint64() == true) {
            activities[i].tax = static_cast<double>(taxArray[i].GetUint64());
        } else if (taxArray[i].IsDouble() == true) {
            activities[i].tax = taxArray[i].GetDouble();
        } else if (taxArray[i].IsNull() == false) {
            return Error::TradevilleInvalidTax;
        }
    }

    return Error::NoError;
}

Error Tradeville::ParseActivityMarket(
    const rapidjson::Value& doc,
    Activities& activities)
{
    auto marketArray = doc.FindMember("Market")->value.GetArray();

    for (size_t i = 0; i < activities.size(); i++) {
        if (marketArray[i].IsString() == true) {
            activities[i].market = marketArray[i].GetString();
        } else if (marketArray[i].IsNull() == false) {
            return Error::TradevilleInvalidMarket;
        }
    }

    return Error::NoError;
}

bool Tradeville::VerifyStrField(
    const rapidjson::Value& doc,
    const std::string& name,
    const std::string& value)
{
    auto it = doc.FindMember(name.c_str());
    if (it == doc.MemberEnd()) {
        return false;
    }

    if (it->value.IsString() == false) {
        return false;
    }

    if (it->value.GetString() != value) {
        return false;
    }

    return true;
}

bool Tradeville::VerifyIntField(
    const rapidjson::Value& doc,
    const std::string& name,
    int value)
{
    auto it = doc.FindMember(name.c_str());
    if (it == doc.MemberEnd()) {
        return false;
    }

    if (it->value.IsInt() == false) {
        return false;
    }

    if (it->value.GetInt() != value) {
        return false;
    }

    return true;
}

bool Tradeville::VerifyBoolField(
    const rapidjson::Value& doc,
    const std::string& name,
    bool value)
{
    auto it = doc.FindMember(name.c_str());
    if (it == doc.MemberEnd()) {
        return false;
    }

    if (it->value.IsBool() == false) {
        return false;
    }

    if (it->value.GetBool() != value) {
        return false;
    }

    return true;
}

bool Tradeville::VerifyNullField(
    const rapidjson::Value& doc,
    const std::string& name)
{
    auto it = doc.FindMember(name.c_str());
    if (it == doc.MemberEnd()) {
        return false;
    }

    if (it->value.IsNull() == false) {
        return false;
    }

    return true;
}

bool Tradeville::VerifyArrays(
    const rapidjson::Value& doc,
    const std::vector<std::string_view>& arrays)
{
    bool firstArray  = true;
    size_t arraySize = 0;

    for (const auto& e : arrays) {
        auto it = doc.FindMember(e.data());
        if (it == doc.MemberEnd()) {
            return false;
        }

        if (it->value.IsArray() == false) {
            return false;
        }

        if (firstArray == true) {
            arraySize  = it->value.GetArray().Size();
            firstArray = false;
        } else if (it->value.GetArray().Size() != arraySize) {
            return false;
        }
    }

    return true;
}
