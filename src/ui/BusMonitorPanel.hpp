#pragma once

#include <wx/panel.h>
#include <wx/treectrl.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/timer.h>
#include <map>
#include <cstdint>

class MainFrame;
class wxTreeEvent;

class BusMonitorPanel : public wxPanel {
public:
    BusMonitorPanel(wxWindow* parent);
    ~BusMonitorPanel();

    void InitializeHardware(const wxString& host, uint16_t port);

private:
    void onStartStopClicked(wxCommandEvent &event);
    void onClearFilterClicked(wxCommandEvent &event);
    void onClearClicked(wxCommandEvent &event);
    void onTreeItemClicked(wxTreeEvent &event);
    void onLogToFileToggled(wxCommandEvent &event);
    void onResetStreamClicked(wxCommandEvent &event);
    void onStatusTimer(wxTimerEvent &event);

    void appendMessagesToUi(const wxString& messages);
    void updateTreeItemVisualState(char bus, int rt, int sa, bool isActive);
    void resetTreeVisualState();
    void setStatusText(const wxString& text);

    MainFrame* m_mainFrame;
    wxString m_host;
    uint16_t m_port;

    int m_uiRecentMessageCount;
    wxTreeCtrl *m_milStd1553Tree;
    wxTextCtrl *m_messageList;
    wxButton *m_startStopButton;
    wxButton *m_filterButton;
    wxCheckBox *m_logToFileCheckBox;
    wxButton *m_resetStreamButton;
    wxTimer *m_statusTimer;
    std::map<wxTreeItemId, int> m_treeItemToMcMap;
};
