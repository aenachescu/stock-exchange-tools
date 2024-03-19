#include "terminal_ui.h"

#include "chrono_utils.h"
#include "string_utils.h"

#include <magic_enum.hpp>

// ftxui
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>

TerminalUi::TerminalUi(const Config& cfg)
    : m_config(cfg), m_index(tl::unexpected(Error::InvalidData)),
      m_activities(tl::unexpected(Error::InvalidData)),
      m_portfolio(tl::unexpected(Error::InvalidData)),
      m_tabNames{"Index replication", "Portfolio", "Activity", "Dividends"},
      m_screen(ftxui::ScreenInteractive::Fullscreen())
{
}

Error TerminalUi::Init()
{
    std::function<ftxui::Element()> renderCbk;
    std::function<bool(ftxui::Event)> eventCbk;

    LoadIndex();
    GetDataFromTradeville();

    GenerateIndexReplicationTabContent();
    GeneratePortfolioTabContent();
    GenerateActivityTabContent();
    GenerateDividendsTabContent();

    // tab renderers
    renderCbk = std::bind(&TerminalUi::RenderIndexReplicationTab, this);
    m_indexReplicationTab = ftxui::Renderer(renderCbk);
    renderCbk             = std::bind(&TerminalUi::RenderPortfolioTab, this);
    m_portfolioTab        = ftxui::Renderer(renderCbk);
    renderCbk             = std::bind(&TerminalUi::RenderActivityTab, this);
    m_activityTab         = ftxui::Renderer(renderCbk);
    renderCbk             = std::bind(&TerminalUi::RenderDividendsTab, this);
    m_dividendsTab        = ftxui::Renderer(renderCbk);

    // tab container
    m_tabToggle    = ftxui::Toggle(&m_tabNames, &m_selectedTab);
    m_tabContainer = ftxui::Container::Tab(
        {
            m_indexReplicationTab,
            m_portfolioTab,
            m_activityTab,
            m_dividendsTab,
        },
        &m_selectedTab);

    // screen container
    m_screenContainer = ftxui::Container::Vertical({
        m_tabToggle,
        m_tabContainer,
    });

    // screen renderer
    renderCbk        = std::bind(&TerminalUi::RenderScreen, this);
    m_screenRenderer = ftxui::Renderer(m_screenContainer, renderCbk);

    eventCbk = std::bind(&TerminalUi::HandleEvent, this, std::placeholders::_1);
    m_screenRenderer |= ftxui::CatchEvent(eventCbk);

    return Error::NoError;
}

void TerminalUi::Run()
{
    m_screen.Loop(m_screenRenderer);
}

ftxui::Element TerminalUi::RenderIndexReplicationTab()
{
    return m_indexReplicationTabContent;
}

ftxui::Element TerminalUi::RenderPortfolioTab()
{
    return m_portfolioTabContent;
}

ftxui::Element TerminalUi::RenderActivityTab()
{
    return m_activityTabContent;
}

ftxui::Element TerminalUi::RenderDividendsTab()
{
    return m_dividendsTabContent;
}

ftxui::Element TerminalUi::RenderScreen()
{
    return ftxui::vbox({
               m_tabToggle->Render(),
               ftxui::separator(),
               m_tabContainer->Render(),
           }) |
        ftxui::border;
}

bool TerminalUi::HandleEvent(ftxui::Event ev)
{
    if (ev == ftxui::Event::Escape) {
        m_screen.Exit();
        return true;
    }

    return false;
}

void TerminalUi::LoadIndex()
{
    BvbScraper bvb;

    auto indexes = bvb.LoadAdjustmentsHistoryFromFile(*m_config.GetIndexName());
    if (! indexes) {
        m_index = tl::unexpected(indexes.error());
    }

    for (const auto& i : *indexes) {
        if (i.date == *m_config.GetIndexAdjustmentDate() &&
            i.reason == *m_config.GetIndexAdjustmentReason()) {
            m_index = i;
            return;
        }
    }

    m_index = tl::unexpected(Error::InvalidArg);
}

void TerminalUi::GetDataFromTradeville()
{
    Tradeville tv(*m_config.GetTradevilleUser(), *m_config.GetTradevillePass());
    uint64_t startYear = std::stoull(*m_config.GetTradevilleStartYear());
    uint64_t endYear   = get_current_year();

    m_portfolio = tv.GetPortfolio();
    if (! m_portfolio) {
        return;
    }

    m_activities = tv.GetActivity(std::nullopt, startYear, endYear);
    if (! m_activities) {
        return;
    }
}

void TerminalUi::GenerateIndexReplicationTabContent()
{
    m_indexReplicationTabContent =
        ftxui::text("index replication tab") | ftxui::center;
}

void TerminalUi::GeneratePortfolioTabContent()
{
    if (! m_portfolio) {
        std::string errorMsg = "Failed to get portfolio: ";
        errorMsg += magic_enum::enum_name(m_portfolio.error());
        m_portfolioTabContent = ftxui::text(errorMsg) | ftxui::center;
    }

    std::vector<std::vector<std::string>> tableData;
    size_t id = 1;

    tableData.reserve(m_portfolio->entries.size() + 1);
    tableData.emplace_back(std::vector<std::string>{
        "#",
        "Account",
        "Symbol",
        "Quantity",
        "Avg price",
        "Market price",
        "Cost",
        "Value",
        "P/L",
        "P/L %",
        "Currency",
        "Asset",
    });

    for (const auto& i : m_portfolio->entries) {
        double quantity = 0.0;
        if (std::holds_alternative<uint64_t>(i.quantity) == true) {
            quantity = std::get<uint64_t>(i.quantity);
        } else {
            quantity = std::get<double>(i.quantity);
        }

        double cost  = i.avg_price * quantity;
        double value = i.market_price * quantity;
        double pl    = value - cost;
        double plp   = pl / cost * 100.0;

        tableData.emplace_back(std::vector<std::string>{
            std::to_string(id),
            i.account,
            i.symbol,
            quantity_to_string(i.quantity),
            double_to_string(i.avg_price, 4),
            double_to_string(i.market_price, 4),
            double_to_string(cost),
            double_to_string(value),
            double_to_string(pl),
            double_to_string(plp),
            std::string{magic_enum::enum_name(i.currency)},
            std::string{magic_enum::enum_name(i.asset)},
        });
        id++;
    }

    auto table = ftxui::Table(tableData);
    table.SelectAll().Border(ftxui::BorderStyle::DOUBLE);

    table.SelectRow(0).Decorate(ftxui::bold);
    table.SelectRow(0).SeparatorVertical(ftxui::BorderStyle::LIGHT);
    table.SelectRow(0).Border(ftxui::BorderStyle::DOUBLE);

    table.SelectColumns(0, -1).SeparatorVertical(ftxui::BorderStyle::LIGHT);

    m_portfolioTabContent = table.Render();
}

void TerminalUi::GenerateActivityTabContent()
{
    m_activityTabContent = ftxui::text("activity tab") | ftxui::center;
}

void TerminalUi::GenerateDividendsTabContent()
{
    m_dividendsTabContent = ftxui::text("dvd tab") | ftxui::center;
}
