#include "TcpConnectionPanel.hpp"
#include "MainFrame.hpp"
#include "bm.hpp"
#include "bc.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>

TcpConnectionPanel::TcpConnectionPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    
    m_mainFrame = dynamic_cast<MainFrame*>(parent);
    
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddStretchSpacer(1);
    
    auto* contentSizer = new wxBoxSizer(wxVERTICAL);
    
    // TCP Suite Style Title
    auto* title = new wxStaticText(this, wxID_ANY, "TCP 1553 Suite", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(24);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    title->SetForegroundColour(wxColour("#004488"));
    contentSizer->Add(title, 0, wxALIGN_CENTER | wxBOTTOM, 5);

    auto* subtitle = new wxStaticText(this, wxID_ANY, "Passive TCP Monitoring Setup", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    wxFont subFont = subtitle->GetFont();
    subFont.SetPointSize(12);
    subtitle->SetFont(subFont);
    contentSizer->Add(subtitle, 0, wxALIGN_CENTER | wxBOTTOM, 30);
    
    // Form Container
    auto* formSizer = new wxFlexGridSizer(2, 2, 15, 15);
    
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Network Interface:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    m_hostText = new wxTextCtrl(this, wxID_ANY, "lo", wxDefaultPosition, wxSize(200, 30));
    formSizer->Add(m_hostText, 1, wxEXPAND);
    
    formSizer->Add(new wxStaticText(this, wxID_ANY, "TCP Bus Port:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
    m_portText = new wxTextCtrl(this, wxID_ANY, "5002", wxDefaultPosition, wxSize(200, 30));
    formSizer->Add(m_portText, 1, wxEXPAND);
    
    contentSizer->Add(formSizer, 0, wxALIGN_CENTER | wxALL, 20);

    m_connectButton = new wxButton(this, wxID_ANY, "Connect TCP Connection", wxDefaultPosition, wxSize(220, 50));
    m_connectButton->SetBackgroundColour(wxColour("#ffcc00")); // AIM Yellow
    m_connectButton->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    contentSizer->Add(m_connectButton, 0, wxALIGN_CENTER | wxTOP, 30);
    
    mainSizer->Add(contentSizer, 0, wxALIGN_CENTER);
    mainSizer->AddStretchSpacer(2);
    
    SetSizer(mainSizer);
    
    m_connectButton->Bind(wxEVT_BUTTON, &TcpConnectionPanel::onConnect, this);
}

void TcpConnectionPanel::onConnect(wxCommandEvent& event) {
    wxString interface = m_hostText->GetValue();
    long port = 0;
    
    if (!m_portText->GetValue().ToLong(&port)) {
        wxMessageBox("Invalid bus port number.", "Input Error", wxOK | wxICON_ERROR);
        return;
    }
    
    // Initialize passive monitor
    if (BM::getInstance().start(interface.ToStdString(), (uint16_t)port)) {
        if (m_mainFrame) {
            m_mainFrame->InitializePanels(interface, (uint16_t)port);
        }
    } else {
        wxString errorMsg = wxString::Format(
            "Failed to connect TCP Sniffer on %s:%ld\n\n"
            "This is likely a 'Permission Denied' error.\n"
            "To capture packets on %s, the application needs elevated privileges.\n\n"
            "Solution:\n"
            "Run the permission script provided in the project root:\n"
            "  sudo ./scripts/setup_permissions.sh\n\n"
            "Then restart this application.",
            interface, port, interface);
        wxMessageBox(errorMsg, "TCP Connection Error - Permission Denied", wxOK | wxICON_ERROR);
    }
}
