#include "BusControllerPanel.hpp"
#include "MainFrame.hpp"
#include "CreateFrameWindow.hpp"
#include "FrameComponent.hpp"
#include "bc.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/statline.h>
#include <algorithm>
#include <future>

BusControllerPanel::BusControllerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_port(0) {

    m_mainFrame = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
  
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
  
    m_repeatToggle = new wxToggleButton(this, wxID_ANY, "Repeat Off", wxDefaultPosition, wxSize(120, 35));
    m_sendActiveFramesToggle = new wxToggleButton(this, wxID_ANY, "Send Active Frames", wxDefaultPosition, wxSize(180, 35));
    m_sendActiveFramesToggle->SetForegroundColour(*wxBLACK);
    
    auto* addButton = new wxButton(this, wxID_ANY, "Add Frame", wxDefaultPosition, wxSize(120, 35));
    auto* clearButton = new wxButton(this, wxID_ANY, "Clear All", wxDefaultPosition, wxSize(100, 35));

    topSizer->Add(new wxStaticText(this, wxID_ANY, "Injection Controls:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    topSizer->AddStretchSpacer();
    topSizer->Add(m_repeatToggle, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    topSizer->Add(m_sendActiveFramesToggle, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    topSizer->Add(addButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    topSizer->Add(clearButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

    m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    m_scrolledWindow->SetBackgroundColour(wxColour("#f0f0f0")); // Light gray background
    m_scrolledSizer = new wxBoxSizer(wxVERTICAL);
    m_scrolledWindow->SetSizer(m_scrolledSizer);
    m_scrolledWindow->SetScrollRate(0, 10);
    m_scrolledWindow->FitInside();

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(topSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(new wxStaticLine(this, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxBOTTOM, 5);
    mainSizer->Add(m_scrolledWindow, 1, wxEXPAND | wxALL, 5);
    
    this->SetSizer(mainSizer);

    addButton->Bind(wxEVT_BUTTON, &BusControllerPanel::onAddFrameClicked, this);
    clearButton->Bind(wxEVT_BUTTON, &BusControllerPanel::onClearFramesClicked, this);
    m_repeatToggle->Bind(wxEVT_TOGGLEBUTTON, &BusControllerPanel::onRepeatToggle, this);
    m_sendActiveFramesToggle->Bind(wxEVT_TOGGLEBUTTON, &BusControllerPanel::onSendActiveFramesToggle, this);
}

BusControllerPanel::~BusControllerPanel() {
    stopSendingThread();
}

void BusControllerPanel::InitializeHardware(const wxString& host, uint16_t port) {
    m_host = host;
    m_port = port;
    auto& bc = BusController::getInstance();
    if (bc.initialize(host.ToStdString(), port) == 0) {
        setStatusText(wxString::Format("BC Panel Initialized for %s:%u. Ready.", m_host, m_port));
    } else {
        setStatusText("BC initialization failed: " + wxString(bc.getLastError()));
    }
}

void BusControllerPanel::setStatusText(const wxString &status) {
    if (m_mainFrame) {
        m_mainFrame->SetStatusText(status); 
    }
}

void BusControllerPanel::addFrameToList(FrameConfig config) {
    auto *component = new FrameComponent(m_scrolledWindow, config);
    m_frameComponents.push_back(component);
    m_scrolledSizer->Add(component, 0, wxEXPAND | wxALL, 5);
    updateListLayout();
}

void BusControllerPanel::updateFrame(FrameComponent* oldFrame, const FrameConfig& newConfig) {
    removeFrame(oldFrame);
    addFrameToList(newConfig);
}

void BusControllerPanel::removeFrame(FrameComponent* frame) {
    if (!frame) return;
    m_scrolledSizer->Detach(frame);
    m_frameComponents.erase(std::remove(m_frameComponents.begin(), m_frameComponents.end(), frame), m_frameComponents.end());
    updateListLayout();
    wxTheApp->CallAfter([frame](){ frame->Destroy(); });
}

void BusControllerPanel::onAddFrameClicked(wxCommandEvent &) {
    auto *frame = new FrameCreationFrame(this);
    frame->Show(true);
}

void BusControllerPanel::onClearFramesClicked(wxCommandEvent &) {
    if (m_isSending) {
        wxMessageBox("Please stop sending frames before clearing the list.", "Warning", wxOK | wxICON_WARNING);
        return;
    }
    auto components_to_delete = m_frameComponents;
    for (auto* comp : components_to_delete) {
        removeFrame(comp);
    }
    setStatusText("All frames cleared.");
}

void BusControllerPanel::onRepeatToggle(wxCommandEvent &) {
    bool is_on = m_repeatToggle->GetValue();
    m_repeatToggle->SetLabel(is_on ? "Repeat On" : "Repeat Off");
    m_isRepeatOn = is_on;
}

void BusControllerPanel::onSendActiveFramesToggle(wxCommandEvent &event) {
    if (m_sendActiveFramesToggle->GetValue()) {
        if (m_isSending) return; 
        m_sendActiveFramesToggle->SetLabel("Sending...");
        startSendingThread();
    } else {
        stopSendingThread();
    }
}

void BusControllerPanel::startSendingThread() {
    if (m_isSending) return;
    m_isSending = true;
    m_isRepeatOn = m_repeatToggle->GetValue();
    m_sendThread = std::thread(&BusControllerPanel::sendActiveFramesLoop, this);
}

void BusControllerPanel::stopSendingThread() {
    m_isSending = false;
    if (m_sendThread.joinable()) {
        m_sendThread.join();
    }
    
    wxTheApp->CallAfter([this] {
        if(this) { 
            m_sendActiveFramesToggle->SetValue(false); 
            m_sendActiveFramesToggle->SetLabel("Send Active Frames");
            m_sendActiveFramesToggle->Enable(); 
            setStatusText("Sending stopped.");
        }
    });
}

void BusControllerPanel::sendActiveFramesLoop() {
    while (m_isSending) {
        
        // Auto-reconnect mentality (like AIM)
        auto& bc = BusController::getInstance();
        if (!bc.isInitialized()) {
             if (bc.initialize(m_host.ToStdString(), m_port) != 0) {
                 wxTheApp->CallAfter([this, &bc] { 
                     if(this) { 
                         setStatusText("BC init failed: " + wxString(bc.getLastError())); 
                         stopSendingThread(); 
                     }
                 });
                 break;
             }
        }

        // Fetch active frames dynamically inside the loop
        auto promise_ptr = std::make_shared<std::promise<std::vector<FrameComponent*>>>();
        auto future = promise_ptr->get_future();
        
        wxTheApp->CallAfter([this, promise_ptr]() {
            std::vector<FrameComponent*> activeFrames;
            if (this) {
                for (auto* frame : m_frameComponents) {
                    if (frame) activeFrames.push_back(frame);
                }
            }
            promise_ptr->set_value(activeFrames);
        });
        
        std::vector<FrameComponent*> frames = future.get();
        if (frames.empty()) {
            wxTheApp->CallAfter([this] { if(this) { setStatusText("No frames available."); stopSendingThread(); }});
            break;
        }

        bool sentAny = false;
        for (FrameComponent* frame : frames) {
            if (!m_isSending) break;
            
            // Critical fix: Check isActive() right before sending!
            if (frame->isActive()) {
                frame->sendFrame();
                sentAny = true;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        
        if (!m_isSending) break; 

        if (!sentAny) {
             wxTheApp->CallAfter([this] { if(this) { setStatusText("All frames are disabled."); } });
             std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Prevent CPU spin loop
        }

        if (!m_isRepeatOn) {
            break;
        }
    }
    
    wxTheApp->CallAfter([this] {
        if(this) { stopSendingThread(); }
    });
}

void BusControllerPanel::updateListLayout() {
    m_scrolledSizer->Layout();
    m_scrolledWindow->FitInside();
    this->Layout();
}
