#include "bvb_scraper.h"

#include "html_parser.h"

#include <string_view>
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

bool BvbScraper::IsValidIndexPerformanceValue(const std::string& name)
{
    if (name.size() < 4) {
        return false;
    }

    if (! std::isdigit(name[name.size() - 1]) ||
        ! std::isdigit(name[name.size() - 2]) || name[name.size() - 3] != '.') {
        return false;
    }

    size_t i = 0;

    if (name[0] == '-') {
        if (name.size() < 5) {
            return false;
        }
        i = 1;
    }

    for (; i < name.size() - 3; i++) {
        if (! std::isdigit(name[i])) {
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
            return tl::unexpected(Error::InvalidData);
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
    static constexpr std::array<std::string_view, 7> kColumnNames = {
        "Index",
        "today (%)",
        "1 week (%)",
        "1 month (%)",
        "6 months (%)",
        "1 year (%)",
        "YTD (%)"};

    IndexesPerformance res;
    HtmlParser html(data);

    auto tableLocation =
        html.FindElement(HtmlTag::Table, {}, HtmlAttribute::Id, kTableId);
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

    if (thLocations.value().size() != kColumnNames.size()) {
        return tl::unexpected(Error::UnexpectedData);
    }

    for (size_t i = 0; i < kColumnNames.size(); i++) {
        std::string val = data.substr(
            thLocations.value()[i].data.Lower(),
            thLocations.value()[i].data.Size());
        if (val != kColumnNames[i]) {
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

        if (tdLocations.value().size() != kColumnNames.size()) {
            return tl::unexpected(Error::UnexpectedData);
        }

        IndexPerformance perf;
        std::string val;

        val = data.substr(
            tdLocations.value()[0].data.Lower(),
            tdLocations.value()[0].data.Size());
        if (! IsValidIndexName(val)) {
            return tl::unexpected(Error::InvalidData);
        }
        perf.name = val;

        val = data.substr(
            tdLocations.value()[1].data.Lower(),
            tdLocations.value()[1].data.Size());
        if (! IsValidIndexPerformanceValue(val)) {
            return tl::unexpected(Error::InvalidData);
        }
        perf.today = std::stod(val);

        val = data.substr(
            tdLocations.value()[2].data.Lower(),
            tdLocations.value()[2].data.Size());
        if (! IsValidIndexPerformanceValue(val)) {
            return tl::unexpected(Error::InvalidData);
        }
        perf.one_week = std::stod(val);

        val = data.substr(
            tdLocations.value()[3].data.Lower(),
            tdLocations.value()[3].data.Size());
        if (! IsValidIndexPerformanceValue(val)) {
            return tl::unexpected(Error::InvalidData);
        }
        perf.one_month = std::stod(val);

        val = data.substr(
            tdLocations.value()[4].data.Lower(),
            tdLocations.value()[4].data.Size());
        if (! IsValidIndexPerformanceValue(val)) {
            return tl::unexpected(Error::InvalidData);
        }
        perf.six_months = std::stod(val);

        val = data.substr(
            tdLocations.value()[5].data.Lower(),
            tdLocations.value()[5].data.Size());
        if (! IsValidIndexPerformanceValue(val)) {
            return tl::unexpected(Error::InvalidData);
        }
        perf.one_year = std::stod(val);

        val = data.substr(
            tdLocations.value()[6].data.Lower(),
            tdLocations.value()[6].data.Size());
        if (! IsValidIndexPerformanceValue(val)) {
            return tl::unexpected(Error::InvalidData);
        }
        perf.year_to_date = std::stod(val);

        res.push_back(std::move(perf));
    }

    return res;
}

tl::expected<Index, Error> BvbScraper::ParseConstituents(
    const std::string& data)
{
    return tl::unexpected(Error::InvalidArg);
}
