#pragma once

#include "common.hpp"
#include <wx/panel.h>
#include <wx/tglbtn.h>
#include <wx/scrolwin.h>
#include <thread>
#include <atomic>
#include <vector>

class FrameComponent;
class wxBoxSizer;
class MainFrame;

class BusControllerPanel : public wxPanel {
public:
    BusControllerPanel(wxWindow* parent);
    ~BusControllerPanel();

    void InitializeHardware(const wxString& host, uint16_t port);

    void addFrameToList(FrameConfig config);
    void removeFrame(FrameComponent* frame);
    void updateFrame(FrameComponent* oldFrame, const FrameConfig& newConfig);
    void updateListLayout();
    void setStatusText(const wxString &status);

    wxString getHost() const { return m_host; }
    uint16_t getPort() const { return m_port; }

private:
    void onAddFrameClicked(wxCommandEvent &event);
    void onClearFramesClicked(wxCommandEvent &event);
    void onRepeatToggle(wxCommandEvent &event);
    void onSendActiveFramesToggle(wxCommandEvent &event);

    void sendActiveFramesLoop();
    void startSendingThread();
    void stopSendingThread();
    
    MainFrame* m_mainFrame;
    wxString m_host;
    uint16_t m_port;

    wxToggleButton *m_repeatToggle;
    wxToggleButton *m_sendActiveFramesToggle;
    wxScrolledWindow *m_scrolledWindow;
    wxBoxSizer *m_scrolledSizer;

    std::thread m_sendThread;
    std::atomic<bool> m_isSending{false};
    std::atomic<bool> m_isRepeatOn{false}; 
    std::vector<FrameComponent*> m_frameComponents;
};
