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

    tl::expected<Index, Error> ParseConstituents(
        const std::string& data,
        const IndexName& indexName)
    {
        return m_bvbScraper.ParseConstituents(data, indexName);
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

TEST(BvbScraperTest, ParseConstituentsSelected)
{
    // clang-format off
    std::vector<Company> companies = {
        {"TLV", "BANCA TRANSILVANIA S.A.", 798658233ull, 24.2600, 1.00, 0.667000, 1.000000, 20.31},
        {"H2O", "S.P.E.E.H. HIDROELECTRICA S.A.", 449802567ull, 128.0000, 0.20, 1.000000, 1.000000, 18.10},
        {"SNP", "OMV PETROM S.A.", 62311667058ull, 0.5745, 0.30, 1.000000, 1.000000, 16.88},
        {"SNG", "S.N.G.N. ROMGAZ S.A.", 385422400ull, 50.1000, 0.30, 1.000000, 1.000000, 9.11},
        {"BRD", "BRD - GROUPE SOCIETE GENERALE S.A.", 696901518ull, 17.9200, 0.40, 1.000000, 1.000000, 7.85},
        {"SNN", "S.N. NUCLEARELECTRICA S.A.", 301643894ull, 49.1000, 0.20, 1.000000, 1.000000, 4.66},
        {"FP", "FONDUL PROPRIETATEA", 5668806128ull, 0.5230, 0.60, 1.000000, 1.000000, 2.80},
        {"TGN", "S.N.T.G.N. TRANSGAZ S.A.", 188381504ull, 18.8600, 0.50, 1.000000, 1.000000, 2.79},
        {"DIGI", "Digi Communications N.V.", 100000000ull, 44.0000, 0.40, 1.000000, 1.000000, 2.77},
        {"EL", "SOCIETATEA ENERGETICA ELECTRICA S.A.", 346443597ull, 11.4800, 0.40, 1.000000, 1.000000, 2.50},
        {"ONE", "ONE UNITED PROPERTIES", 3797654315ull, 0.9880, 0.40, 1.000000, 1.000000, 2.36},
        {"M", "MedLife S.A.", 531481968ull, 3.9800, 0.70, 1.000000, 1.000000, 2.33},
        {"TTS", "TTS (TRANSPORT TRADE SERVICES)", 60000000ull, 27.0000, 0.70, 1.000000, 1.000000, 1.78},
        {"TEL", "C.N.T.E.E. TRANSELECTRICA", 73303142ull, 30.1000, 0.40, 1.000000, 1.000000, 1.39},
        {"TRP", "TERAPLAST SA", 2179000358ull, 0.5150, 0.60, 1.000000, 1.000000, 1.06},
        {"BVB", "BURSA DE VALORI BUCURESTI SA", 8049246ull, 65.6000, 1.00, 1.000000, 1.000000, 0.83},
        {"WINE", "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull, 14.3400, 0.80, 1.000000, 1.000000, 0.72},
        {"AQ", "AQUILA PART PROD COM", 1200002400ull, 0.9200, 0.40, 1.000000, 1.000000, 0.69},
        {"SFG", "Sphera Franchise Group", 38799340ull, 25.7000, 0.40, 1.000000, 1.000000, 0.63},
        {"COTE", "CONPET SA", 8657528ull, 81.8000, 0.40, 1.000000, 1.000000, 0.45},
    };
    // clang-format on
    IndexName name     = "BET";
    std::string date   = "real-time";
    std::string reason = "Index Composition";
    BvbScraperTest bvbTest;
    std::ifstream f("test/data/parse_index_constituents_selected.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseConstituents(data, name);
    ASSERT_TRUE(res.has_value());

    ASSERT_EQ(res.value().name, name);
    ASSERT_EQ(res.value().date, date);
    ASSERT_EQ(res.value().reason, reason);
    ASSERT_EQ(res.value().companies.size(), companies.size());

    for (size_t i = 0; i < companies.size(); i++) {
        ASSERT_EQ(res.value().companies[i].symbol, companies[i].symbol);
        ASSERT_EQ(res.value().companies[i].name, companies[i].name);
        ASSERT_EQ(res.value().companies[i].shares, companies[i].shares);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].reference_price,
            companies[i].reference_price);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].free_float_factor,
            companies[i].free_float_factor);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].representation_factor,
            companies[i].representation_factor);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].price_correction_factor,
            companies[i].price_correction_factor);
        ASSERT_DOUBLE_EQ(res.value().companies[i].weight, companies[i].weight);
    }
}

TEST(BvbScraperTest, ParseConstituents)
{
    // clang-format off
    std::vector<Company> companies = {
        {"FP", "FONDUL PROPRIETATEA", 5668806128ull, 0.5230, 0.60, 1.000000, 1.000000, 24.85},
        {"LION", "LION CAPITAL S.A.", 507510056ull, 2.5900, 1.00, 1.000000, 1.000000, 18.36},
        {"EVER", "EVERGENT INVESTMENTS S.A.", 961753592ull, 1.2700, 1.00, 1.000000, 1.000000, 17.06},
        {"SIF4", "SIF MUNTENIA S.A.", 784645201ull, 1.5150, 1.00, 1.000000, 1.000000, 16.61},
        {"INFINITY", "INFINITY CAPITAL INVESTMENTS S.A.", 500000000ull, 1.9000, 1.00, 1.000000, 1.000000, 13.27},
        {"TRANSI", "TRANSILVANIA INVESTMENTS ALLIANCE S.A.", 2162443797ull, 0.3260, 1.00, 1.000000, 1.000000, 9.85},
    };
    // clang-format on
    IndexName name     = "BET-FI";
    std::string date   = "real-time";
    std::string reason = "Index Composition";
    BvbScraperTest bvbTest;
    std::ifstream f("test/data/parse_index_constituents.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseConstituents(data, name);
    ASSERT_TRUE(res.has_value());

    ASSERT_EQ(res.value().name, name);
    ASSERT_EQ(res.value().date, date);
    ASSERT_EQ(res.value().reason, reason);
    ASSERT_EQ(res.value().companies.size(), companies.size());

    for (size_t i = 0; i < companies.size(); i++) {
        ASSERT_EQ(res.value().companies[i].symbol, companies[i].symbol);
        ASSERT_EQ(res.value().companies[i].name, companies[i].name);
        ASSERT_EQ(res.value().companies[i].shares, companies[i].shares);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].reference_price,
            companies[i].reference_price);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].free_float_factor,
            companies[i].free_float_factor);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].representation_factor,
            companies[i].representation_factor);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].price_correction_factor,
            companies[i].price_correction_factor);
        ASSERT_DOUBLE_EQ(res.value().companies[i].weight, companies[i].weight);
    }
}
