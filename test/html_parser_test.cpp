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
    ASSERT_EQ(result->beginTag.Lower(), 0);
    ASSERT_EQ(result->beginTag.Upper(), 6);
    ASSERT_EQ(result->data.Lower(), 7);
    ASSERT_EQ(result->data.Upper(), 17);
    ASSERT_EQ(result->endTag.Lower(), 18);
    ASSERT_EQ(result->endTag.Upper(), 25);
    ASSERT_EQ(
        data.substr(result->data.Lower(), result->data.Size()),
        "first table");

    result =
        htmlParser.FindElement(HtmlTag::Table, {}, HtmlAttribute::Id, "test");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->beginTag.Lower(), 27);
    ASSERT_EQ(result->beginTag.Upper(), 46);
    ASSERT_EQ(result->data.Lower(), 47);
    ASSERT_EQ(result->data.Upper(), 50);
    ASSERT_EQ(result->endTag.Lower(), 51);
    ASSERT_EQ(result->endTag.Upper(), 58);
    ASSERT_EQ(data.substr(result->data.Lower(), result->data.Size()), "blah");

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
    ASSERT_EQ(result->beginTag.Lower(), 27);
    ASSERT_EQ(result->beginTag.Upper(), 34);
    ASSERT_EQ(result->data.Lower(), 35);
    ASSERT_EQ(result->data.Upper(), 103);
    ASSERT_EQ(result->endTag.Lower(), 104);
    ASSERT_EQ(result->endTag.Upper(), 112);

    ci     = result->data;
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->beginTag.Lower(), 36);
    ASSERT_EQ(result->beginTag.Upper(), 43);
    ASSERT_EQ(result->data.Lower(), 44);
    ASSERT_EQ(result->data.Upper(), 46);
    ASSERT_EQ(result->endTag.Lower(), 47);
    ASSERT_EQ(result->endTag.Upper(), 55);
    ASSERT_EQ(data.substr(result->data.Lower(), result->data.Size()), "abc");

    ci.SetLower(result->endTag.Upper() + 1);
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->beginTag.Lower(), 60);
    ASSERT_EQ(result->beginTag.Upper(), 67);
    ASSERT_EQ(result->data.Lower(), 68);
    ASSERT_EQ(result->data.Upper(), 70);
    ASSERT_EQ(result->endTag.Lower(), 71);
    ASSERT_EQ(result->endTag.Upper(), 79);
    ASSERT_EQ(data.substr(result->data.Lower(), result->data.Size()), "def");

    ci.SetLower(result->endTag.Upper() + 1);
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->beginTag.Lower(), 80);
    ASSERT_EQ(result->beginTag.Upper(), 90);
    ASSERT_EQ(result->data.Lower(), 91);
    ASSERT_EQ(result->data.Upper(), 93);
    ASSERT_EQ(result->endTag.Lower(), 94);
    ASSERT_EQ(result->endTag.Upper(), 102);
    ASSERT_EQ(data.substr(result->data.Lower(), result->data.Size()), "ghi");

    ci.SetLower(result->endTag.Upper() + 1);
    result = htmlParser.FindElement(HtmlTag::Option, ci);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), Error::HtmlElementNotFound);
}

TEST(HtmlParserTest, FindElementsNested)
{
    std::string data =
        "<table>"
        "  <table>"
        "    <table></table>"
        "    <table></table>"
        "  </table>"
        "  <table>"
        "    <table></table>"
        "    <table></table>"
        "  </table>"
        "</table>"
        "<table id=\"1\">qwe"
        "  <table id=\"2\">asd"
        "    <table id=\"3\"></table>"
        "    <table id=\"3\"></table>"
        "  </table>"
        "  <table id=\"2\">asd"
        "    <table id=\"3\"></table>"
        "    <table id=\"3\"></table>"
        "  </table>"
        "</table>";
    HtmlParser htmlParser(data);
    tl::expected<HtmlElementLocation, Error> result1;
    tl::expected<HtmlElementLocation, Error> result2;
    tl::expected<HtmlElementLocation, Error> result3;
    ClosedInterval ci;

    result1 = htmlParser.FindElement(HtmlTag::Table);
    ASSERT_TRUE(result1.has_value());
    ASSERT_EQ(result1->beginTag.Lower(), 0);
    ASSERT_EQ(result1->beginTag.Upper(), 6);
    ASSERT_EQ(result1->data.Lower(), 7);
    ASSERT_EQ(result1->data.Upper(), 120);
    ASSERT_EQ(result1->endTag.Lower(), 121);
    ASSERT_EQ(result1->endTag.Upper(), 128);

    result2 = htmlParser.FindElement(HtmlTag::Table, result1->data);
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2->beginTag.Lower(), 9);
    ASSERT_EQ(result2->beginTag.Upper(), 15);
    ASSERT_EQ(result2->data.Lower(), 16);
    ASSERT_EQ(result2->data.Upper(), 55);
    ASSERT_EQ(result2->endTag.Lower(), 56);
    ASSERT_EQ(result2->endTag.Upper(), 63);

    result3 = htmlParser.FindElement(HtmlTag::Table, result2->data);
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 20);
    ASSERT_EQ(result3->beginTag.Upper(), 26);
    ASSERT_EQ(result3->data.Lower(), 27);
    ASSERT_EQ(result3->data.Upper(), 26);
    ASSERT_EQ(result3->endTag.Lower(), 27);
    ASSERT_EQ(result3->endTag.Upper(), 34);

    ci.SetLower(result3->endTag.Upper() + 1);
    ci.SetUpper(result1->data.Upper());
    result3 = htmlParser.FindElement(HtmlTag::Table, ci);
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 39);
    ASSERT_EQ(result3->beginTag.Upper(), 45);
    ASSERT_EQ(result3->data.Lower(), 46);
    ASSERT_EQ(result3->data.Upper(), 45);
    ASSERT_EQ(result3->endTag.Lower(), 46);
    ASSERT_EQ(result3->endTag.Upper(), 53);

    ci.SetLower(result2->endTag.Upper() + 1);
    ci.SetUpper(result1->data.Upper());
    result2 = htmlParser.FindElement(HtmlTag::Table, ci);
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2->beginTag.Lower(), 66);
    ASSERT_EQ(result2->beginTag.Upper(), 72);
    ASSERT_EQ(result2->data.Lower(), 73);
    ASSERT_EQ(result2->data.Upper(), 112);
    ASSERT_EQ(result2->endTag.Lower(), 113);
    ASSERT_EQ(result2->endTag.Upper(), 120);

    result3 = htmlParser.FindElement(HtmlTag::Table, result2->data);
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 77);
    ASSERT_EQ(result3->beginTag.Upper(), 83);
    ASSERT_EQ(result3->data.Lower(), 84);
    ASSERT_EQ(result3->data.Upper(), 83);
    ASSERT_EQ(result3->endTag.Lower(), 84);
    ASSERT_EQ(result3->endTag.Upper(), 91);

    ci.SetLower(result3->endTag.Upper() + 1);
    ci.SetUpper(result1->data.Upper());
    result3 = htmlParser.FindElement(HtmlTag::Table, ci);
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 96);
    ASSERT_EQ(result3->beginTag.Upper(), 102);
    ASSERT_EQ(result3->data.Lower(), 103);
    ASSERT_EQ(result3->data.Upper(), 102);
    ASSERT_EQ(result3->endTag.Lower(), 103);
    ASSERT_EQ(result3->endTag.Upper(), 110);

    result1 =
        htmlParser.FindElement(HtmlTag::Table, {}, HtmlAttribute::Id, "1");
    ASSERT_TRUE(result1.has_value());
    ASSERT_EQ(result1->beginTag.Lower(), 129);
    ASSERT_EQ(result1->beginTag.Upper(), 142);
    ASSERT_EQ(result1->data.Lower(), 143);
    ASSERT_EQ(result1->data.Upper(), 307);
    ASSERT_EQ(result1->endTag.Lower(), 308);
    ASSERT_EQ(result1->endTag.Upper(), 315);

    result2 = htmlParser.FindElement(
        HtmlTag::Table,
        result1->data,
        HtmlAttribute::Id,
        "2");
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2->beginTag.Lower(), 148);
    ASSERT_EQ(result2->beginTag.Upper(), 161);
    ASSERT_EQ(result2->data.Lower(), 162);
    ASSERT_EQ(result2->data.Upper(), 218);
    ASSERT_EQ(result2->endTag.Lower(), 219);
    ASSERT_EQ(result2->endTag.Upper(), 226);

    result3 = htmlParser.FindElement(
        HtmlTag::Table,
        result2->data,
        HtmlAttribute::Id,
        "3");
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 169);
    ASSERT_EQ(result3->beginTag.Upper(), 182);
    ASSERT_EQ(result3->data.Lower(), 183);
    ASSERT_EQ(result3->data.Upper(), 182);
    ASSERT_EQ(result3->endTag.Lower(), 183);
    ASSERT_EQ(result3->endTag.Upper(), 190);

    ci.SetLower(result3->endTag.Upper() + 1);
    ci.SetUpper(result1->data.Upper());
    result3 =
        htmlParser.FindElement(HtmlTag::Table, ci, HtmlAttribute::Id, "3");
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 195);
    ASSERT_EQ(result3->beginTag.Upper(), 208);
    ASSERT_EQ(result3->data.Lower(), 209);
    ASSERT_EQ(result3->data.Upper(), 208);
    ASSERT_EQ(result3->endTag.Lower(), 209);
    ASSERT_EQ(result3->endTag.Upper(), 216);

    ci.SetLower(result2->endTag.Upper() + 1);
    ci.SetUpper(result1->data.Upper());
    result2 =
        htmlParser.FindElement(HtmlTag::Table, ci, HtmlAttribute::Id, "2");
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2->beginTag.Lower(), 229);
    ASSERT_EQ(result2->beginTag.Upper(), 242);
    ASSERT_EQ(result2->data.Lower(), 243);
    ASSERT_EQ(result2->data.Upper(), 299);
    ASSERT_EQ(result2->endTag.Lower(), 300);
    ASSERT_EQ(result2->endTag.Upper(), 307);

    result3 = htmlParser.FindElement(
        HtmlTag::Table,
        result2->data,
        HtmlAttribute::Id,
        "3");
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 250);
    ASSERT_EQ(result3->beginTag.Upper(), 263);
    ASSERT_EQ(result3->data.Lower(), 264);
    ASSERT_EQ(result3->data.Upper(), 263);
    ASSERT_EQ(result3->endTag.Lower(), 264);
    ASSERT_EQ(result3->endTag.Upper(), 271);

    ci.SetLower(result3->endTag.Upper() + 1);
    ci.SetUpper(result1->data.Upper());
    result3 =
        htmlParser.FindElement(HtmlTag::Table, ci, HtmlAttribute::Id, "3");
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->beginTag.Lower(), 276);
    ASSERT_EQ(result3->beginTag.Upper(), 289);
    ASSERT_EQ(result3->data.Lower(), 290);
    ASSERT_EQ(result3->data.Upper(), 289);
    ASSERT_EQ(result3->endTag.Lower(), 290);
    ASSERT_EQ(result3->endTag.Upper(), 297);
}

TEST(HtmlParserTest, FindElementNoEndTag)
{
    std::string data = "<input/><input id=\"test\"/>";
    HtmlParser htmlParser(data);
    tl::expected<HtmlElementLocation, Error> result;

    result = htmlParser.FindElement(HtmlTag::Input);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->beginTag.Lower(), 0);
    ASSERT_EQ(result->beginTag.Upper(), 7);
    ASSERT_EQ(result->data.Lower(), 8);
    ASSERT_EQ(result->data.Upper(), 7);
    ASSERT_EQ(result->endTag.Lower(), 8);
    ASSERT_EQ(result->endTag.Upper(), 7);

    result =
        htmlParser.FindElement(HtmlTag::Input, {}, HtmlAttribute::Id, "test");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->beginTag.Lower(), 8);
    ASSERT_EQ(result->beginTag.Upper(), 25);
    ASSERT_EQ(result->data.Lower(), 26);
    ASSERT_EQ(result->data.Upper(), 25);
    ASSERT_EQ(result->endTag.Lower(), 26);
    ASSERT_EQ(result->endTag.Upper(), 25);
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
    std::string data =
        "<table>"
        "  <table>"
        "    <table></table>"
        "    <table></table>"
        "  </table>"
        "  <table>"
        "    <table></table>"
        "    <table></table>"
        "  </table>"
        "</table>"
        "<table id=\"1\">qwe"
        "  <table id=\"2\">asd"
        "    <table id=\"3\"></table>"
        "    <table id=\"3\"></table>"
        "  </table>"
        "  <table id=\"2\">asd"
        "    <table id=\"3\"></table>"
        "    <table id=\"3\"></table>"
        "  </table>"
        "</table>";
    HtmlParser htmlParser(data);
    tl::expected<HtmlElementLocations, Error> result1;
    tl::expected<HtmlElementLocations, Error> result2;
    tl::expected<HtmlElementLocations, Error> result3;

    result1 = htmlParser.FindAllElements(HtmlTag::Table);
    ASSERT_TRUE(result1.has_value());
    ASSERT_EQ(result1->size(), 2);

    ASSERT_EQ((*result1)[0].beginTag.Lower(), 0);
    ASSERT_EQ((*result1)[0].beginTag.Upper(), 6);
    ASSERT_EQ((*result1)[0].data.Lower(), 7);
    ASSERT_EQ((*result1)[0].data.Upper(), 120);
    ASSERT_EQ((*result1)[0].endTag.Lower(), 121);
    ASSERT_EQ((*result1)[0].endTag.Upper(), 128);

    ASSERT_EQ((*result1)[1].beginTag.Lower(), 129);
    ASSERT_EQ((*result1)[1].beginTag.Upper(), 142);
    ASSERT_EQ((*result1)[1].data.Lower(), 143);
    ASSERT_EQ((*result1)[1].data.Upper(), 307);
    ASSERT_EQ((*result1)[1].endTag.Lower(), 308);
    ASSERT_EQ((*result1)[1].endTag.Upper(), 315);

    result2 =
        htmlParser.FindAllElements(HtmlTag::Table, (*result1)[0].data);
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2->size(), 2);

    ASSERT_EQ((*result2)[0].beginTag.Lower(), 9);
    ASSERT_EQ((*result2)[0].beginTag.Upper(), 15);
    ASSERT_EQ((*result2)[0].data.Lower(), 16);
    ASSERT_EQ((*result2)[0].data.Upper(), 55);
    ASSERT_EQ((*result2)[0].endTag.Lower(), 56);
    ASSERT_EQ((*result2)[0].endTag.Upper(), 63);

    ASSERT_EQ((*result2)[1].beginTag.Lower(), 66);
    ASSERT_EQ((*result2)[1].beginTag.Upper(), 72);
    ASSERT_EQ((*result2)[1].data.Lower(), 73);
    ASSERT_EQ((*result2)[1].data.Upper(), 112);
    ASSERT_EQ((*result2)[1].endTag.Lower(), 113);
    ASSERT_EQ((*result2)[1].endTag.Upper(), 120);

    result3 =
        htmlParser.FindAllElements(HtmlTag::Table, (*result2)[0].data);
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->size(), 2);

    ASSERT_EQ((*result3)[0].beginTag.Lower(), 20);
    ASSERT_EQ((*result3)[0].beginTag.Upper(), 26);
    ASSERT_EQ((*result3)[0].data.Lower(), 27);
    ASSERT_EQ((*result3)[0].data.Upper(), 26);
    ASSERT_EQ((*result3)[0].endTag.Lower(), 27);
    ASSERT_EQ((*result3)[0].endTag.Upper(), 34);

    ASSERT_EQ((*result3)[1].beginTag.Lower(), 39);
    ASSERT_EQ((*result3)[1].beginTag.Upper(), 45);
    ASSERT_EQ((*result3)[1].data.Lower(), 46);
    ASSERT_EQ((*result3)[1].data.Upper(), 45);
    ASSERT_EQ((*result3)[1].endTag.Lower(), 46);
    ASSERT_EQ((*result3)[1].endTag.Upper(), 53);

    result3 =
        htmlParser.FindAllElements(HtmlTag::Table, (*result2)[1].data);
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->size(), 2);

    ASSERT_EQ((*result3)[0].beginTag.Lower(), 77);
    ASSERT_EQ((*result3)[0].beginTag.Upper(), 83);
    ASSERT_EQ((*result3)[0].data.Lower(), 84);
    ASSERT_EQ((*result3)[0].data.Upper(), 83);
    ASSERT_EQ((*result3)[0].endTag.Lower(), 84);
    ASSERT_EQ((*result3)[0].endTag.Upper(), 91);

    ASSERT_EQ((*result3)[1].beginTag.Lower(), 96);
    ASSERT_EQ((*result3)[1].beginTag.Upper(), 102);
    ASSERT_EQ((*result3)[1].data.Lower(), 103);
    ASSERT_EQ((*result3)[1].data.Upper(), 102);
    ASSERT_EQ((*result3)[1].endTag.Lower(), 103);
    ASSERT_EQ((*result3)[1].endTag.Upper(), 110);

    result2 = htmlParser.FindAllElements(
        HtmlTag::Table,
        (*result1)[1].data,
        HtmlAttribute::Id,
        "2");
    ASSERT_TRUE(result2.has_value());
    ASSERT_EQ(result2->size(), 2);

    ASSERT_EQ((*result2)[0].beginTag.Lower(), 148);
    ASSERT_EQ((*result2)[0].beginTag.Upper(), 161);
    ASSERT_EQ((*result2)[0].data.Lower(), 162);
    ASSERT_EQ((*result2)[0].data.Upper(), 218);
    ASSERT_EQ((*result2)[0].endTag.Lower(), 219);
    ASSERT_EQ((*result2)[0].endTag.Upper(), 226);

    ASSERT_EQ((*result2)[1].beginTag.Lower(), 229);
    ASSERT_EQ((*result2)[1].beginTag.Upper(), 242);
    ASSERT_EQ((*result2)[1].data.Lower(), 243);
    ASSERT_EQ((*result2)[1].data.Upper(), 299);
    ASSERT_EQ((*result2)[1].endTag.Lower(), 300);
    ASSERT_EQ((*result2)[1].endTag.Upper(), 307);

    result3 = htmlParser.FindAllElements(
        HtmlTag::Table,
        (*result1)[1].data,
        HtmlAttribute::Id,
        "3");
    ASSERT_TRUE(result3.has_value());
    ASSERT_EQ(result3->size(), 4);

    ASSERT_EQ((*result3)[0].beginTag.Lower(), 169);
    ASSERT_EQ((*result3)[0].beginTag.Upper(), 182);
    ASSERT_EQ((*result3)[0].data.Lower(), 183);
    ASSERT_EQ((*result3)[0].data.Upper(), 182);
    ASSERT_EQ((*result3)[0].endTag.Lower(), 183);
    ASSERT_EQ((*result3)[0].endTag.Upper(), 190);

    ASSERT_EQ((*result3)[1].beginTag.Lower(), 195);
    ASSERT_EQ((*result3)[1].beginTag.Upper(), 208);
    ASSERT_EQ((*result3)[1].data.Lower(), 209);
    ASSERT_EQ((*result3)[1].data.Upper(), 208);
    ASSERT_EQ((*result3)[1].endTag.Lower(), 209);
    ASSERT_EQ((*result3)[1].endTag.Upper(), 216);

    ASSERT_EQ((*result3)[2].beginTag.Lower(), 250);
    ASSERT_EQ((*result3)[2].beginTag.Upper(), 263);
    ASSERT_EQ((*result3)[2].data.Lower(), 264);
    ASSERT_EQ((*result3)[2].data.Upper(), 263);
    ASSERT_EQ((*result3)[2].endTag.Lower(), 264);
    ASSERT_EQ((*result3)[2].endTag.Upper(), 271);

    ASSERT_EQ((*result3)[3].beginTag.Lower(), 276);
    ASSERT_EQ((*result3)[3].beginTag.Upper(), 289);
    ASSERT_EQ((*result3)[3].data.Lower(), 290);
    ASSERT_EQ((*result3)[3].data.Upper(), 289);
    ASSERT_EQ((*result3)[3].endTag.Lower(), 290);
    ASSERT_EQ((*result3)[3].endTag.Upper(), 297);
}

TEST(HtmlParserTest, FindAllElementsNoEndTag)
{
    std::string data = "<input /><input id=\"test\" />";
    HtmlParser htmlParser(data);
    tl::expected<HtmlElementLocations, Error> result;

    result = htmlParser.FindAllElements(HtmlTag::Input);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2);

    ASSERT_EQ((*result)[0].beginTag.Lower(), 0);
    ASSERT_EQ((*result)[0].beginTag.Upper(), 8);
    ASSERT_EQ((*result)[0].data.Lower(), 9);
    ASSERT_EQ((*result)[0].data.Upper(), 8);
    ASSERT_EQ((*result)[0].endTag.Lower(), 9);
    ASSERT_EQ((*result)[0].endTag.Upper(), 8);

    ASSERT_EQ((*result)[1].beginTag.Lower(), 9);
    ASSERT_EQ((*result)[1].beginTag.Upper(), 27);
    ASSERT_EQ((*result)[1].data.Lower(), 28);
    ASSERT_EQ((*result)[1].data.Upper(), 27);
    ASSERT_EQ((*result)[1].endTag.Lower(), 28);
    ASSERT_EQ((*result)[1].endTag.Upper(), 27);

    result = htmlParser.FindAllElements(
        HtmlTag::Input,
        {},
        HtmlAttribute::Id,
        "test");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);

    ASSERT_EQ((*result)[0].beginTag.Lower(), 9);
    ASSERT_EQ((*result)[0].beginTag.Upper(), 27);
    ASSERT_EQ((*result)[0].data.Lower(), 28);
    ASSERT_EQ((*result)[0].data.Upper(), 27);
    ASSERT_EQ((*result)[0].endTag.Lower(), 28);
    ASSERT_EQ((*result)[0].endTag.Upper(), 27);
}
