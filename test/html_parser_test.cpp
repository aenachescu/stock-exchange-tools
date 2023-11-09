#include "html_parser.h"

#include <gtest/gtest.h>

TEST(HtmlParserTest, FindElement)
{
    std::string data =
        "<table>first table</table>\n"
        "<table id=\"test\"   >blah</table>\n"
        "<table   >third table</table>";
    HtmlParser htmlParser(data);
    tl::expected<HtmlElementLocation, Error> result;

    result = htmlParser.FindElement(HtmlTag::Table);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().beginTag.Lower(), 0);
    ASSERT_EQ(result.value().beginTag.Upper(), 6);
    ASSERT_EQ(result.value().data.Lower(), 7);
    ASSERT_EQ(result.value().data.Upper(), 17);
    ASSERT_EQ(result.value().endTag.Lower(), 18);
    ASSERT_EQ(result.value().endTag.Upper(), 25);
    ASSERT_EQ(
        data.substr(result.value().data.Lower(), result.value().data.Size()),
        "first table");

    result =
        htmlParser.FindElement(HtmlTag::Table, {}, HtmlAttribute::Id, "test");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().beginTag.Lower(), 27);
    ASSERT_EQ(result.value().beginTag.Upper(), 46);
    ASSERT_EQ(result.value().data.Lower(), 47);
    ASSERT_EQ(result.value().data.Upper(), 50);
    ASSERT_EQ(result.value().endTag.Lower(), 51);
    ASSERT_EQ(result.value().endTag.Upper(), 58);
    ASSERT_EQ(
        data.substr(result.value().data.Lower(), result.value().data.Size()),
        "blah");

    result =
        htmlParser.FindElement(HtmlTag::Table, {}, HtmlAttribute::Name, "test");
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::HtmlElementNotFound);

    result = htmlParser.FindElement(HtmlTag::Select);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::HtmlElementNotFound);
}

TEST(HtmlParserTest, FindMultipleElements)
{
    std::string data =
        "<table>first table</table>\n<select>\n<option>abc</option>\n"
        "   <option>def</option><option   >ghi</option> </select>";
    HtmlParser htmlParser(data);
    tl::expected<HtmlElementLocation, Error> result;
    ClosedInterval ci;

    result = htmlParser.FindElement(HtmlTag::Select);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().beginTag.Lower(), 27);
    ASSERT_EQ(result.value().beginTag.Upper(), 34);
    ASSERT_EQ(result.value().data.Lower(), 35);
    ASSERT_EQ(result.value().data.Upper(), 103);
    ASSERT_EQ(result.value().endTag.Lower(), 104);
    ASSERT_EQ(result.value().endTag.Upper(), 112);

    ci     = result.value().data;
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().beginTag.Lower(), 36);
    ASSERT_EQ(result.value().beginTag.Upper(), 43);
    ASSERT_EQ(result.value().data.Lower(), 44);
    ASSERT_EQ(result.value().data.Upper(), 46);
    ASSERT_EQ(result.value().endTag.Lower(), 47);
    ASSERT_EQ(result.value().endTag.Upper(), 55);
    ASSERT_EQ(
        data.substr(result.value().data.Lower(), result.value().data.Size()),
        "abc");

    ci.SetLower(result.value().endTag.Upper() + 1);
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().beginTag.Lower(), 60);
    ASSERT_EQ(result.value().beginTag.Upper(), 67);
    ASSERT_EQ(result.value().data.Lower(), 68);
    ASSERT_EQ(result.value().data.Upper(), 70);
    ASSERT_EQ(result.value().endTag.Lower(), 71);
    ASSERT_EQ(result.value().endTag.Upper(), 79);
    ASSERT_EQ(
        data.substr(result.value().data.Lower(), result.value().data.Size()),
        "def");

    ci.SetLower(result.value().endTag.Upper() + 1);
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().beginTag.Lower(), 80);
    ASSERT_EQ(result.value().beginTag.Upper(), 90);
    ASSERT_EQ(result.value().data.Lower(), 91);
    ASSERT_EQ(result.value().data.Upper(), 93);
    ASSERT_EQ(result.value().endTag.Lower(), 94);
    ASSERT_EQ(result.value().endTag.Upper(), 102);
    ASSERT_EQ(
        data.substr(result.value().data.Lower(), result.value().data.Size()),
        "ghi");

    ci.SetLower(result.value().endTag.Upper() + 1);
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::HtmlElementNotFound);
}

TEST(HtmlParserTest, FindElementsNested)
{
}

TEST(HtmlParserTest, FindAllElements)
{
    std::string data =
        "some data blah blah blah 1\n<select>\n<option>abc</option>\n"
        "   <option>def</option><option   >ghi</option> </select>";
    HtmlParser htmlParser(data);

    auto result = htmlParser.FindAllElements(HtmlTag::Option);
    ASSERT_TRUE(result.has_value());

    auto& loc = result.value();
    ASSERT_EQ(loc.size(), 3);

    ASSERT_EQ(loc[0].beginTag.Lower(), 36);
    ASSERT_EQ(loc[0].beginTag.Upper(), 43);
    ASSERT_EQ(loc[0].data.Lower(), 44);
    ASSERT_EQ(loc[0].data.Upper(), 46);
    ASSERT_EQ(loc[0].endTag.Lower(), 47);
    ASSERT_EQ(loc[0].endTag.Upper(), 55);
    ASSERT_EQ(data.substr(loc[0].data.Lower(), loc[0].data.Size()), "abc");

    ASSERT_EQ(loc[1].beginTag.Lower(), 60);
    ASSERT_EQ(loc[1].beginTag.Upper(), 67);
    ASSERT_EQ(loc[1].data.Lower(), 68);
    ASSERT_EQ(loc[1].data.Upper(), 70);
    ASSERT_EQ(loc[1].endTag.Lower(), 71);
    ASSERT_EQ(loc[1].endTag.Upper(), 79);
    ASSERT_EQ(data.substr(loc[1].data.Lower(), loc[1].data.Size()), "def");

    ASSERT_EQ(loc[2].beginTag.Lower(), 80);
    ASSERT_EQ(loc[2].beginTag.Upper(), 90);
    ASSERT_EQ(loc[2].data.Lower(), 91);
    ASSERT_EQ(loc[2].data.Upper(), 93);
    ASSERT_EQ(loc[2].endTag.Lower(), 94);
    ASSERT_EQ(loc[2].endTag.Upper(), 102);
    ASSERT_EQ(data.substr(loc[2].data.Lower(), loc[2].data.Size()), "ghi");

    result = htmlParser.FindAllElements(HtmlTag::Table);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::HtmlElementNotFound);
}

TEST(HtmlParserTest, FindAllElementsByAttribute)
{
    std::string data =
        "<option id=\"1\" >id_1</option>"
        "<option id=\"test\" >id_test</option>"
        "<option id=\"1\" >id_1</option>"
        "<option id=\"test\" >id_test</option>";
    HtmlParser htmlParser(data);

    auto result =
        htmlParser.FindAllElements(HtmlTag::Option, {}, HtmlAttribute::Id, "1");
    ASSERT_TRUE(result.has_value());

    auto& loc = result.value();
    ASSERT_EQ(loc.size(), 2);

    ASSERT_EQ(loc[0].beginTag.Lower(), 0);
    ASSERT_EQ(loc[0].beginTag.Upper(), 15);
    ASSERT_EQ(loc[0].data.Lower(), 16);
    ASSERT_EQ(loc[0].data.Upper(), 19);
    ASSERT_EQ(loc[0].endTag.Lower(), 20);
    ASSERT_EQ(loc[0].endTag.Upper(), 28);
    ASSERT_EQ(data.substr(loc[0].data.Lower(), loc[0].data.Size()), "id_1");

    ASSERT_EQ(loc[1].beginTag.Lower(), 64);
    ASSERT_EQ(loc[1].beginTag.Upper(), 79);
    ASSERT_EQ(loc[1].data.Lower(), 80);
    ASSERT_EQ(loc[1].data.Upper(), 83);
    ASSERT_EQ(loc[1].endTag.Lower(), 84);
    ASSERT_EQ(loc[1].endTag.Upper(), 92);
    ASSERT_EQ(data.substr(loc[1].data.Lower(), loc[1].data.Size()), "id_1");

    result = htmlParser.FindAllElements(
        HtmlTag::Option,
        {},
        HtmlAttribute::Id,
        "test");
    ASSERT_TRUE(result.has_value());

    loc = result.value();
    ASSERT_EQ(loc.size(), 2);

    ASSERT_EQ(loc[0].beginTag.Lower(), 29);
    ASSERT_EQ(loc[0].beginTag.Upper(), 47);
    ASSERT_EQ(loc[0].data.Lower(), 48);
    ASSERT_EQ(loc[0].data.Upper(), 54);
    ASSERT_EQ(loc[0].endTag.Lower(), 55);
    ASSERT_EQ(loc[0].endTag.Upper(), 63);
    ASSERT_EQ(data.substr(loc[0].data.Lower(), loc[0].data.Size()), "id_test");

    ASSERT_EQ(loc[1].beginTag.Lower(), 93);
    ASSERT_EQ(loc[1].beginTag.Upper(), 111);
    ASSERT_EQ(loc[1].data.Lower(), 112);
    ASSERT_EQ(loc[1].data.Upper(), 118);
    ASSERT_EQ(loc[1].endTag.Lower(), 119);
    ASSERT_EQ(loc[1].endTag.Upper(), 127);
    ASSERT_EQ(data.substr(loc[1].data.Lower(), loc[1].data.Size()), "id_test");
}

TEST(HtmlParserTest, FindAllElementsNested)
{
}
