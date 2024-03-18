#ifndef STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_TERMINAL_UI_H
#define STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_TERMINAL_UI_H

#include "bvb_scraper.h"
#include "config.h"
#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"
#include "stock_index.h"
#include "tradeville.h"

#include <expected.hpp>
#include <string>
#include <vector>

// ftxui
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>

class TerminalUi : private noncopyable, private nonmovable {
public:
    TerminalUi(const Config& cfg);
    ~TerminalUi() = default;

    Error Init();
    void Run();

private:
    ftxui::Element RenderIndexReplicationTab();
    ftxui::Element RenderPortfolioTab();
    ftxui::Element RenderActivityTab();
    ftxui::Element RenderDividendsTab();

    ftxui::Element RenderScreen();

    bool HandleEvent(ftxui::Event ev);

    void LoadIndex();
    void GetDataFromTradeville();

    void GenerateIndexReplicationTabContent();
    void GeneratePortfolioTabContent();
    void GenerateActivityTabContent();
    void GenerateDividendsTabContent();

private:
    const Config& m_config;

    tl::expected<Index, Error> m_index;
    tl::expected<Activities, Error> m_activities;
    tl::expected<Portfolio, Error> m_portfolio;

    std::vector<std::string> m_tabNames;
    int m_selectedTab = 0;

    ftxui::Component m_indexReplicationTab;
    ftxui::Component m_portfolioTab;
    ftxui::Component m_activityTab;
    ftxui::Component m_dividendsTab;
    ftxui::Component m_tabContainer;
    ftxui::Component m_tabToggle;

    ftxui::Component m_screenContainer;
    ftxui::Component m_screenRenderer;

    ftxui::ScreenInteractive m_screen;

    ftxui::Element m_indexReplicationTabContent;
    ftxui::Element m_portfolioTabContent;
    ftxui::Element m_activityTabContent;
    ftxui::Element m_dividendsTabContent;
};

#endif // STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_TERMINAL_UI_H
