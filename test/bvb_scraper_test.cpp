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

    tl::expected<Indexes, Error> ParseAdjustmentsHistory(
        const std::string& data,
        const IndexName& indexName)
    {
        return m_bvbScraper.ParseAdjustmentsHistory(data, indexName);
    }

    tl::expected<IndexTradingData, Error> ParseTradingData(
        const std::string& data,
        const IndexName& indexName)
    {
        return m_bvbScraper.ParseTradingData(data, indexName);
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
        {"TLV", "BANCA TRANSILVANIA S.A.", 798658233ull, 24.2600, 1.00, 0.667000, 1.000000, 1.000000, 20.31},
        {"H2O", "S.P.E.E.H. HIDROELECTRICA S.A.", 449802567ull, 128.0000, 0.20, 1.000000, 1.000000, 1.000000, 18.10},
        {"SNP", "OMV PETROM S.A.", 62311667058ull, 0.5745, 0.30, 1.000000, 1.000000, 1.000000, 16.88},
        {"SNG", "S.N.G.N. ROMGAZ S.A.", 385422400ull, 50.1000, 0.30, 1.000000, 1.000000, 1.000000, 9.11},
        {"BRD", "BRD - GROUPE SOCIETE GENERALE S.A.", 696901518ull, 17.9200, 0.40, 1.000000, 1.000000, 1.000000, 7.85},
        {"SNN", "S.N. NUCLEARELECTRICA S.A.", 301643894ull, 49.1000, 0.20, 1.000000, 1.000000, 1.000000, 4.66},
        {"FP", "FONDUL PROPRIETATEA", 5668806128ull, 0.5230, 0.60, 1.000000, 1.000000, 1.000000, 2.80},
        {"TGN", "S.N.T.G.N. TRANSGAZ S.A.", 188381504ull, 18.8600, 0.50, 1.000000, 1.000000, 1.000000, 2.79},
        {"DIGI", "Digi Communications N.V.", 100000000ull, 44.0000, 0.40, 1.000000, 1.000000, 1.000000, 2.77},
        {"EL", "SOCIETATEA ENERGETICA ELECTRICA S.A.", 346443597ull, 11.4800, 0.40, 1.000000, 1.000000, 1.000000, 2.50},
        {"ONE", "ONE UNITED PROPERTIES", 3797654315ull, 0.9880, 0.40, 1.000000, 1.000000, 1.000000, 2.36},
        {"M", "MedLife S.A.", 531481968ull, 3.9800, 0.70, 1.000000, 1.000000, 1.000000, 2.33},
        {"TTS", "TTS (TRANSPORT TRADE SERVICES)", 60000000ull, 27.0000, 0.70, 1.000000, 1.000000, 1.000000, 1.78},
        {"TEL", "C.N.T.E.E. TRANSELECTRICA", 73303142ull, 30.1000, 0.40, 1.000000, 1.000000, 1.000000, 1.39},
        {"TRP", "TERAPLAST SA", 2179000358ull, 0.5150, 0.60, 1.000000, 1.000000, 1.000000, 1.06},
        {"BVB", "BURSA DE VALORI BUCURESTI SA", 8049246ull, 65.6000, 1.00, 1.000000, 1.000000, 1.000000, 0.83},
        {"WINE", "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull, 14.3400, 0.80, 1.000000, 1.000000, 1.000000, 0.72},
        {"AQ", "AQUILA PART PROD COM", 1200002400ull, 0.9200, 0.40, 1.000000, 1.000000, 1.000000, 0.69},
        {"SFG", "Sphera Franchise Group", 38799340ull, 25.7000, 0.40, 1.000000, 1.000000, 1.000000, 0.63},
        {"COTE", "CONPET SA", 8657528ull, 81.8000, 0.40, 1.000000, 1.000000, 1.000000, 0.45},
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
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].liquidity_factor,
            companies[i].liquidity_factor);
        ASSERT_DOUBLE_EQ(res.value().companies[i].weight, companies[i].weight);
    }
}

TEST(BvbScraperTest, ParseConstituents)
{
    // clang-format off
    std::vector<Company> companies = {
        {"EBS", "Erste Group Bank AG", 429800000ull, 200.0000, 0.80, 0.026209, 1.000000, 0.75, 7.56},
        {"TLV", "BANCA TRANSILVANIA S.A.", 798658233ull, 24.8800, 1.00, 0.066353, 1.000000, 1.00, 7.38},
        {"SNP", "OMV PETROM S.A.", 62311667058ull, 0.5780, 0.30, 0.118152, 1.000000, 1.00, 7.14},
        {"H2O", "S.P.E.E.H. HIDROELECTRICA S.A.", 449802567ull, 129.1000, 0.20, 0.108773, 1.000000, 1.00, 7.07},
        {"EL", "SOCIETATEA ENERGETICA ELECTRICA S.A.", 346443597ull, 11.5800, 0.40, 0.550014, 1.000000, 1.00, 4.94},
        {"DIGI", "Digi Communications N.V.", 100000000ull, 45.6000, 0.40, 0.637891, 1.000000, 0.75, 4.88},
        {"FP", "FONDUL PROPRIETATEA", 5668806128ull, 0.5290, 0.60, 0.478666, 1.000000, 1.00, 4.82},
        {"TTS", "TTS (TRANSPORT TRADE SERVICES)", 60000000ull, 29.0000, 0.70, 0.942774, 1.000000, 0.75, 4.82},
        {"SNG", "S.N.G.N. ROMGAZ S.A.", 385422400ull, 52.7000, 0.30, 0.139110, 1.000000, 1.00, 4.74},
        {"ONE", "ONE UNITED PROPERTIES", 3797654315ull, 0.9970, 0.40, 0.733118, 1.000000, 0.75, 4.66},
        {"BRD", "BRD - GROUPE SOCIETE GENERALE S.A.", 696901518ull, 18.0800, 0.40, 0.164554, 1.000000, 1.00, 4.64},
        {"TGN", "S.N.T.G.N. TRANSGAZ S.A.", 188381504ull, 19.2800, 0.50, 0.442915, 1.000000, 1.00, 4.50},
        {"SNN", "S.N. NUCLEARELECTRICA S.A.", 301643894ull, 49.1500, 0.20, 0.269868, 1.000000, 1.00, 4.48},
        {"M", "MedLife S.A.", 531481968ull, 3.9800, 0.70, 0.684203, 1.000000, 0.75, 4.25},
        {"TEL", "C.N.T.E.E. TRANSELECTRICA", 73303142ull, 29.7000, 0.40, 1.000000, 1.000000, 0.75, 3.65},
        {"TRP", "TERAPLAST SA", 2179000358ull, 0.5950, 0.60, 1.000000, 1.000000, 0.75, 3.26},
        {"EVER", "EVERGENT INVESTMENTS S.A.", 961753592ull, 1.1700, 1.00, 1.000000, 1.000000, 0.50, 3.15},
        {"INFINITY", "INFINITY CAPITAL INVESTMENTS S.A.", 500000000ull, 1.8150, 1.00, 1.000000, 1.000000, 0.50, 2.54},
        {"WINE", "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull, 15.4000, 0.80, 1.000000, 1.000000, 0.75, 2.07},
        {"LION", "LION CAPITAL S.A.", 507510056ull, 2.5500, 1.00, 1.000000, 1.000000, 0.25, 1.81},
        {"BVB", "BURSA DE VALORI BUCURESTI SA", 8049246ull, 66.8000, 1.00, 1.000000, 1.000000, 0.50, 1.50},
        {"AQ", "AQUILA PART PROD COM", 1200002400ull, 1.1150, 0.40, 1.000000, 1.000000, 0.50, 1.50},
        {"SFG", "Sphera Franchise Group", 38799340ull, 26.8000, 0.40, 1.000000, 1.000000, 0.50, 1.16},
        {"TRANSI", "TRANSILVANIA INVESTMENTS ALLIANCE S.A.", 2162443797ull, 0.2990, 1.00, 1.000000, 1.000000, 0.25, 0.90},
        {"ATB", "ANTIBIOTICE S.A.", 671338040ull, 1.8200, 0.50, 1.000000, 1.000000, 0.25, 0.85},
        {"COTE", "CONPET SA", 8657528ull, 85.2000, 0.40, 1.000000, 1.000000, 0.50, 0.83},
        {"ALR", "ALRO S.A.", 713779135ull, 1.4800, 0.30, 1.000000, 1.000000, 0.25, 0.44},
        {"BNET", "BITTNET SYSTEMS SA BUCURESTI", 634176714ull, 0.2760, 0.50, 1.000000, 1.000000, 0.50, 0.24},
        {"ROCE", "ROMCARBON SA", 528244192ull, 0.2000, 0.40, 1.000000, 1.000000, 0.50, 0.12},
        {"BRK", "SSIF BRK FINANCIAL GROUP SA", 337429952ull, 0.1455, 0.90, 1.000000, 1.000000, 0.25, 0.06},
    };
    // clang-format on
    IndexName name     = "BET-BK";
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
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].liquidity_factor,
            companies[i].liquidity_factor);
        ASSERT_DOUBLE_EQ(res.value().companies[i].weight, companies[i].weight);
    }
}

TEST(BvbScraperTest, ParseTradingData)
{
    // clang-format off
    std::vector<CompanyTradingData> companies = {
        {"TRANSI", 0.2990, -1.32, 81ull, 1361693ull, 409608.49, 0.2980, 0.3050, 9.30},
        {"SIF4", 1.5000, -0.66, 21ull, 4550ull, 6788.40, 1.4900, 1.5150, 16.94},
        {"LION", 2.5500, 2.00, 28ull, 596495ull, 1521062.66, 2.5000, 2.5600, 18.62},
        {"INFINITY", 1.8150, -0.27, 29ull, 49401ull, 89820.50, 1.8100, 1.8200, 13.06},
        {"FP", 0.5290, 0.00, 172ull, 873048ull, 461295.39, 0.5270, 0.5290, 25.89},
        {"EVER", 1.1700, 0.00, 55ull, 168957ull, 197548.59, 1.1650, 1.1700, 16.19},
    };
    // clang-format on
    IndexName name   = "BET-FI";
    std::string date = "2/6/2024 6:00:01 PM";
    BvbScraperTest bvbTest;
    std::ifstream f("test/data/parse_index_trading_data.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseTradingData(data, name);
    ASSERT_TRUE(res.has_value());

    ASSERT_EQ(res.value().name, name);
    ASSERT_EQ(res.value().date, date);
    ASSERT_EQ(res.value().companies.size(), companies.size());

    for (size_t i = 0; i < companies.size(); i++) {
        ASSERT_EQ(res.value().companies[i].symbol, companies[i].symbol);
        ASSERT_DOUBLE_EQ(res.value().companies[i].price, companies[i].price);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].variation,
            companies[i].variation);
        ASSERT_EQ(res.value().companies[i].trades, companies[i].trades);
        ASSERT_EQ(res.value().companies[i].volume, companies[i].volume);
        ASSERT_DOUBLE_EQ(res.value().companies[i].value, companies[i].value);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].lowest_price,
            companies[i].lowest_price);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].highest_price,
            companies[i].highest_price);
        ASSERT_DOUBLE_EQ(res.value().companies[i].weight, companies[i].weight);
    }
}

TEST(BvbScraperTest, ParseTradingDataWithMissingFields)
{
    // clang-format off
    std::vector<CompanyTradingData> companies = {
        {"UARG", 1.9800, -1.00, 4ull, 840ull, 1683.20, 1.9800, 2.0200, 0.73},
        {"SPX", 0.3080, -0.65, 23ull, 100052ull, 29988.52, 0.2940, 0.3080, 1.18},
        {"SMTL", 62.0000, 6.90, 227ull, 15910ull, 970682.00, 59.0000, 64.0000, 9.99},
        {"ROC1", 8.9400, -0.22, 3ull, 571ull, 5084.74, 8.9000, 8.9400, 6.56},
        {"REIT", 0.1380, -0.72, 7ull, 12150ull, 1677.20, 0.1380, 0.1390, 0.54},
        {"PRSN", 0.2240, 0.45, 24ull, 104075ull, 23026.06, 0.2180, 0.2240, 5.98},
        {"NRF", 4.2400, -1.40, 15ull, 3725ull, 15799.97, 4.2200, 4.2900, 2.89},
        {"MILK", 6.7600, 4.64, 47ull, 7893ull, 53179.64, 6.5000, 6.9800, 2.57},
        {"MET", 0.7450, -1.97, 23ull, 125496ull, 91681.02, 0.7200, 0.7500, 5.50},
        {"MAMA", 0.2740, 3.79, 4ull, 20569ull, 5436.69, 0.2640, 0.2760, 1.31},
        {"MAM", 2.3600, -3.28, 6ull, 1635ull, 3917.52, 2.3600, 2.4400, 0.42},
        {"MACO", 18.4000, 0.55, 5ull, 250ull, 4560.00, 18.1000, 18.4000, 3.72},
        {"LIH", 2.1400, -1.83, 11ull, 13481ull, 28922.15, 2.1400, 2.1500, 2.05},
        {"IPRU", 0.9200, 0.00, 5ull, 7580ull, 6978.60, 0.9200, 0.9250, 3.02},
        {"HUNT", 0.5920, 0.00, 19ull, 22859ull, 13409.53, 0.5760, 0.5920, 1.59},
        {"HAI", 1.0100, 0.00, 9ull, 2105ull, 2112.68, 0.9960, 1.0100, 8.14},
        {"GSH", 1.6100, 3.87, 8ull, 48887ull, 74904.95, 1.5300, 1.6200, 2.77},
        {"FRB", 0.2230, 3.72, 7ull, 8542ull, 1901.89, 0.2170, 0.2250, 0.57},
        {"FOJE", 14.0000, 2.19, 7ull, 341ull, 4771.30, 13.9000, 14.0000, 3.04},
        {"ELZY", 14.3000, 5.15, 1ull, 10ull, 143.00, 14.3000, 14.3000, 3.38},
        {"DN", 1.4200, 2.53, 54ull, 246732ull, 343424.65, 1.3800, 1.4200, 6.21},
        {"CODE", 2.3100, 0.43, 15ull, 3462ull, 7916.80, 2.2600, 2.3100, 1.50},
        {"CLAIM", 4.3600, 1.87, 3ull, 566ull, 2462.76, 4.3400, 4.3600, 0.52},
        {"CHRD", 18.2000, 0.55, 1ull, 50ull, 910.00, 18.2000, 18.2000, 0.34},
        {"CC", 6.2500, 0.81, 12ull, 737ull, 4672.90, 6.2500, 6.4000, 2.13},
        {"CACU", 31.8000, 0.00, 0ull, 0ull, 0.00, 0.0000, 0.0000, 0.68},
        {"BRNA", 82.5000, 5.10, 8ull, 70ull, 5714.00, 81.0000, 82.5000, 3.45},
        {"BONA", 1.3400, -4.29, 8ull, 4788ull, 6425.65, 1.3300, 1.3700, 0.88},
        {"BIOW", 0.1640, 0.00, 5ull, 1960ull, 317.28, 0.1570, 0.1640, 0.31},
        {"BENTO", 11.5500, 0.43, 17ull, 2688ull, 30957.90, 11.4000, 11.6500, 4.60},
        {"AST", 31.2000, 0.00, 11ull, 490ull, 15283.00, 31.0000, 31.3000, 3.71},
        {"ASC", 4.5000, 3.21, 23ull, 4708ull, 21157.40, 4.4600, 4.5100, 1.72},
        {"ALW", 12.6500, -0.39, 12ull, 1063ull, 13429.15, 12.4500, 12.6500, 1.18},
        {"AG", 1.2300, 0.00, 19ull, 10060ull, 12237.69, 1.2100, 1.2300, 4.37},
        {"ADISS", 0.8800, -1.12, 5ull, 730ull, 613.60, 0.8400, 0.8800, 0.37},
        {"AAB", 4.2600, -0.93, 13ull, 3200ull, 12997.08, 4.0200, 4.2600, 0.48},
        {"2P", 2.5500, 2.00, 6ull, 2019ull, 5136.81, 2.5000, 2.5500, 1.61},
    };
    // clang-format on
    IndexName name   = "BETAero";
    std::string date = "2/6/2024 6:00:01 PM";
    BvbScraperTest bvbTest;
    std::ifstream f(
        "test/data/parse_index_trading_data_with_missing_fields.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseTradingData(data, name);
    ASSERT_TRUE(res.has_value());

    ASSERT_EQ(res.value().name, name);
    ASSERT_EQ(res.value().date, date);
    ASSERT_EQ(res.value().companies.size(), companies.size());

    for (size_t i = 0; i < companies.size(); i++) {
        ASSERT_EQ(res.value().companies[i].symbol, companies[i].symbol);
        ASSERT_DOUBLE_EQ(res.value().companies[i].price, companies[i].price);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].variation,
            companies[i].variation);
        ASSERT_EQ(res.value().companies[i].trades, companies[i].trades);
        ASSERT_EQ(res.value().companies[i].volume, companies[i].volume);
        ASSERT_DOUBLE_EQ(res.value().companies[i].value, companies[i].value);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].lowest_price,
            companies[i].lowest_price);
        ASSERT_DOUBLE_EQ(
            res.value().companies[i].highest_price,
            companies[i].highest_price);
        ASSERT_DOUBLE_EQ(res.value().companies[i].weight, companies[i].weight);
    }
}

TEST(BvbScraperTest, ParseAdjustmentsHistorySelected)
{
    // clang-format off
    Indexes expectedIndexes = {
        {
            "BET", "12/8/2023", "Periodical adjustment",
            {
                {"TLV",  "BANCA TRANSILVANIA S.A.",                 798658233ull,   23.3400,  1.00, 0.667000, 1.000000, 1.00, 19.99},
                {"H2O",  "S.P.E.E.H. HIDROELECTRICA S.A.",          449802567ull,   126.4000, 0.20, 1.000000, 1.000000, 1.00, 18.28},
                {"SNP",  "OMV PETROM S.A.",                         62311667058ull, 0.5600,   0.30, 1.000000, 1.000000, 1.00, 16.83},
                {"SNG",  "S.N.G.N. ROMGAZ S.A.",                    385422400ull,   49.8500,  0.30, 1.000000, 1.000000, 1.00, 9.26},
                {"BRD",  "BRD - GROUPE SOCIETE GENERALE S.A.",      696901518ull,   17.4800,  0.40, 1.000000, 1.000000, 1.00, 7.83},
                {"SNN",  "S.N. NUCLEARELECTRICA S.A.",              301643894ull,   49.2500,  0.20, 1.000000, 1.000000, 1.00, 4.78},
                {"TGN",  "S.N.T.G.N. TRANSGAZ S.A.",                188381504ull,   19.2200,  0.50, 1.000000, 1.000000, 1.00, 2.91},
                {"DIGI", "Digi Communications N.V.",                100000000ull,   41.9000,  0.40, 1.000000, 1.000000, 1.00, 2.69},
                {"FP",   "FONDUL PROPRIETATEA",                     5668806128ull,  0.4925,   0.60, 1.000000, 1.000000, 1.00, 2.69},
                {"M",    "MedLife S.A.",                            531481968ull,   4.2000,   0.70, 1.000000, 1.000000, 1.00, 2.51},
                {"ONE",  "ONE UNITED PROPERTIES",                   3797654315ull,  0.9600,   0.40, 1.000000, 1.000000, 1.00, 2.34},
                {"EL",   "SOCIETATEA ENERGETICA ELECTRICA S.A.",    346443597ull,   10.5200,  0.40, 1.000000, 1.000000, 1.00, 2.34},
                {"TTS",  "TTS (TRANSPORT TRADE SERVICES)",          60000000ull,    27.0000,  0.70, 1.000000, 1.000000, 1.00, 1.82},
                {"TEL",  "C.N.T.E.E. TRANSELECTRICA",               73303142ull,    29.5000,  0.40, 1.000000, 1.000000, 1.00, 1.39},
                {"TRP",  "TERAPLAST SA",                            2179000358ull,  0.4800,   0.60, 1.000000, 1.000000, 1.00, 1.01},
                {"BVB",  "BURSA DE VALORI BUCURESTI SA",            8049246ull,     68.0000,  1.00, 1.000000, 1.000000, 1.00, 0.88},
                {"WINE", "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull,    13.9000,  0.80, 1.000000, 1.000000, 1.00, 0.72},
                {"AQ",   "AQUILA PART PROD COM",                    1200002400ull,  0.8900,   0.40, 1.000000, 1.000000, 1.00, 0.69},
                {"SFG",  "Sphera Franchise Group",                  38799340ull,    23.7000,  0.40, 1.000000, 1.000000, 1.00, 0.59},
                {"COTE", "CONPET SA",                               8657528ull,     80.0000,  0.40, 1.000000, 1.000000, 1.00, 0.45},
            }
        },
        {
            "BET", "9/8/2023", "Periodical adjustment",
            {
                {"TLV",  "BANCA TRANSILVANIA S.A.",                 798658233ull,   21.9000,  1.00, 0.658000, 1.000000, 1.00, 19.99},
                {"SNP",  "OMV PETROM S.A.",                         62311667058ull, 0.5640,   0.30, 1.000000, 1.000000, 1.00, 18.31},
                {"H2O",  "S.P.E.E.H. HIDROELECTRICA S.A.",          449802567ull,   113.8000, 0.20, 1.000000, 1.000000, 1.00, 17.78},
                {"SNG",  "S.N.G.N. ROMGAZ S.A.",                    385422400ull,   42.1000,  0.30, 1.000000, 1.000000, 1.00, 8.45},
                {"BRD",  "BRD - GROUPE SOCIETE GENERALE S.A.",      696901518ull,   14.6600,  0.40, 1.000000, 1.000000, 1.00, 7.10},
                {"SNN",  "S.N. NUCLEARELECTRICA S.A.",              301643894ull,   46.9500,  0.20, 1.000000, 1.000000, 1.00, 4.92},
                {"FP",   "FONDUL PROPRIETATEA",                     6217825213ull,  0.3860,   0.80, 1.000000, 1.000000, 1.00, 3.33},
                {"M",    "MedLife S.A.",                            531481968ull,   4.5450,   0.70, 1.000000, 1.000000, 1.00, 2.94},
                {"TGN",  "S.N.T.G.N. TRANSGAZ S.A.",                188381504ull,   17.7400,  0.50, 1.000000, 1.000000, 1.00, 2.90},
                {"DIGI", "Digi Communications N.V.",                100000000ull,   35.9000,  0.40, 1.000000, 1.000000, 1.00, 2.49},
                {"ONE",  "ONE UNITED PROPERTIES",                   3797654315ull,  0.9180,   0.40, 1.000000, 1.000000, 1.00, 2.42},
                {"EL",   "SOCIETATEA ENERGETICA ELECTRICA S.A.",    346443597ull,   9.5100,   0.40, 1.000000, 1.000000, 1.00, 2.29},
                {"TTS",  "TTS (TRANSPORT TRADE SERVICES)",          60000000ull,    20.2000,  0.70, 1.000000, 1.000000, 1.00, 1.47},
                {"TEL",  "C.N.T.E.E. TRANSELECTRICA",               73303142ull,    27.0000,  0.40, 1.000000, 1.000000, 1.00, 1.37},
                {"TRP",  "TERAPLAST SA",                            2179000358ull,  0.4700,   0.60, 1.000000, 1.000000, 1.00, 1.07},
                {"AQ",   "AQUILA PART PROD COM",                    1200002400ull,  0.8880,   0.40, 1.000000, 1.000000, 1.00, 0.74},
                {"BVB",  "BURSA DE VALORI BUCURESTI SA",            8049246ull,     52.2000,  1.00, 1.000000, 1.000000, 1.00, 0.73},
                {"WINE", "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull,    12.2000,  0.80, 1.000000, 1.000000, 1.00, 0.68},
                {"SFG",  "Sphera Franchise Group",                  38799340ull,    20.6000,  0.40, 1.000000, 1.000000, 1.00, 0.56},
                {"COTE", "CONPET SA",                               8657528ull,     74.4000,  0.40, 1.000000, 1.000000, 1.00, 0.45},
            }
        },
        {
            "BET", "9/6/2023", "Operational adjustment FP",
            {
                {"TLV",  "BANCA TRANSILVANIA S.A.",                 707658233ull,   21.6000,  1.00, 0.856000, 1.128593, 1.00, 24.83},
                {"SNP",  "OMV PETROM S.A.",                         62311667058ull, 0.5555,   0.30, 1.000000, 1.000000, 1.00, 17.46},
                {"H2O",  "S.P.E.E.H. HIDROELECTRICA S.A.",          449802567ull,   113.7000, 0.20, 1.000000, 1.000000, 1.00, 17.20},
                {"SNG",  "S.N.G.N. ROMGAZ S.A.",                    385422400ull,   41.6500,  0.30, 1.000000, 1.000000, 1.00, 8.10},
                {"BRD",  "BRD - GROUPE SOCIETE GENERALE S.A.",      696901518ull,   14.4800,  0.40, 1.000000, 1.000000, 1.00, 6.79},
                {"SNN",  "S.N. NUCLEARELECTRICA S.A.",              301643894ull,   45.9000,  0.20, 1.000000, 1.000000, 1.00, 4.66},
                {"M",    "MedLife S.A.",                            132870492ull,   4.5450,   0.70, 1.000000, 4.000000, 1.00, 2.84},
                {"TGN",  "S.N.T.G.N. TRANSGAZ S.A.",                188381504ull,   16.9600,  0.50, 1.000000, 1.000000, 1.00, 2.69},
                {"FP",   "FONDUL PROPRIETATEA",                     6217825213ull,  0.2615,   0.90, 1.000000, 1.000000, 1.00, 2.46},
                {"DIGI", "Digi Communications N.V.",                100000000ull,   35.9000,  0.40, 1.000000, 1.000000, 1.00, 2.41},
                {"EL",   "SOCIETATEA ENERGETICA ELECTRICA S.A.",    346443597ull,   9.4000,   0.40, 1.000000, 1.000000, 1.00, 2.19},
                {"ONE",  "ONE UNITED PROPERTIES",                   3702818586ull,  0.9050,   0.30, 1.000000, 1.000000, 1.00, 1.69},
                {"TTS",  "TTS (TRANSPORT TRADE SERVICES)",          60000000ull,    19.3500,  0.70, 1.000000, 1.000000, 1.00, 1.37},
                {"TEL",  "C.N.T.E.E. TRANSELECTRICA",               73303142ull,    26.4000,  0.40, 1.000000, 1.000000, 1.00, 1.30},
                {"TRP",  "TERAPLAST SA",                            2179000358ull,  0.4640,   0.60, 1.000000, 1.000000, 1.00, 1.02},
                {"BVB",  "BURSA DE VALORI BUCURESTI SA",            8049246ull,     51.8000,  1.00, 1.000000, 1.000000, 1.00, 0.70},
                {"AQ",   "AQUILA PART PROD COM",                    1200002400ull,  0.8680,   0.40, 1.000000, 1.000000, 1.00, 0.70},
                {"WINE", "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull,    11.8000,  0.80, 1.000000, 1.000000, 1.00, 0.64},
                {"SFG",  "Sphera Franchise Group",                  38799340ull,    20.4000,  0.40, 1.000000, 1.000000, 1.00, 0.53},
                {"COTE", "CONPET SA",                               8657528ull,     73.0000,  0.40, 1.000000, 1.000000, 1.00, 0.43},
            }
        },
    };
    // clang-format on
    IndexName name = "BET";
    BvbScraperTest bvbTest;
    std::ifstream f("test/data/parse_index_adjustments_history_selected.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseAdjustmentsHistory(data, name);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value().size(), expectedIndexes.size());

    for (size_t i = 0; i < expectedIndexes.size(); i++) {
        const auto& index = res.value()[i];

        ASSERT_EQ(index.name, expectedIndexes[i].name);
        ASSERT_EQ(index.date, expectedIndexes[i].date);
        ASSERT_EQ(index.reason, expectedIndexes[i].reason);
        ASSERT_EQ(index.companies.size(), expectedIndexes[i].companies.size());

        for (size_t j = 0; j < index.companies.size(); j++) {
            ASSERT_EQ(
                index.companies[j].symbol,
                expectedIndexes[i].companies[j].symbol);
            ASSERT_EQ(
                index.companies[j].name,
                expectedIndexes[i].companies[j].name);
            ASSERT_EQ(
                index.companies[j].shares,
                expectedIndexes[i].companies[j].shares);
            ASSERT_DOUBLE_EQ(
                index.companies[j].reference_price,
                expectedIndexes[i].companies[j].reference_price);
            ASSERT_DOUBLE_EQ(
                index.companies[j].free_float_factor,
                expectedIndexes[i].companies[j].free_float_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].representation_factor,
                expectedIndexes[i].companies[j].representation_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].price_correction_factor,
                expectedIndexes[i].companies[j].price_correction_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].liquidity_factor,
                expectedIndexes[i].companies[j].liquidity_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].weight,
                expectedIndexes[i].companies[j].weight);
        }
    }
}

TEST(BvbScraperTest, ParseAdjustmentsHistory)
{
    // clang-format off
    Indexes expectedIndexes = {
        {
            "BET-BK", "12/8/2023", "Periodical adjustment",
            {
                {"TLV",      "BANCA TRANSILVANIA S.A.",                 798658233ull,   23.3400,  1.00, 0.066353, 1.000000, 1.00, 7.25},
                {"H2O",      "S.P.E.E.H. HIDROELECTRICA S.A.",          449802567ull,   126.4000, 0.20, 0.108773, 1.000000, 1.00, 7.25},
                {"SNP",      "OMV PETROM S.A.",                         62311667058ull, 0.5600,   0.30, 0.118152, 1.000000, 1.00, 7.25},
                {"EBS",      "Erste Group Bank AG",                     429800000ull,   183.0000, 0.80, 0.026209, 1.000000, 0.75, 7.25},
                {"SNN",      "S.N. NUCLEARELECTRICA S.A.",              301643894ull,   49.2500,  0.20, 0.269868, 1.000000, 1.00, 4.70},
                {"M",        "MedLife S.A.",                            531481968ull,   4.2000,   0.70, 0.684203, 1.000000, 0.75, 4.70},
                {"TGN",      "S.N.T.G.N. TRANSGAZ S.A.",                188381504ull,   19.2200,  0.50, 0.442915, 1.000000, 1.00, 4.70},
                {"EL",       "SOCIETATEA ENERGETICA ELECTRICA S.A.",    346443597ull,   10.5200,  0.40, 0.550014, 1.000000, 1.00, 4.70},
                {"TTS",      "TTS (TRANSPORT TRADE SERVICES)",          60000000ull,    27.0000,  0.70, 0.942774, 1.000000, 0.75, 4.70},
                {"ONE",      "ONE UNITED PROPERTIES",                   3797654315ull,  0.9600,   0.40, 0.733118, 1.000000, 0.75, 4.70},
                {"DIGI",     "Digi Communications N.V.",                100000000ull,   41.9000,  0.40, 0.637891, 1.000000, 0.75, 4.70},
                {"SNG",      "S.N.G.N. ROMGAZ S.A.",                    385422400ull,   49.8500,  0.30, 0.139110, 1.000000, 1.00, 4.70},
                {"FP",       "FONDUL PROPRIETATEA",                     5668806128ull,  0.4925,   0.60, 0.478666, 1.000000, 1.00, 4.70},
                {"BRD",      "BRD - GROUPE SOCIETE GENERALE S.A.",      696901518ull,   17.4800,  0.40, 0.164554, 1.000000, 1.00, 4.70},
                {"TEL",      "C.N.T.E.E. TRANSELECTRICA",               73303142ull,    29.5000,  0.40, 1.000000, 1.000000, 0.75, 3.80},
                {"EVER",     "EVERGENT INVESTMENTS S.A.",               961753592ull,   1.2750,   1.00, 1.000000, 1.000000, 0.50, 3.59},
                {"TRP",      "TERAPLAST SA",                            2179000358ull,  0.4800,   0.60, 1.000000, 1.000000, 0.75, 2.76},
                {"INFINITY", "INFINITY CAPITAL INVESTMENTS S.A.",       500000000ull,   1.8300,   1.00, 1.000000, 1.000000, 0.50, 2.68},
                {"WINE",     "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull,    13.9000,  0.80, 1.000000, 1.000000, 0.75, 1.96},
                {"LION",     "LION CAPITAL S.A.",                       507510056ull,   2.4900,   1.00, 1.000000, 1.000000, 0.25, 1.85},
                {"BVB",      "BURSA DE VALORI BUCURESTI SA",            8049246ull,     68.0000,  1.00, 1.000000, 1.000000, 0.50, 1.60},
                {"AQ",       "AQUILA PART PROD COM",                    1200002400ull,  0.8900,   0.40, 1.000000, 1.000000, 0.50, 1.25},
                {"SFG",      "Sphera Franchise Group",                  38799340ull,    23.7000,  0.40, 1.000000, 1.000000, 0.50, 1.08},
                {"TRANSI",   "TRANSILVANIA INVESTMENTS ALLIANCE S.A.",  2162443797ull,  0.3040,   1.00, 1.000000, 1.000000, 0.25, 0.96},
                {"COTE",     "CONPET SA",                               8657528ull,     80.0000,  0.40, 1.000000, 1.000000, 0.50, 0.81},
                {"ATB",      "ANTIBIOTICE S.A.",                        671338040ull,   1.4350,   0.50, 1.000000, 1.000000, 0.25, 0.71},
                {"ALR",      "ALRO S.A.",                               713779135ull,   1.5300,   0.30, 1.000000, 1.000000, 0.25, 0.48},
                {"BNET",     "BITTNET SYSTEMS SA BUCURESTI",            634176714ull,   0.2760,   0.50, 1.000000, 1.000000, 0.50, 0.26},
                {"ROCE",     "ROMCARBON SA",                            528244192ull,   0.2070,   0.40, 1.000000, 1.000000, 0.50, 0.13},
                {"BRK",      "SSIF BRK FINANCIAL GROUP SA",             337429952ull,   0.1575,   0.90, 1.000000, 1.000000, 0.25, 0.07},
            }
        },
        {
            "BET-BK", "12/8/2023", "Operational adjustment INFINITY",
            {
                {"H2O",      "S.P.E.E.H. HIDROELECTRICA S.A.",          449802567ull,   126.4000, 0.20, 0.113508, 1.000000, 1.00, 7.55},
                {"EBS",      "Erste Group Bank AG",                     429800000ull,   183.0000, 0.80, 0.026309, 1.000000, 0.75, 7.26},
                {"SNP",      "OMV PETROM S.A.",                         62311667058ull, 0.5600,   0.30, 0.118088, 1.000000, 1.00, 7.23},
                {"TLV",      "BANCA TRANSILVANIA S.A.",                 798658233ull,   23.3400,  1.00, 0.066095, 1.000000, 1.00, 7.20},
                {"EL",       "SOCIETATEA ENERGETICA ELECTRICA S.A.",    346443597ull,   10.5200,  0.40, 0.588574, 1.000000, 1.00, 5.02},
                {"TTS",      "TTS (TRANSPORT TRADE SERVICES)",          60000000ull,    27.0000,  0.70, 1.000000, 1.000000, 0.75, 4.97},
                {"TGN",      "S.N.T.G.N. TRANSGAZ S.A.",                188381504ull,   19.2200,  0.50, 0.459641, 1.000000, 1.00, 4.87},
                {"SNN",      "S.N. NUCLEARELECTRICA S.A.",              301643894ull,   49.2500,  0.20, 0.274954, 1.000000, 1.00, 4.78},
                {"SNG",      "S.N.G.N. ROMGAZ S.A.",                    385422400ull,   49.8500,  0.30, 0.139204, 1.000000, 1.00, 4.69},
                {"DIGI",     "Digi Communications N.V.",                100000000ull,   41.9000,  0.40, 0.635257, 1.000000, 0.75, 4.67},
                {"BRD",      "BRD - GROUPE SOCIETE GENERALE S.A.",      696901518ull,   17.4800,  0.40, 0.162457, 1.000000, 1.00, 4.63},
                {"M",        "MedLife S.A.",                            531481968ull,   4.2000,   0.70, 0.663719, 1.000000, 0.75, 4.55},
                {"ONE",      "ONE UNITED PROPERTIES",                   3797654315ull,  0.9600,   0.40, 0.701371, 1.000000, 0.75, 4.48},
                {"FP",       "FONDUL PROPRIETATEA",                     6217825213ull,  0.4925,   0.80, 0.290866, 1.000000, 1.00, 4.17},
                {"TEL",      "C.N.T.E.E. TRANSELECTRICA",               73303142ull,    29.5000,  0.40, 1.000000, 1.000000, 0.75, 3.79},
                {"EVER",     "EVERGENT INVESTMENTS S.A.",               961753592ull,   1.2750,   1.00, 1.000000, 1.000000, 0.50, 3.58},
                {"TRP",      "TERAPLAST SA",                            2179000358ull,  0.4800,   0.60, 1.000000, 1.000000, 0.75, 2.75},
                {"INFINITY", "INFINITY CAPITAL INVESTMENTS S.A.",       500000000ull,   1.8300,   1.00, 1.000000, 1.000000, 0.50, 2.67},
                {"WINE",     "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull,    13.9000,  0.80, 1.000000, 1.000000, 0.75, 1.96},
                {"LION",     "LION CAPITAL S.A.",                       507510056ull,   2.4900,   1.00, 1.000000, 1.000000, 0.25, 1.85},
                {"BVB",      "BURSA DE VALORI BUCURESTI SA",            8049246ull,     68.0000,  1.00, 1.000000, 1.000000, 0.50, 1.60},
                {"AQ",       "AQUILA PART PROD COM",                    1200002400ull,  0.8900,   0.40, 1.000000, 1.000000, 0.50, 1.25},
                {"SFG",      "Sphera Franchise Group",                  38799340ull,    23.7000,  0.40, 1.000000, 1.000000, 0.50, 1.08},
                {"TRANSI",   "TRANSILVANIA INVESTMENTS ALLIANCE S.A.",  2162443797ull,  0.3040,   1.00, 1.000000, 1.000000, 0.25, 0.96},
                {"COTE",     "CONPET SA",                               8657528ull,     80.0000,  0.40, 1.000000, 1.000000, 0.50, 0.81},
                {"ATB",      "ANTIBIOTICE S.A.",                        671338040ull,   1.4350,   0.50, 1.000000, 1.000000, 0.25, 0.70},
                {"ALR",      "ALRO S.A.",                               713779135ull,   1.5300,   0.30, 1.000000, 1.000000, 0.25, 0.48},
                {"BNET",     "BITTNET SYSTEMS SA BUCURESTI",            634176714ull,   0.2760,   0.50, 1.000000, 1.000000, 0.50, 0.26},
                {"ROCE",     "ROMCARBON SA",                            264122096ull,   0.2070,   0.40, 1.000000, 2.000000, 0.50, 0.13},
                {"BRK",      "SSIF BRK FINANCIAL GROUP SA",             337429952ull,   0.1575,   1.00, 1.000000, 1.000000, 0.25, 0.08},
            }
        },
        {
            "BET-BK", "11/17/2023", "Operational adjustment ",
            {
                {"H2O",    "S.P.E.E.H. HIDROELECTRICA S.A.",          449802567ull,   118.9000, 0.2, 0.113508, 1.000000, 1.00, 7.25},
                {"SNP",    "OMV PETROM S.A.",                         62311667058ull, 0.5500,   0.3, 0.118088, 1.000000, 1.00, 7.25},
                {"TLV",    "BANCA TRANSILVANIA S.A.",                 798658233ull,   23.0000,  1.0, 0.066095, 1.000000, 1.00, 7.25},
                {"EBS",    "Erste Group Bank AG",                     429800000ull,   178.9500, 0.8, 0.026309, 1.000000, 0.75, 7.25},
                {"M",      "MedLife S.A.",                            531481968ull,   4.2500,   0.7, 0.663719, 1.000000, 0.75, 4.70},
                {"EL",     "SOCIETATEA ENERGETICA ELECTRICA S.A.",    346443597ull,   9.6500,   0.4, 0.588574, 1.000000, 1.00, 4.70},
                {"TGN",    "S.N.T.G.N. TRANSGAZ S.A.",                188381504ull,   18.1800,  0.5, 0.459641, 1.000000, 1.00, 4.70},
                {"SNN",    "S.N. NUCLEARELECTRICA S.A.",              301643894ull,   47.4500,  0.2, 0.274954, 1.000000, 1.00, 4.70},
                {"DIGI",   "Digi Communications N.V.",                100000000ull,   41.3000,  0.4, 0.635257, 1.000000, 0.75, 4.70},
                {"ONE",    "ONE UNITED PROPERTIES",                   3797654315ull,  0.9850,   0.4, 0.701371, 1.000000, 0.75, 4.70},
                {"FP",     "FONDUL PROPRIETATEA",                     6217825213ull,  0.5440,   0.8, 0.290866, 1.000000, 1.00, 4.70},
                {"BRD",    "BRD - GROUPE SOCIETE GENERALE S.A.",      696901518ull,   17.3800,  0.4, 0.162457, 1.000000, 1.00, 4.70},
                {"SNG",    "S.N.G.N. ROMGAZ S.A.",                    385422400ull,   48.9000,  0.3, 0.139204, 1.000000, 1.00, 4.70},
                {"TTS",    "TTS (TRANSPORT TRADE SERVICES)",          60000000ull,    24.5000,  0.7, 1.000000, 1.000000, 0.75, 4.61},
                {"TEL",    "C.N.T.E.E. TRANSELECTRICA",               73303142ull,    29.1000,  0.4, 1.000000, 1.000000, 0.75, 3.82},
                {"EVER",   "EVERGENT INVESTMENTS S.A.",               961753592ull,   1.2650,   1.0, 1.000000, 1.000000, 0.50, 3.63},
                {"TRP",    "TERAPLAST SA",                            2179000358ull,  0.4830,   0.6, 1.000000, 1.000000, 0.75, 2.83},
                {"SIF5",   "INFINITY CAPITAL INVESTMENTS S.A.",       500000000ull,   1.8450,   1.0, 1.000000, 1.000000, 0.50, 2.75},
                {"WINE",   "PURCARI WINERIES PUBLIC COMPANY LIMITED", 40117500ull,    13.9600,  0.8, 1.000000, 1.000000, 0.75, 2.01},
                {"LION",   "LION CAPITAL S.A.",                       507510056ull,   2.4800,   1.0, 1.000000, 1.000000, 0.25, 1.88},
                {"BVB",    "BURSA DE VALORI BUCURESTI SA",            8049246ull,     64.2000,  1.0, 1.000000, 1.000000, 0.50, 1.54},
                {"AQ",     "AQUILA PART PROD COM",                    1200002400ull,  0.8820,   0.4, 1.000000, 1.000000, 0.50, 1.26},
                {"TRANSI", "TRANSILVANIA INVESTMENTS ALLIANCE S.A.",  2162443797ull,  0.3040,   1.0, 1.000000, 1.000000, 0.25, 0.98},
                {"SFG",    "Sphera Franchise Group",                  38799340ull,    20.3000,  0.4, 1.000000, 1.000000, 0.50, 0.94},
                {"COTE",   "CONPET SA",                               8657528ull,     76.8000,  0.4, 1.000000, 1.000000, 0.50, 0.79},
                {"ATB",    "ANTIBIOTICE S.A.",                        671338040ull,   1.4550,   0.5, 1.000000, 1.000000, 0.25, 0.73},
                {"ALR",    "ALRO S.A.",                               713779135ull,   1.4900,   0.3, 1.000000, 1.000000, 0.25, 0.48},
                {"BNET",   "BITTNET SYSTEMS SA BUCURESTI",            634176714ull,   0.2560,   0.5, 1.000000, 1.000000, 0.50, 0.24},
                {"ROCE",   "ROMCARBON SA",                            264122096ull,   0.1895,   0.4, 1.000000, 2.000000, 0.50, 0.12},
                {"BRK",    "SSIF BRK FINANCIAL GROUP SA",             337429952ull,   0.1585,   1.0, 1.000000, 1.000000, 0.25, 0.08},
            }
        },
    };
    // clang-format on
    IndexName name = "BET-BK";
    BvbScraperTest bvbTest;
    std::ifstream f("test/data/parse_index_adjustments_history.txt");

    ASSERT_TRUE(f.is_open());

    std::string data(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    f.close();

    ASSERT_TRUE(data.size() > 0);

    auto res = bvbTest.ParseAdjustmentsHistory(data, name);
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res.value().size(), expectedIndexes.size());

    for (size_t i = 0; i < expectedIndexes.size(); i++) {
        const auto& index = res.value()[i];

        ASSERT_EQ(index.name, expectedIndexes[i].name);
        ASSERT_EQ(index.date, expectedIndexes[i].date);
        ASSERT_EQ(index.reason, expectedIndexes[i].reason);
        ASSERT_EQ(index.companies.size(), expectedIndexes[i].companies.size());

        for (size_t j = 0; j < index.companies.size(); j++) {
            ASSERT_EQ(
                index.companies[j].symbol,
                expectedIndexes[i].companies[j].symbol);
            ASSERT_EQ(
                index.companies[j].name,
                expectedIndexes[i].companies[j].name);
            ASSERT_EQ(
                index.companies[j].shares,
                expectedIndexes[i].companies[j].shares);
            ASSERT_DOUBLE_EQ(
                index.companies[j].reference_price,
                expectedIndexes[i].companies[j].reference_price);
            ASSERT_DOUBLE_EQ(
                index.companies[j].free_float_factor,
                expectedIndexes[i].companies[j].free_float_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].representation_factor,
                expectedIndexes[i].companies[j].representation_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].price_correction_factor,
                expectedIndexes[i].companies[j].price_correction_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].liquidity_factor,
                expectedIndexes[i].companies[j].liquidity_factor);
            ASSERT_DOUBLE_EQ(
                index.companies[j].weight,
                expectedIndexes[i].companies[j].weight);
        }
    }
}
