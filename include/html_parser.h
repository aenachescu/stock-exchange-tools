#ifndef STOCK_EXCHANGE_TOOLS_HTML_PARSER_H
#define STOCK_EXCHANGE_TOOLS_HTML_PARSER_H

#include "closed_interval.h"
#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"

#include <expected.hpp>
#include <string>
#include <string_view>
#include <vector>

struct HtmlElementLocation
{
    ClosedInterval beginTag;
    ClosedInterval data;
    ClosedInterval endTag;
};

using HtmlElementLocations = std::vector<HtmlElementLocation>;

enum class HtmlTag
{
    None,
    Select,
    Option,
    Table,
    Thead,
    Tbody,
    Tr,
    Th,
    Td,
    Input,
    A,
};

enum class HtmlAttribute
{
    None,
    Id,
    Name,
};

class HtmlParser : private noncopyable, private nonmovable {
private:
    struct HtmlTagMarks
    {
        constexpr HtmlTagMarks(const char* b, const char* bs, const char* e)
            : begin(b), beginStop(bs), end(e)
        {
        }

        std::string_view begin;
        std::string_view beginStop;
        std::string_view end;
    };

    struct HtmlBeginTagPosition
    {
        constexpr HtmlBeginTagPosition(size_t bp, size_t bsp)
            : beginPos(bp), beginStopPos(bsp)
        {
        }

        size_t beginPos;
        size_t beginStopPos;
    };

public:
    HtmlParser(const std::string& htmlPage) : m_htmlPage(htmlPage)
    {
    }
    ~HtmlParser() = default;

    tl::expected<HtmlElementLocation, Error> FindElement(
        HtmlTag tag,
        ClosedInterval ci          = {},
        HtmlAttribute attr         = HtmlAttribute::None,
        std::string_view attrValue = {});

    tl::expected<HtmlElementLocations, Error> FindAllElements(
        HtmlTag tag,
        ClosedInterval ci          = {},
        HtmlAttribute attr         = HtmlAttribute::None,
        std::string_view attrValue = {});

private:
    size_t FindInInterval(std::string_view val, ClosedInterval ci);

    tl::expected<HtmlBeginTagPosition, Error> FindBeginTagMark(
        const HtmlTagMarks& tagMarks,
        ClosedInterval& ci);
    tl::expected<size_t, Error> CountBeginTagMarks(
        const HtmlTagMarks& tagMarks,
        ClosedInterval ci);

    tl::expected<HtmlTagMarks, Error> GetTagMarks(HtmlTag tag);
    tl::expected<std::string, Error> GetAttributeMark(
        HtmlAttribute attr,
        std::string_view value);

private:
    const std::string& m_htmlPage;
};

#endif // STOCK_EXCHANGE_TOOLS_HTML_PARSER_H
