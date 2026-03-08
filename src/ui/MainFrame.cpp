#include "MainFrame.hpp"
#include "BusControllerPanel.hpp"
#include "BusMonitorPanel.hpp"
#include "TcpConnectionPanel.hpp"
#include "bm.hpp"
#include "bc.hpp"
#include <wx/sizer.h>

enum { ID_Exit = wxID_EXIT };

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "TCP 1553 Suite - Connection Setup", wxDefaultPosition, wxSize(1100, 800)) {
    
    CreateStatusBar();
    SetStatusText("Ready. Please initialize Passive TCP connection.");

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Setup Panel (Initial view)
    m_tcpPanel = new TcpConnectionPanel(this);
    sizer->Add(m_tcpPanel, 1, wxEXPAND);

    // Notebook (Hidden until connection)
    m_notebook = new wxNotebook(this, wxID_ANY);
    m_bcPanel = new BusControllerPanel(m_notebook);
    m_bmPanel = new BusMonitorPanel(m_notebook);
    
    m_notebook->AddPage(m_bcPanel, "Bus Controller");
    m_notebook->AddPage(m_bmPanel, "Bus Monitor");
    
    sizer->Add(m_notebook, 1, wxEXPAND);
    m_notebook->Hide();

    SetSizer(sizer);
    
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onClose, this);
    
    Centre();
}

void MainFrame::InitializePanels(const wxString& host, uint16_t port) {
    m_bcPanel->InitializeHardware(host, port);
    m_bmPanel->InitializeHardware(host, port);

    m_tcpPanel->Hide();
    m_notebook->Show();
    GetSizer()->Layout();
    SetTitle(wxString::Format("TCP 1553 Suite - Monitoring %s:%u", host, port));

    auto *menuFile = new wxMenu;
    menuFile->Append(ID_Exit, "E&xit\tAlt-X");
    auto *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);
    Bind(wxEVT_MENU, &MainFrame::onExit, this, ID_Exit);
    
    SetStatusText(wxString::Format("Passive Monitoring Active: %s:%u", host, port));
}

void MainFrame::onExit(wxCommandEvent& event) {
    Close(true); 
}

void MainFrame::onClose(wxCloseEvent& event) {
    Destroy();
}