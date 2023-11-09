#include "bvb_scraper.h"

#include <fstream>
#include <gtest/gtest.h>
#include <streambuf>

class BvbScraperTest {
public:
    tl::expected<IndexesNames, Error> ParseIndexesNames(const std::string& data)
    {
        return m_bvbScraper.ParseIndexesNames(data);
    }

private:
    BvbScraper m_bvbScraper;
};

TEST(BvbScraperTest, ParseIndexesNames)
{
    BvbScraperTest bvbTest;
    IndexesNames expected = {
        "BET",
        "BET-BK",
        "BET-FI",
        "BET-NG",
        "BET-TR",
        "BET-TRN",
        "BET-XT",
        "BET-XT-TR",
        "BET-XT-TRN",
        "BETAeRO",
        "BETPlus",
        "ROTX",
    };
    std::ifstream f("test/data/parse_indexes_names_data.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseIndexesNames(data);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value().size(), expected.size());

    for (size_t i = 0; i < expected.size(); i++) {
        ASSERT_EQ(res.value()[i], expected[i]);
    }
}
