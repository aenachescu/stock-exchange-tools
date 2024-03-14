#include "tradeville.h"

tl::expected<AssetValue, Error> Portfolio::GetValueByAsset(
    Currency currency,
    const ExchangeRates& rates) const
{
    AssetValue res;

    for (const auto& e : entries) {
        double val = 0.0;
        if (std::holds_alternative<uint64_t>(e.quantity) == true) {
            val = std::get<uint64_t>(e.quantity) * e.market_price;
        } else {
            val = std::get<double>(e.quantity) * e.market_price;
        }

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
        double val = 0.0;
        if (std::holds_alternative<uint64_t>(e.quantity) == true) {
            val = std::get<uint64_t>(e.quantity) * e.market_price;
        } else {
            val = std::get<double>(e.quantity) * e.market_price;
        }

        auto it = res.emplace(e.currency, val);
        if (it.second == false) {
            it.first->second += val;
        }
    }

    return res;
}

AssetAndCurrencyValue Portfolio::GetValueByAssetAndCurrency() const
{
    AssetAndCurrencyValue res;

    for (const auto& e : entries) {
        double val = 0.0;
        if (std::holds_alternative<uint64_t>(e.quantity) == true) {
            val = std::get<uint64_t>(e.quantity) * e.market_price;
        } else {
            val = std::get<double>(e.quantity) * e.market_price;
        }

        auto assetIt    = res.emplace(e.asset, CurrencyValue{});
        auto currencyIt = assetIt.first->second.emplace(e.currency, val);
        if (currencyIt.second == false) {
            currencyIt.first->second += val;
        }
    }

    return res;
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

    if (VerifyLoginResponse(rsp.value()) == false) {
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
    return Activities{};
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
