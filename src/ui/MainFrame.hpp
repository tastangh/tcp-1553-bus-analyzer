#pragma once

#include <wx/wx.h>
#include <wx/notebook.h>
#include <cstdint>

class BusControllerPanel;
class BusMonitorPanel;
class TcpConnectionPanel;

class MainFrame : public wxFrame {
public:
    MainFrame();
    void InitializePanels(const wxString& host, uint16_t port);

private:
    void onExit(wxCommandEvent& event);
    void onClose(wxCloseEvent& event);

    wxNotebook* m_notebook;
    BusControllerPanel* m_bcPanel;
    BusMonitorPanel* m_bmPanel;
    TcpConnectionPanel* m_tcpPanel;
};