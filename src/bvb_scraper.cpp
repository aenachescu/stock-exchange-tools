#include "bvb_scraper.h"

#include <utility>

#define DEF_SETTER(entry, field, func)                                         \
    static TableValueSetter<entry> field =                                     \
        [](entry& e, const std::string& val) -> void { e.field = func(val); }
#define NO_FUNC(val)             val
#define NBSP_OR_DOUBLE_FUNC(val) (val == "&nbsp;" ? 0.0 : std::stod(val))

uint64_t StringToU64(const std::string& val)
{
    uint64_t res = 0;

    for (char c : val) {
        if (std::isdigit(c)) {
            res = res * 10 + (c - '0');
        }
    }

    return res;
}

tl::expected<IndexesNames, Error> BvbScraper::GetIndexesNames()
{
    auto rsp = GetIndicesProfilesPage();
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    auto res = ParseIndexesNames(rsp.value().body);
    if (! res) {
        return tl::unexpected(res.error());
    }

    return std::move(res.value().names);
}

tl::expected<IndexesPerformance, Error> BvbScraper::GetIndexesPerformance()
{
    auto rsp = GetIndicesProfilesPage();
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    return ParseIndexesPerformance(rsp.value().body);
}

tl::expected<Index, Error> BvbScraper::GetConstituents(const IndexName& name)
{
    auto rsp = GetIndicesProfilesPage();
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    auto indexesDetails = ParseIndexesNames(rsp.value().body);
    if (! indexesDetails) {
        return tl::unexpected(indexesDetails.error());
    }

    if (indexesDetails.value().selected == name) {
        return ParseConstituents(rsp.value().body, name);
    }

    auto it = std::find(
        indexesDetails.value().names.begin(),
        indexesDetails.value().names.end(),
        name);
    if (it == indexesDetails.value().names.end()) {
        return tl::unexpected(Error::InvalidArg);
    }

    auto reqData = ParseRequestDataFromMainPage(rsp.value().body);
    if (! reqData) {
        return tl::unexpected(reqData.error());
    }

    rsp = SelectIndex(name, reqData.value());
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    return ParseConstituents(rsp.value().body, name);
}

bool BvbScraper::IsValidIndexName(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    if (! std::isalpha(name.front()) || ! std::isalpha(name.back())) {
        return false;
    }

    for (size_t i = 1; i < name.size() - 1; i++) {
        if (isalpha(name[i])) {
            continue;
        }
        if (name[i] == '-' && isalpha(name[i - 1]) && isalpha(name[i + 1])) {
            continue;
        }

        return false;
    }

    return true;
}

bool BvbScraper::IsValidCompanySymbol(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    for (char c : name) {
        if (std::isupper(c) || std::isdigit(c)) {
            continue;
        }

        return false;
    }

    return true;
}

bool BvbScraper::IsValidCompanyName(const std::string& name)
{
    if (name.empty()) {
        return false;
    }

    if (! std::isalpha(name[0])) {
        return false;
    }

    if (! std::isalpha(name.back()) && name.back() != '.' &&
        name.back() != ')') {
        return false;
    }

    for (size_t i = 1; i < name.size() - 1; i++) {
        if (name[i] == '(' || name[i] == ')') {
            continue;
        }
        if (std::isalpha(name[i])) {
            continue;
        }
        if (name[i] == '-' || name[i] == ' ') {
            continue;
        }
        if (name[i] == '.' && std::isalpha(name[i - 1])) {
            continue;
        }

        return false;
    }

    return true;
}

bool BvbScraper::IsValidInt(const std::string& value)
{
    if (value.empty()) {
        return false;
    }

    if (! std::isdigit(value[0]) || value[0] == '0') {
        return false;
    }

    int digits = 0;
    for (auto it = value.rbegin(); it != value.rend(); ++it) {
        if (digits == 3) {
            if (*it != ',') {
                return false;
            }
            digits = 0;
            continue;
        }
        if (! std::isdigit(*it)) {
            return false;
        }
        digits++;
    }

    return true;
}

bool BvbScraper::IsValidDouble(
    const std::string& val,
    size_t decimals,
    bool allowNegative,
    bool allowNbsp)
{
    size_t i        = 0;
    size_t pointPos = 0;

    if (val.empty()) {
        return false;
    }

    if (allowNbsp == true && val == "&nbsp;") {
        return true;
    }

    if (allowNegative == true && val[0] == '-') {
        i = 1;
    }

    if (val.size() < decimals + i + 2) {
        return false;
    }

    pointPos = val.size() - decimals - 1;

    if (val[i] == '0') {
        if (val[i + 1] != '.') {
            return false;
        }
        i += 2;
    }

    for (; i < val.size(); i++) {
        if (i == pointPos) {
            if (val[i] != '.') {
                return false;
            }
            continue;
        }
        if (! std::isdigit(val[i])) {
            return false;
        }
    }

    return true;
}

tl::expected<HttpResponse, Error> BvbScraper::SendHttpRequest(
    const char* url,
    const CurlHeaders& headers,
    HttpMethod method,
    HttpVersion version)
{
    Error err = Error::NoError;
    ScopedCurl curl;

    if (! curl) {
        return tl::unexpected(Error::CurlInitError);
    }

#define RETURN_IF_ERROR(func)                                                  \
    err = func;                                                                \
    if (err != Error::NoError) {                                               \
        return tl::unexpected(err);                                            \
    }

    RETURN_IF_ERROR(curl.SetHttpMethod(method));
    RETURN_IF_ERROR(curl.SetHttpVersion(version));
    RETURN_IF_ERROR(curl.SetUrl(url));
    RETURN_IF_ERROR(curl.SetEncoding("gzip"));
    RETURN_IF_ERROR(curl.SetHeaders(headers));

#undef RETURN_IF_ERROR

    return curl.Perform();
}

tl::expected<HttpResponse, Error> BvbScraper::GetIndicesProfilesPage()
{
    CurlHeaders headers;
    Error err = Error::NoError;

    err = headers.Add({
        "Host: m.bvb.ro",
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 "
        "Firefox/111.0",
        "Accept: "
        "text/html,application/xhtml+xml,application/xml;q=0.9,image/"
        "avif,image/webp,*/*;q=0.8",
        "Accept-Language: en-US,en;q=0.5",
        "Referer: https://www.google.com/",
        "Cookie: MobBVBCulturePref=en-US",
        "Upgrade-Insecure-Requests: 1",
        "Sec-Fetch-Dest: document",
        "Sec-Fetch-Mode: navigate",
        "Sec-Fetch-Site: cross-site",
        "Sec-Fetch-User: ?1",
        "Pragma: no-cache",
        "Cache-Control: no-cache",
    });
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    auto rsp = SendHttpRequest(
        "https://m.bvb.ro/FinancialInstruments/Indices/IndicesProfiles",
        headers);
    if (! rsp) {
        return rsp;
    }

    if (rsp.value().code != 200) {
        return tl::unexpected(Error::UnexpectedResponseCode);
    }

    return std::move(rsp);
}

tl::expected<HttpResponse, Error> BvbScraper::SelectIndex(
    const IndexName& name,
    const RequestData& reqData)
{
    return tl::unexpected(Error::InvalidArg);
}

tl::expected<BvbScraper::RequestData, Error> BvbScraper::
    ParseRequestDataFromMainPage(const std::string& data)
{
    return tl::unexpected(Error::InvalidArg);
}

tl::expected<BvbScraper::RequestData, Error> BvbScraper::
    ParseRequestDataFromPostRsp(const std::string& data)
{
    return tl::unexpected(Error::InvalidArg);
}

template <typename Table, typename Entry>
tl::expected<Table, Error> BvbScraper::ParseTable(
    const std::vector<TableColumnDetails<Entry>>& columns,
    const std::string& data,
    ClosedInterval ci,
    HtmlAttribute attr,
    std::string_view attrValue,
    AddEntryToTable<Table, Entry> addFunc)
{
    Table table;
    HtmlParser html(data);

    auto tableLocation = html.FindElement(HtmlTag::Table, ci, attr, attrValue);
    if (! tableLocation) {
        return tl::unexpected(tableLocation.error());
    }

    auto theadLocation =
        html.FindElement(HtmlTag::Thead, tableLocation.value().data);
    if (! theadLocation) {
        return tl::unexpected(theadLocation.error());
    }

    auto trLocation = html.FindElement(HtmlTag::Tr, theadLocation.value().data);
    if (! trLocation) {
        return tl::unexpected(trLocation.error());
    }

    auto thLocations =
        html.FindAllElements(HtmlTag::Th, trLocation.value().data);
    if (! thLocations) {
        return tl::unexpected(thLocations.error());
    }

    if (thLocations.value().size() != columns.size()) {
        return tl::unexpected(Error::UnexpectedData);
    }

    for (size_t i = 0; i < columns.size(); i++) {
        std::string_view val(
            data.c_str() + thLocations.value()[i].data.Lower(),
            thLocations.value()[i].data.Size());
        if (val != columns[i].name) {
            return tl::unexpected(Error::InvalidData);
        }
    }

    auto tbodyLocation =
        html.FindElement(HtmlTag::Tbody, tableLocation.value().data);
    if (! tbodyLocation) {
        return tl::unexpected(tbodyLocation.error());
    }

    auto trLocations =
        html.FindAllElements(HtmlTag::Tr, tbodyLocation.value().data);
    if (! trLocations) {
        return tl::unexpected(trLocations.error());
    }

    for (const auto& loc : trLocations.value()) {
        auto tdLocations = html.FindAllElements(HtmlTag::Td, loc.data);
        if (! tdLocations) {
            return tl::unexpected(tdLocations.error());
        }

        if (tdLocations.value().size() != columns.size()) {
            return tl::unexpected(Error::UnexpectedData);
        }

        Entry entry;

        for (size_t i = 0; i < tdLocations.value().size(); i++) {
            std::string val;
            if (columns[i].innerTag != HtmlTag::None) {
                auto innerLocation = html.FindElement(
                    columns[i].innerTag,
                    tdLocations.value()[i].data);
                if (! innerLocation) {
                    return tl::unexpected(innerLocation.error());
                }

                val = data.substr(
                    innerLocation.value().data.Lower(),
                    innerLocation.value().data.Size());
            } else {
                val = data.substr(
                    tdLocations.value()[i].data.Lower(),
                    tdLocations.value()[i].data.Size());
            }

            if (! columns[i].validator(val)) {
                return tl::unexpected(Error::InvalidValue);
            }

            columns[i].setter(entry, val);
        }

        addFunc(table, std::move(entry));
    }

    return table;
}

tl::expected<BvbScraper::IndexesDetails, Error> BvbScraper::ParseIndexesNames(
    const std::string& data)
{
    static constexpr std::string_view kSelectName =
        "ctl00$ctl00$body$rightColumnPlaceHolder$IndexProfilesCurrentValues$"
        "IndexControlList$ddIndices";
    static constexpr std::string_view kSelectedMark = "selected=\"selected\"";

    IndexesDetails res;
    HtmlParser html(data);

    auto selectLocation =
        html.FindElement(HtmlTag::Select, {}, HtmlAttribute::Name, kSelectName);
    if (! selectLocation) {
        return tl::unexpected(selectLocation.error());
    }

    auto optionLocations =
        html.FindAllElements(HtmlTag::Option, selectLocation.value().data);
    if (! optionLocations) {
        return tl::unexpected(optionLocations.error());
    }

    for (const auto& loc : optionLocations.value()) {
        if (loc.data.Empty()) {
            return tl::unexpected(Error::NoData);
        }

        std::string name = data.substr(loc.data.Lower(), loc.data.Size());
        if (! IsValidIndexName(name)) {
            return tl::unexpected(Error::InvalidValue);
        }

        if (res.selected.empty()) {
            std::string_view slice(
                data.c_str() + loc.beginTag.Lower(),
                loc.beginTag.Size());
            if (slice.find(kSelectedMark) != std::string_view::npos) {
                res.selected = name;
            }
        }

        res.names.push_back(std::move(name));
    }

    return res;
}

tl::expected<IndexesPerformance, Error> BvbScraper::ParseIndexesPerformance(
    const std::string& data)
{
    static constexpr std::string_view kTableId = "gvIndexPerformance";

    DEF_SETTER(IndexPerformance, name, NO_FUNC);
    DEF_SETTER(IndexPerformance, today, std::stod);
    DEF_SETTER(IndexPerformance, one_week, std::stod);
    DEF_SETTER(IndexPerformance, one_month, std::stod);
    DEF_SETTER(IndexPerformance, six_months, std::stod);
    DEF_SETTER(IndexPerformance, one_year, std::stod);
    DEF_SETTER(IndexPerformance, year_to_date, NBSP_OR_DOUBLE_FUNC);

    TableValueValidator isValidName = [this](const std::string& val) -> bool {
        return this->IsValidIndexName(val);
    };
    TableValueValidator isValidValue = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, true, false);
    };
    TableValueValidator isValidYtd = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, true, true);
    };

    AddEntryToTable<IndexesPerformance, IndexPerformance> addFunc =
        [](IndexesPerformance& table, IndexPerformance&& entry) -> void {
        table.push_back(std::move(entry));
    };

    std::vector<TableColumnDetails<IndexPerformance>> columns = {
        {"Index", isValidName, name},
        {"today (%)", isValidValue, today},
        {"1 week (%)", isValidValue, one_week},
        {"1 month (%)", isValidValue, one_month},
        {"6 months (%)", isValidValue, six_months},
        {"1 year (%)", isValidValue, one_year},
        {"YTD (%)", isValidYtd, year_to_date},
    };

    return ParseTable<IndexesPerformance, IndexPerformance>(
        columns,
        data,
        {},
        HtmlAttribute::Id,
        kTableId,
        addFunc);
}

tl::expected<Index, Error> BvbScraper::ParseConstituents(
    const std::string& data,
    const IndexName& indexName)
{
    static constexpr std::string_view kTableId = "gvC";

    DEF_SETTER(Company, symbol, NO_FUNC);
    DEF_SETTER(Company, name, NO_FUNC);
    DEF_SETTER(Company, shares, StringToU64);
    DEF_SETTER(Company, reference_price, std::stod);
    DEF_SETTER(Company, free_float_factor, std::stod);
    DEF_SETTER(Company, representation_factor, std::stod);
    DEF_SETTER(Company, price_correction_factor, std::stod);
    DEF_SETTER(Company, weight, std::stod);

    TableValueValidator isValidSymbol = [this](const std::string& val) -> bool {
        return this->IsValidCompanySymbol(val);
    };
    TableValueValidator isValidName = [this](const std::string& val) -> bool {
        return this->IsValidCompanyName(val);
    };
    TableValueValidator isValidShares = [this](const std::string& val) -> bool {
        return this->IsValidInt(val);
    };
    TableValueValidator isValidPDouble2 =
        [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, false, false);
    };
    TableValueValidator isValidPDouble4 =
        [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 4, false, false);
    };
    TableValueValidator isValidPDouble6 =
        [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 6, false, false);
    };

    AddEntryToTable<Index, Company> addFunc = [](Index& i,
                                                 Company&& c) -> void {
        i.companies.push_back(std::move(c));
    };

    std::vector<TableColumnDetails<Company>> columns = {
        {"Symbol", isValidSymbol, symbol, HtmlTag::A},
        {"Company", isValidName, name},
        {"Shares", isValidShares, shares},
        {"Ref. price", isValidPDouble4, reference_price},
        {"FF", isValidPDouble2, free_float_factor},
        {"FR", isValidPDouble6, representation_factor},
        {"FC", isValidPDouble6, price_correction_factor},
        {"Weight (%)", isValidPDouble2, weight},
    };

    auto res = ParseTable<Index, Company>(
        columns,
        data,
        {},
        HtmlAttribute::Id,
        kTableId,
        addFunc);
    if (! res) {
        return tl::unexpected(res.error());
    }

    res.value().name   = indexName;
    res.value().date   = "real-time";
    res.value().reason = "Index Composition";

    return res;
}
