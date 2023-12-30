#include "bvb_scraper.h"

#include <fstream>
#include <gtest/gtest.h>
#include <streambuf>

class BvbScraperTest {
public:
    tl::expected<BvbScraper::IndexesDetails, Error> ParseIndexesNames(
        const std::string& data)
    {
        return m_bvbScraper.ParseIndexesNames(data);
    }

    tl::expected<IndexesPerformance, Error> ParseIndexesPerformance(
        const std::string& data)
    {
        return m_bvbScraper.ParseIndexesPerformance(data);
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
    IndexName selected = "BET";
    std::ifstream f("test/data/parse_indexes_names_data.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseIndexesNames(data);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value().names.size(), expected.size());

    for (size_t i = 0; i < expected.size(); i++) {
        ASSERT_EQ(res.value().names[i], expected[i]);
    }

    ASSERT_EQ(res.value().selected, selected);
}

TEST(BvbScraperTest, ParseIndexesPerformance)
{
    IndexesPerformance expected = {
        {"BET", 0.75, -0.80, 3.76, 26.23, 25.68, 31.05},
        {"BET-BK", 0.69, -0.94, 3.25, 24.49, 26.56, 30.10},
        {"BET-FI", 0.82, -0.28, -1.66, 18.41, 14.87, 16.10},
        {"BET-NG", 0.82, -1.52, 4.17, 20.07, 23.61, 30.74},
        {"BET-TR", 0.75, -0.80, 3.78, 30.61, 33.44, 39.15},
        {"BET-TRN", 0.75, -0.80, 3.78, 28.57, 31.07, 36.68},
        {"BET-XT", 0.75, -0.82, 3.34, 24.74, 24.39, 29.22},
        {"BET-XT-TR", 0.75, -0.82, 3.36, 28.48, 31.39, 36.49},
        {"BET-XT-TRN", 0.75, -0.82, 3.36, 26.64, 29.23, 34.24},
        {"BETAeRO", -0.01, -0.30, 2.70, 13.03, 18.02, 18.55},
        {"BETPlus", 0.70, -0.75, 3.60, 25.58, 24.75, 29.67},
        {"ROTX", 0.71, -0.43, 2.58, 25.51, 24.54, 30.47},
    };
    BvbScraperTest bvbTest;
    std::ifstream f("test/data/parse_indexes_performance_data.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseIndexesPerformance(data);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value().size(), expected.size());

    for (size_t i = 0; i < expected.size(); i++) {
        ASSERT_EQ(res.value()[i].name, expected[i].name);
        ASSERT_DOUBLE_EQ(res.value()[i].today, expected[i].today);
        ASSERT_DOUBLE_EQ(res.value()[i].one_week, expected[i].one_week);
        ASSERT_DOUBLE_EQ(res.value()[i].one_month, expected[i].one_month);
        ASSERT_DOUBLE_EQ(res.value()[i].six_months, expected[i].six_months);
        ASSERT_DOUBLE_EQ(res.value()[i].one_year, expected[i].one_year);
        ASSERT_DOUBLE_EQ(res.value()[i].year_to_date, expected[i].year_to_date);
    }
}
