#include "html_parser.h"

static_assert(
    std::string::npos == static_cast<size_t>(-1),
    "Unexpected std::string::npos");

tl::expected<HtmlElementLocation, Error> HtmlParser::FindElement(
    HtmlTag tag,
    ClosedInterval ci,
    HtmlAttribute attr,
    std::string_view attrValue)
{
    if (ci.Upper() >= m_htmlPage.size()) {
        ci.SetUpper(m_htmlPage.size() - 1);
    }

    if (ci.Empty()) {
        return tl::unexpected(Error::InvalidClosedInterval);
    }

    auto eAttrMark = GetAttributeMark(attr, attrValue);
    if (attr != HtmlAttribute::None && ! eAttrMark) {
        return tl::unexpected(eAttrMark.error());
    }

    auto eTagMarks = GetTagMarks(tag);
    if (! eTagMarks) {
        return tl::unexpected(eTagMarks.error());
    }

    const HtmlTagMarks& tagMarks = eTagMarks.value();
    size_t beginPos              = std::string::npos;
    size_t beginStopPos          = std::string::npos;
    size_t endPos                = std::string::npos;
    size_t attrPos               = std::string::npos;
    size_t numOfBeginMarks       = 0;

    while (true) {
        auto eBeginTagPos = FindBeginTagMark(tagMarks, ci);
        if (! eBeginTagPos) {
            return tl::unexpected(eBeginTagPos.error());
        }
        beginPos     = eBeginTagPos.value().beginPos;
        beginStopPos = eBeginTagPos.value().beginStopPos;

        if (attr != HtmlAttribute::None) {
            attrPos = FindInInterval(
                eAttrMark.value(),
                {beginPos + tagMarks.begin.size(), beginStopPos - 1});
            if (attrPos == std::string::npos) {
                continue;
            }
        }

        while (true) {
            endPos = FindInInterval(tagMarks.end, ci);
            if (endPos == std::string::npos) {
                return tl::unexpected(Error::IncompleteHtmlElement);
            }

            auto eCount =
                CountBeginTagMarks(tagMarks, {ci.Lower(), endPos - 1});
            if (! eCount) {
                return tl::unexpected(eCount.error());
            }

            numOfBeginMarks += eCount.value();
            if (numOfBeginMarks == 0) {
                break;
            }

            ci.SetLower(endPos + tagMarks.end.size());
            --numOfBeginMarks;
        }

        break;
    }

    return HtmlElementLocation{
        {beginPos, beginStopPos + tagMarks.beginStop.size() - 1},
        {beginStopPos + tagMarks.beginStop.size(), endPos - 1},
        {endPos, endPos + tagMarks.end.size() - 1}};
}

tl::expected<HtmlElementLocations, Error> HtmlParser::FindAllElements(
    HtmlTag tag,
    ClosedInterval ci,
    HtmlAttribute attr,
    std::string_view attrValue)
{
    if (ci.Upper() >= m_htmlPage.size()) {
        ci.SetUpper(m_htmlPage.size() - 1);
    }

    if (ci.Empty()) {
        return tl::unexpected(Error::InvalidClosedInterval);
    }

    auto eAttrMark = GetAttributeMark(attr, attrValue);
    if (attr != HtmlAttribute::None && ! eAttrMark) {
        return tl::unexpected(eAttrMark.error());
    }

    auto eTagMarks = GetTagMarks(tag);
    if (! eTagMarks) {
        return tl::unexpected(eTagMarks.error());
    }

    HtmlElementLocations locations;
    const HtmlTagMarks& tagMarks = eTagMarks.value();
    size_t beginPos              = std::string::npos;
    size_t beginStopPos          = std::string::npos;
    size_t endPos                = std::string::npos;
    size_t attrPos               = std::string::npos;
    size_t numOfBeginMarks       = 0;

    while (true) {
        auto eBeginTagPos = FindBeginTagMark(tagMarks, ci);
        if (! eBeginTagPos) {
            if (eBeginTagPos.error() == Error::HtmlElementNotFound &&
                ! locations.empty()) {
                break;
            }
            return tl::unexpected(eBeginTagPos.error());
        }
        beginPos     = eBeginTagPos.value().beginPos;
        beginStopPos = eBeginTagPos.value().beginStopPos;

        if (attr != HtmlAttribute::None) {
            attrPos = FindInInterval(
                eAttrMark.value(),
                {beginPos + tagMarks.begin.size(), beginStopPos - 1});
            if (attrPos == std::string::npos) {
                continue;
            }
        }

        while (true) {
            endPos = FindInInterval(tagMarks.end, ci);
            if (endPos == std::string::npos) {
                return tl::unexpected(Error::IncompleteHtmlElement);
            }

            auto eCount =
                CountBeginTagMarks(tagMarks, {ci.Lower(), endPos - 1});
            if (! eCount) {
                return tl::unexpected(eCount.error());
            }

            numOfBeginMarks += eCount.value();
            if (numOfBeginMarks == 0) {
                break;
            }

            ci.SetLower(endPos + tagMarks.end.size());
            --numOfBeginMarks;
        }

        locations.push_back(HtmlElementLocation{
            {beginPos, beginStopPos + tagMarks.beginStop.size() - 1},
            {beginStopPos + tagMarks.beginStop.size(), endPos - 1},
            {endPos, endPos + tagMarks.end.size() - 1}});
    }

    return locations;
}

size_t HtmlParser::FindInInterval(std::string_view val, ClosedInterval ci)
{
    if (ci.Empty()) {
        return std::string::npos;
    }

    std::string_view slice(m_htmlPage.c_str() + ci.Lower(), ci.Size());

    size_t pos = slice.find(val);
    if (pos == std::string::npos) {
        return pos;
    }

    return ci.Lower() + pos;
}

tl::expected<HtmlParser::HtmlBeginTagPosition, Error> HtmlParser::
    FindBeginTagMark(const HtmlTagMarks& tagMarks, ClosedInterval& ci)
{
    size_t beginPos = FindInInterval(tagMarks.begin, ci);
    if (beginPos == std::string::npos) {
        return tl::unexpected(Error::HtmlElementNotFound);
    }
    ci.SetLower(beginPos + tagMarks.begin.size());

    size_t beginStopPos = FindInInterval(tagMarks.beginStop, ci);
    if (beginStopPos == std::string::npos) {
        return tl::unexpected(Error::IncompleteHtmlElement);
    }
    ci.SetLower(beginStopPos + tagMarks.beginStop.size());

    if (beginPos + tagMarks.begin.size() != beginStopPos &&
        m_htmlPage[beginPos + tagMarks.begin.size()] != ' ') {
        return tl::unexpected(Error::InvalidHtmlElement);
    }

    return HtmlBeginTagPosition{beginPos, beginStopPos};
}

tl::expected<size_t, Error> HtmlParser::CountBeginTagMarks(
    const HtmlTagMarks& tagMarks,
    ClosedInterval ci)
{
    size_t num = 0;

    while (true) {
        auto res = FindBeginTagMark(tagMarks, ci);
        if (res) {
            num++;
            continue;
        }

        if (res.error() == Error::HtmlElementNotFound) {
            break;
        }

        return tl::unexpected(res.error());
    }

    return num;
}

tl::expected<HtmlParser::HtmlTagMarks, Error> HtmlParser::GetTagMarks(
    HtmlTag tag)
{
    switch (tag) {
    case HtmlTag::Select:
        return HtmlTagMarks{"<select", ">", "</select>"};
    case HtmlTag::Option:
        return HtmlTagMarks{"<option", ">", "</option>"};
    case HtmlTag::Table:
        return HtmlTagMarks{"<table", ">", "</table>"};
    default:
        break;
    }

    return tl::unexpected(Error::InvalidHtmlTag);
}

tl::expected<std::string, Error> HtmlParser::GetAttributeMark(
    HtmlAttribute attr,
    std::string_view value)
{
    std::string mark;
    std::string_view attrStr;

    switch (attr) {
    case HtmlAttribute::Id:
        attrStr = "id";
        break;
    case HtmlAttribute::Name:
        attrStr = "name";
        break;
    case HtmlAttribute::None:
    default:
        return tl::unexpected(Error::InvalidHtmlAttribute);
    }

    mark.reserve(attrStr.size() + value.size() + 4);
    mark = attrStr;
    mark += "=\"";
    mark += value;
    mark += "\"";

    return std::move(mark);
}
