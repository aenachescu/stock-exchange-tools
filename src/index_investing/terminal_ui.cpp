#include "terminal_ui.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

TerminalUi::TerminalUi()
    : m_tabNames{"Index replication", "Portfolio", "Activity", "Dividends"},
      m_screen(ftxui::ScreenInteractive::Fullscreen())
{
}

Error TerminalUi::Init()
{
    std::function<ftxui::Element()> renderCbk;
    std::function<bool(ftxui::Event)> eventCbk;

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
    return ftxui::text("index replication tab") | ftxui::center;
}

ftxui::Element TerminalUi::RenderPortfolioTab()
{
    return ftxui::text("portfolio tab") | ftxui::center;
}

ftxui::Element TerminalUi::RenderActivityTab()
{
    return ftxui::text("activity tab") | ftxui::center;
}

ftxui::Element TerminalUi::RenderDividendsTab()
{
    return ftxui::text("dividends tab") | ftxui::center;
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
