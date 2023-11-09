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
    size_t beginPos     = std::string::npos;
    size_t beginStopPos = std::string::npos;
    size_t endPos       = std::string::npos;
    size_t attrPos      = std::string::npos;

    if (ci.Upper() == std::string::npos) {
        ci.SetUpper(m_htmlPage.size() - 1);
    }

    if (ci.Empty()) {
        return tl::unexpected(Error::InvalidClosedInterval);
    }

    auto attrMark = GetAttributeMark(attr, attrValue);
    if (attr != HtmlAttribute::None && ! attrMark) {
        return tl::unexpected(attrMark.error());
    }

    auto tagMarks = GetTagMarks(tag);
    if (! tagMarks) {
        return tl::unexpected(tagMarks.error());
    }

    while (true) {
        beginPos = FindInInterval(tagMarks.value().begin, ci);
        if (beginPos == std::string::npos) {
            return tl::unexpected(Error::HtmlElementNotFound);
        }
        ci.SetLower(beginPos + tagMarks.value().begin.size());

        beginStopPos = FindInInterval(tagMarks.value().beginStop, ci);
        if (beginStopPos == std::string::npos) {
            return tl::unexpected(Error::IncompleteHtmlElement);
        }
        ci.SetLower(beginStopPos + tagMarks.value().beginStop.size());

        if (beginPos + tagMarks.value().begin.size() != beginStopPos &&
            m_htmlPage[beginPos + tagMarks.value().begin.size()] != ' ') {
            return tl::unexpected(Error::InvalidHtmlElement);
        }

        if (attr != HtmlAttribute::None) {
            attrPos = FindInInterval(
                attrMark.value(),
                {beginPos + tagMarks.value().begin.size(), beginStopPos - 1});
            if (attrPos == std::string::npos) {
                continue;
            }
        }

        endPos = FindInInterval(tagMarks.value().end, ci);
        if (endPos == std::string::npos) {
            return tl::unexpected(Error::IncompleteHtmlElement);
        }

        break;
    }

    return HtmlElementLocation{
        {beginPos, beginStopPos + tagMarks.value().beginStop.size() - 1},
        {beginStopPos + tagMarks.value().beginStop.size(), endPos - 1},
        {endPos, endPos + tagMarks.value().end.size() - 1}};
}

tl::expected<HtmlElementLocations, Error> HtmlParser::FindAllElements(
    HtmlTag tag,
    ClosedInterval ci,
    HtmlAttribute attr,
    std::string_view attrValue)
{
    size_t beginPos     = std::string::npos;
    size_t beginStopPos = std::string::npos;
    size_t endPos       = std::string::npos;
    size_t attrPos      = std::string::npos;

    if (ci.Upper() == std::string::npos) {
        ci.SetUpper(m_htmlPage.size() - 1);
    }

    if (ci.Empty()) {
        return tl::unexpected(Error::InvalidClosedInterval);
    }

    auto attrMark = GetAttributeMark(attr, attrValue);
    if (attr != HtmlAttribute::None && ! attrMark) {
        return tl::unexpected(attrMark.error());
    }

    auto tagMarks = GetTagMarks(tag);
    if (! tagMarks) {
        return tl::unexpected(tagMarks.error());
    }

    HtmlElementLocations locations;

    while (true) {
        beginPos = FindInInterval(tagMarks.value().begin, ci);
        if (beginPos == std::string::npos) {
            if (! locations.empty()) {
                break;
            }

            return tl::unexpected(Error::HtmlElementNotFound);
        }
        ci.SetLower(beginPos + tagMarks.value().begin.size());

        beginStopPos = FindInInterval(tagMarks.value().beginStop, ci);
        if (beginStopPos == std::string::npos) {
            return tl::unexpected(Error::IncompleteHtmlElement);
        }
        ci.SetLower(beginStopPos + tagMarks.value().beginStop.size());

        if (beginPos + tagMarks.value().begin.size() != beginStopPos &&
            m_htmlPage[beginPos + tagMarks.value().begin.size()] != ' ') {
            return tl::unexpected(Error::InvalidHtmlElement);
        }

        if (attr != HtmlAttribute::None) {
            attrPos = FindInInterval(
                attrMark.value(),
                {beginPos + tagMarks.value().begin.size(), beginStopPos - 1});
            if (attrPos == std::string::npos) {
                continue;
            }
        }

        endPos = FindInInterval(tagMarks.value().end, ci);
        if (endPos == std::string::npos) {
            return tl::unexpected(Error::IncompleteHtmlElement);
        }

        locations.push_back(HtmlElementLocation{
            {beginPos, beginStopPos + tagMarks.value().beginStop.size() - 1},
            {beginStopPos + tagMarks.value().beginStop.size(), endPos - 1},
            {endPos, endPos + tagMarks.value().end.size() - 1}});
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
