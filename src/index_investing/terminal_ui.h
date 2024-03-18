#ifndef STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_TERMINAL_UI_H
#define STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_TERMINAL_UI_H

#include "error.h"
#include "noncopyable.h"
#include "nonmovable.h"

#include <string>
#include <vector>

// ftxui
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>

class TerminalUi : private noncopyable, private nonmovable {
public:
    TerminalUi();
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

private:
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
};

#endif // STOCK_EXCHANGE_TOOLS_INDEX_INVESTING_TERMINAL_UI_H
