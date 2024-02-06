#include "bvb_scraper.h"

#include <algorithm>
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

double StringToDouble(std::string val)
{
    val.erase(std::remove(val.begin(), val.end(), ','), val.end());
    return std::stod(val);
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

tl::expected<IndexTradingData, Error> BvbScraper::GetTradingData(
    const IndexName& name)
{
    auto rsp = GetIndicesProfilesPage();
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    auto indexesDetails = ParseIndexesNames(rsp.value().body);
    if (! indexesDetails) {
        return tl::unexpected(indexesDetails.error());
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

    if (indexesDetails.value().selected != name) {
        rsp = SelectIndex(name, reqData.value());
        if (! rsp) {
            return tl::unexpected(rsp.error());
        }

        reqData = ParseRequestDataFromPostRsp(rsp.value().body);
        if (! reqData) {
            return tl::unexpected(reqData.error());
        }
    }

    rsp = SelectTradingData(name, reqData.value());
    if (! rsp) {
        return tl::unexpected(rsp.error());
    }

    return ParseTradingData(rsp.value().body, name);
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

    if (! std::isalnum(name[0])) {
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
        if (std::isalnum(name[i])) {
            continue;
        }
        if (name[i] == '-' || name[i] == ' ' || name[i] == '&') {
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
    bool hasSeparators,
    bool allowNbsp)
{
    size_t firstDigitPos = 0;
    size_t pointPos      = 0;
    int digits           = 0;

    if (val.empty()) {
        return false;
    }

    if (allowNbsp == true && val == "&nbsp;") {
        return true;
    }

    if (allowNegative == true && val[0] == '-') {
        firstDigitPos = 1;
    }

    if (val.size() < decimals + firstDigitPos + 2) {
        return false;
    }

    pointPos = val.size() - decimals - 1;

    if (val[firstDigitPos] == '0' && val[firstDigitPos + 1] != '.') {
        return false;
    }

    if (val[pointPos] != '.') {
        return false;
    }

    for (size_t i = pointPos; i > firstDigitPos; i--) {
        char c = val[i - 1];
        if (hasSeparators == true) {
            if (digits == 3) {
                if (c != ',') {
                    return false;
                }
                digits = 0;
                continue;
            } else {
                digits++;
            }
        }

        if (! std::isdigit(c)) {
            return false;
        }
    }

    for (size_t i = pointPos + 1; i < val.size(); i++) {
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
    HttpVersion version,
    const PostData& postData)
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

    if (method == HttpMethod::post && postData.empty() == false) {
        RETURN_IF_ERROR(curl.SetPostData(postData));
    }

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

    return rsp;
}

tl::expected<HttpResponse, Error> BvbScraper::SelectIndex(
    const IndexName& name,
    const RequestData& reqData)
{
    CurlHeaders headers;
    Error err = Error::NoError;

    PostData postData = {
        {"ctl00$ctl00$MasterScriptManager",
         "ctl00$ctl00$MasterScriptManager|ctl00$ctl00$body$"
         "rightColumnPlaceHolder$IndexProfilesCurrentValues$IndexControlList$"
         "ddIndices"},
        {"__EVENTTARGET",
         "ctl00$ctl00$body$rightColumnPlaceHolder$IndexProfilesCurrentValues$"
         "IndexControlList$ddIndices"},
        {"__EVENTARGUMENT", reqData.eventArg},
        {"__LASTFOCUS", reqData.lastFocus},
        {"__VIEWSTATE", reqData.viewState},
        {"__VIEWSTATEGENERATOR", reqData.viewStateGenerator},
        {"__VIEWSTATEENCRYPTED", reqData.viewStateEncrypted},
        {"__EVENTVALIDATION", reqData.eventValidation},
        {"autocomplete-form-mob", ""},
        {"ctl00$ctl00$body$rightColumnPlaceHolder$IndexProfilesCurrentValues$"
         "IndexControlList$ddIndices",
         name},
        {"gvC_length", "10"},
        {"__ASYNCPOST", "true"},
    };

    err = headers.Add({
        "Host: m.bvb.ro",
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 "
        "Firefox/111.0",
        "Accept: */*",
        "Accept-Language: en-US,en;q=0.5",
        "Referer: "
        "https://m.bvb.ro/FinancialInstruments/Indices/IndicesProfiles",
        "Cookie: MobBVBCulturePref=en-US",
        "X-Requested-With: XMLHttpRequest",
        "X-MicrosoftAjax: Delta=true",
        "Cache-Control: no-cache",
        "Content-Type: application/x-www-form-urlencoded; charset=utf-8",
        "Origin: https://m.bvb.ro",
        "Sec-Fetch-Dest: empty",
        "Sec-Fetch-Mode: cors",
        "Sec-Fetch-Site: same-origin",
    });
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    auto rsp = SendHttpRequest(
        "https://m.bvb.ro/FinancialInstruments/Indices/IndicesProfiles",
        headers,
        HttpMethod::post,
        HttpVersion::http1_1,
        postData);
    if (! rsp) {
        return rsp;
    }

    if (rsp.value().code != 200) {
        return tl::unexpected(Error::UnexpectedResponseCode);
    }

    return rsp;
}

tl::expected<HttpResponse, Error> BvbScraper::SelectTradingData(
    const IndexName& name,
    const RequestData& reqData)
{
    CurlHeaders headers;
    Error err = Error::NoError;

    PostData postData = {
        {"ctl00$ctl00$MasterScriptManager",
         "ctl00$ctl00$body$rightColumnPlaceHolder$TabsControl$upMob|ctl00$"
         "ctl00$body$rightColumnPlaceHolder$TabsControl$lb1"},
        {"autocomplete-form-mob", ""},
        {"ctl00$ctl00$body$rightColumnPlaceHolder$IndexProfilesCurrentValues$"
         "IndexControlList$ddIndices",
         name},
        {"gvC_length", "10"},
        {"__EVENTTARGET",
         "ctl00$ctl00$body$rightColumnPlaceHolder$TabsControl$lb1"},
        {"__EVENTARGUMENT", reqData.eventArg},
        {"__LASTFOCUS", reqData.lastFocus},
        {"__VIEWSTATE", reqData.viewState},
        {"__VIEWSTATEGENERATOR", reqData.viewStateGenerator},
        {"__VIEWSTATEENCRYPTED", reqData.viewStateEncrypted},
        {"__EVENTVALIDATION", reqData.eventValidation},
        {"__ASYNCPOST", "true"},
    };

    err = headers.Add({
        "Host: m.bvb.ro",
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 "
        "Firefox/111.0",
        "Accept: */*",
        "Accept-Language: en-US,en;q=0.5",
        "Referer: "
        "https://m.bvb.ro/FinancialInstruments/Indices/IndicesProfiles",
        "Cookie: MobBVBCulturePref=en-US",
        "X-Requested-With: XMLHttpRequest",
        "X-MicrosoftAjax: Delta=true",
        "Cache-Control: no-cache",
        "Content-Type: application/x-www-form-urlencoded; charset=utf-8",
        "Origin: https://m.bvb.ro",
        "Sec-Fetch-Dest: empty",
        "Sec-Fetch-Mode: cors",
        "Sec-Fetch-Site: same-origin",
    });
    if (err != Error::NoError) {
        return tl::unexpected(err);
    }

    auto rsp = SendHttpRequest(
        "https://m.bvb.ro/FinancialInstruments/Indices/IndicesProfiles",
        headers,
        HttpMethod::post,
        HttpVersion::http1_1,
        postData);
    if (! rsp) {
        return rsp;
    }

    if (rsp.value().code != 200) {
        return tl::unexpected(Error::UnexpectedResponseCode);
    }

    return rsp;
}

tl::expected<std::string_view, Error> BvbScraper::
    ParseRequestDataFieldFromMainPage(
        const std::string& data,
        std::string_view id)
{
    static constexpr std::string_view kValueTag = "value=\"";
    static constexpr char kValueEndTag          = '\"';

    HtmlParser html(data);

    auto inputLocation =
        html.FindElement(HtmlTag::Input, {}, HtmlAttribute::Id, id);
    if (! inputLocation) {
        return tl::unexpected(inputLocation.error());
    }

    std::string_view beginTag(
        data.c_str() + inputLocation.value().beginTag.Lower(),
        inputLocation.value().beginTag.Size());

    size_t valueStartPos = beginTag.find(kValueTag);
    if (valueStartPos == std::string_view::npos) {
        return tl::unexpected(Error::InvalidHtmlElement);
    }
    valueStartPos += kValueTag.size();

    size_t valueEndPos = beginTag.find(kValueEndTag, valueStartPos);
    if (valueEndPos == std::string_view::npos) {
        return tl::unexpected(Error::InvalidHtmlElement);
    }

    return std::string_view(
        data.c_str() + inputLocation.value().beginTag.Lower() + valueStartPos,
        valueEndPos - valueStartPos);
}

tl::expected<BvbScraper::RequestData, Error> BvbScraper::
    ParseRequestDataFromMainPage(const std::string& data)
{
    RequestData reqData;
    tl::expected<std::string_view, Error> fieldValue;

#define PARSE_FIELD_OR_RETURN_IF_ERROR(field, id)                              \
    fieldValue = ParseRequestDataFieldFromMainPage(data, id);                  \
    if (! fieldValue) {                                                        \
        return tl::unexpected(fieldValue.error());                             \
    }                                                                          \
    reqData.field = fieldValue.value();

    PARSE_FIELD_OR_RETURN_IF_ERROR(eventTarget, "__EVENTTARGET");
    PARSE_FIELD_OR_RETURN_IF_ERROR(eventArg, "__EVENTARGUMENT");
    PARSE_FIELD_OR_RETURN_IF_ERROR(eventValidation, "__EVENTVALIDATION");
    PARSE_FIELD_OR_RETURN_IF_ERROR(lastFocus, "__LASTFOCUS");
    PARSE_FIELD_OR_RETURN_IF_ERROR(viewState, "__VIEWSTATE");
    PARSE_FIELD_OR_RETURN_IF_ERROR(viewStateGenerator, "__VIEWSTATEGENERATOR");
    PARSE_FIELD_OR_RETURN_IF_ERROR(viewStateEncrypted, "__VIEWSTATEENCRYPTED");

#undef PARSE_FIELD_OR_RETURN_IF_ERROR

    return reqData;
}

tl::expected<std::string_view, Error> BvbScraper::
    ParseRequestDataFieldFromPostRsp(
        const std::string& data,
        std::string_view id)
{
    size_t idStartPos = data.find(id);
    if (idStartPos == std::string::npos) {
        return tl::unexpected(Error::NoData);
    }

    size_t dataStartPos = idStartPos + id.size();
    size_t dataEndPos   = data.find('|', dataStartPos);
    if (dataEndPos == std::string::npos) {
        return tl::unexpected(Error::InvalidData);
    }

    return std::string_view(
        data.c_str() + dataStartPos,
        dataEndPos - dataStartPos);
}

tl::expected<BvbScraper::RequestData, Error> BvbScraper::
    ParseRequestDataFromPostRsp(const std::string& data)
{
    RequestData reqData;
    tl::expected<std::string_view, Error> fieldValue;

#define PARSE_FIELD_OR_RETURN_IF_ERROR(field, id)                              \
    fieldValue = ParseRequestDataFieldFromPostRsp(data, id);                   \
    if (! fieldValue) {                                                        \
        return tl::unexpected(fieldValue.error());                             \
    }                                                                          \
    reqData.field = fieldValue.value();

    PARSE_FIELD_OR_RETURN_IF_ERROR(eventTarget, "|__EVENTTARGET|");
    PARSE_FIELD_OR_RETURN_IF_ERROR(eventArg, "|__EVENTARGUMENT|");
    PARSE_FIELD_OR_RETURN_IF_ERROR(eventValidation, "|__EVENTVALIDATION|");
    PARSE_FIELD_OR_RETURN_IF_ERROR(lastFocus, "|__LASTFOCUS|");
    PARSE_FIELD_OR_RETURN_IF_ERROR(viewState, "|__VIEWSTATE|");
    PARSE_FIELD_OR_RETURN_IF_ERROR(
        viewStateGenerator,
        "|__VIEWSTATEGENERATOR|");
    PARSE_FIELD_OR_RETURN_IF_ERROR(
        viewStateEncrypted,
        "|__VIEWSTATEENCRYPTED|");

#undef PARSE_FIELD_OR_RETURN_IF_ERROR

    return reqData;
}

tl::expected<std::string_view, Error> BvbScraper::ParseTradingDataTime(
    const std::string& data)
{
    static constexpr std::string_view kLastUpdateDivClass =
        "upHdr d-lg-flex pull-right-lg ";
    static constexpr std::string_view kTimeBeginMark = "Last update: ";
    static constexpr std::string_view kTimeEndMark   = "&nbsp";

    HtmlParser html(data);

    auto divLocation = html.FindElement(
        HtmlTag::Div,
        {},
        HtmlAttribute::Class,
        kLastUpdateDivClass);
    if (! divLocation) {
        return tl::unexpected(divLocation.error());
    }

    std::string_view divData(
        data.c_str() + divLocation.value().data.Lower(),
        divLocation.value().data.Size());

    size_t beginMarkPos = divData.find(kTimeBeginMark);
    if (beginMarkPos == std::string_view::npos) {
        return tl::unexpected(Error::InvalidData);
    }
    beginMarkPos += kTimeBeginMark.size();

    size_t endMarkPos = divData.find(kTimeEndMark, beginMarkPos);
    if (endMarkPos == std::string_view::npos) {
        return tl::unexpected(Error::InvalidData);
    }

    std::string_view date(
        divData.data() + beginMarkPos,
        endMarkPos - beginMarkPos);
    size_t numOfSpaces = 0;

    for (auto it = date.rbegin(); it != date.rend(); ++it) {
        if (std::isspace(static_cast<unsigned char>(*it))) {
            numOfSpaces++;
        } else {
            break;
        }
    }

    return std::string_view{
        divData.data() + beginMarkPos,
        endMarkPos - beginMarkPos - numOfSpaces};
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
        return this->IsValidDouble(val, 2, true, false, false);
    };
    TableValueValidator isValidYtd = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, true, false, true);
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
    DEF_SETTER(Company, liquidity_factor, std::stod);
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
        return this->IsValidDouble(val, 2, false, false, false);
    };
    TableValueValidator isValidPDouble4 =
        [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 4, false, false, false);
    };
    TableValueValidator isValidPDouble6 =
        [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 6, false, false, false);
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

    std::vector<TableColumnDetails<Company>> alternative_columns = {
        {"Symbol", isValidSymbol, symbol, HtmlTag::A},
        {"Company", isValidName, name},
        {"Shares", isValidShares, shares},
        {"Ref. price", isValidPDouble4, reference_price},
        {"FF", isValidPDouble2, free_float_factor},
        {"FR", isValidPDouble6, representation_factor},
        {"FC", isValidPDouble6, price_correction_factor},
        {"FL", isValidPDouble2, liquidity_factor},
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
        res = ParseTable<Index, Company>(
            alternative_columns,
            data,
            {},
            HtmlAttribute::Id,
            kTableId,
            addFunc);
        if (! res) {
            return tl::unexpected(res.error());
        }
    }

    res.value().name   = indexName;
    res.value().date   = "real-time";
    res.value().reason = "Index Composition";

    return res;
}

tl::expected<IndexTradingData, Error> BvbScraper::ParseTradingData(
    const std::string& data,
    const IndexName& indexName)
{
    static constexpr std::string_view kTableId = "gvTD";

    DEF_SETTER(CompanyTradingData, symbol, NO_FUNC);
    DEF_SETTER(CompanyTradingData, price, std::stod);
    DEF_SETTER(CompanyTradingData, variation, std::stod);
    DEF_SETTER(CompanyTradingData, trades, StringToU64);
    DEF_SETTER(CompanyTradingData, volume, StringToU64);
    DEF_SETTER(CompanyTradingData, value, StringToDouble);
    DEF_SETTER(CompanyTradingData, lowest_price, std::stod);
    DEF_SETTER(CompanyTradingData, highest_price, std::stod);
    DEF_SETTER(CompanyTradingData, weight, std::stod);

    TableValueValidator isValidSymbol = [this](const std::string& val) -> bool {
        return this->IsValidCompanySymbol(val);
    };
    TableValueValidator isValidPrice = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 4, false, false, false);
    };
    TableValueValidator isValidVar = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, true, false, false);
    };
    TableValueValidator isValidInt = [this](const std::string& val) -> bool {
        return this->IsValidInt(val);
    };
    TableValueValidator isValidValue = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, false, true, false);
    };
    TableValueValidator isValidWeight = [this](const std::string& val) -> bool {
        return this->IsValidDouble(val, 2, false, false, false);
    };

    AddEntryToTable<IndexTradingData, CompanyTradingData> addFunc =
        [](IndexTradingData& i, CompanyTradingData&& c) -> void {
        i.companies.push_back(std::move(c));
    };

    std::vector<TableColumnDetails<CompanyTradingData>> columns = {
        {"Symbol", isValidSymbol, symbol, HtmlTag::A},
        {"Price", isValidPrice, price},
        {"Var. (%)", isValidVar, variation},
        {"Trades", isValidInt, trades},
        {"Volume", isValidInt, volume},
        {"Value", isValidValue, value},
        {"Low", isValidPrice, lowest_price},
        {"High", isValidPrice, highest_price},
        {"Weight (%)", isValidWeight, weight},
    };

    auto res = ParseTable<IndexTradingData, CompanyTradingData>(
        columns,
        data,
        {},
        HtmlAttribute::Id,
        kTableId,
        addFunc);
    if (! res) {
        return tl::unexpected(res.error());
    }

    auto date = ParseTradingDataTime(data);
    if (! date) {
        return tl::unexpected(date.error());
    }

    res.value().name = indexName;
    res.value().date = date.value();

    return res;
}
