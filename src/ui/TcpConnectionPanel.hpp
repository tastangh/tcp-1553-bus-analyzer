#pragma once

#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/button.h>

class MainFrame;

class TcpConnectionPanel : public wxPanel {
public:
    TcpConnectionPanel(wxWindow* parent);

private:
    void onConnect(wxCommandEvent& event);

    wxTextCtrl* m_hostText;
    wxTextCtrl* m_portText;
    wxButton* m_connectButton;
    MainFrame* m_mainFrame;
};
