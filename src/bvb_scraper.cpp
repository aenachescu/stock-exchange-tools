#include "bvb_scraper.h"

#include <utility>

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
        return ParseConstituents(rsp.value().body);
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
        tl::unexpected(reqData.error());
    }

    rsp = SelectIndex(name, reqData.value());
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    return ParseConstituents(rsp.value().body);
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
    std::string_view attrValue)
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
            std::string val = data.substr(
                tdLocations.value()[i].data.Lower(),
                tdLocations.value()[i].data.Size());
            if (! columns[i].validator(val)) {
                return tl::unexpected(Error::InvalidValue);
            }

            columns[i].setter(entry, val);
        }

        table.push_back(std::move(entry));
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

#define DEF_SETTER(field, func)                                                \
    static TableValueSetter<IndexPerformance> field =                          \
        [](IndexPerformance& ip, const std::string& val) -> void {             \
        ip.field = func(val);                                                  \
    }
#define NO_FUNC(val)  val
#define YTD_FUNC(val) (val == "&nbsp;" ? 0.0 : std::stod(val))

    DEF_SETTER(name, NO_FUNC);
    DEF_SETTER(today, std::stod);
    DEF_SETTER(one_week, std::stod);
    DEF_SETTER(one_month, std::stod);
    DEF_SETTER(six_months, std::stod);
    DEF_SETTER(one_year, std::stod);
    DEF_SETTER(year_to_date, YTD_FUNC);

#undef YTD_FUNC
#undef NO_FUNC
#undef DEF_SETTER

    TableValueValidator isValidName = [this](const std::string& val) -> bool {
        return this->IsValidIndexName(val);
    };
    TableValueValidator isValidValue = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, true, false);
    };
    TableValueValidator isValidYtd = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, true, true);
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
        kTableId);
}

tl::expected<Index, Error> BvbScraper::ParseConstituents(
    const std::string& data)
{
    return tl::unexpected(Error::InvalidArg);
}
