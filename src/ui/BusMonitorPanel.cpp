#include "BusMonitorPanel.hpp"
#include "MainFrame.hpp"
#include "milStd1553.hpp"
#include "bm.hpp"
#include "logger.hpp"
#include "common.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/msgdlg.h>
#include <wx/app.h>
#include <cctype>

BusMonitorPanel::BusMonitorPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_port(0) {

    m_mainFrame = dynamic_cast<MainFrame*>(wxGetTopLevelParent(this));
    
    auto *topHorizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    auto *bottomHorizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    auto *mainVerticalSizer = new wxBoxSizer(wxVERTICAL);

    m_startStopButton = new wxButton(this, wxID_ANY, "Start", wxDefaultPosition, wxSize(100, -1));
    m_startStopButton->SetBackgroundColour(wxColour("#ffcc00")); // AIM Yellow
    m_startStopButton->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));

    m_resetStreamButton = new wxButton(this, wxID_ANY, "Reset Connection");
    m_filterButton = new wxButton(this, wxID_ANY, "No filter set. Click a tree item to filter.", wxDefaultPosition, wxDefaultSize, wxBU_LEFT);
    m_filterButton->Enable(false);
    auto *clearButton = new wxButton(this, wxID_ANY, "Clear");
    m_logToFileCheckBox = new wxCheckBox(this, wxID_ANY, "Log to File"); 

    m_milStd1553Tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(250, -1), wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
    auto rtSaTreeRoot = m_milStd1553Tree->AddRoot("MIL-STD-1553 Buses");
    
    auto& model = MilStd1553::getInstance();
    for (size_t i = 0; i < model.busList.size(); ++i) {
        auto& bus = model.busList.at(i);
        bus.setTreeObject(m_milStd1553Tree->AppendItem(rtSaTreeRoot, bus.getName()));
        for (size_t j = 0; j < bus.rtList.size(); ++j) {
            auto& rt = bus.rtList.at(j);
            rt.setTreeObject(m_milStd1553Tree->AppendItem(bus.getTreeObject(), rt.getName()));
            for (size_t k = 0; k < rt.saList.size(); ++k) {
                auto& sa = rt.saList.at(k);
                sa.setTreeObject(m_milStd1553Tree->AppendItem(rt.getTreeObject(), sa.getName()));
            }
            // Porting Mode Codes from AIM Suite
            wxTreeItemId mcRoot = m_milStd1553Tree->AppendItem(rt.getTreeObject(), "Mode Codes");
            for (const auto& mc_pair : MilStd1553::getModeCodeList()) {
                wxString mcLabel = wxString::Format("MC %d: %s", mc_pair.first, mc_pair.second);
                wxTreeItemId mcItem = m_milStd1553Tree->AppendItem(mcRoot, mcLabel);
                m_treeItemToMcMap[mcItem] = mc_pair.first;
            }
        }
        if (i == 0) m_milStd1553Tree->Expand(bus.getTreeObject()); 
    }
    
    m_messageList = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxHSCROLL | wxTE_DONTWRAP);
    wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_messageList->SetFont(font);

    topHorizontalSizer->Add(m_startStopButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    topHorizontalSizer->Add(m_resetStreamButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    topHorizontalSizer->Add(m_filterButton, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    topHorizontalSizer->Add(m_logToFileCheckBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5); 
    topHorizontalSizer->Add(clearButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    bottomHorizontalSizer->Add(m_milStd1553Tree, 0, wxEXPAND | wxALL, 5); 
    bottomHorizontalSizer->Add(m_messageList, 1, wxEXPAND | wxALL, 5);   
    
    mainVerticalSizer->Add(topHorizontalSizer, 0, wxEXPAND | wxALL, 5);
    mainVerticalSizer->Add(bottomHorizontalSizer, 1, wxEXPAND | wxALL, 5);
    this->SetSizer(mainVerticalSizer);

    m_startStopButton->Bind(wxEVT_BUTTON, &BusMonitorPanel::onStartStopClicked, this);
    m_filterButton->Bind(wxEVT_BUTTON, &BusMonitorPanel::onClearFilterClicked, this);
    clearButton->Bind(wxEVT_BUTTON, &BusMonitorPanel::onClearClicked, this);
    m_resetStreamButton->Bind(wxEVT_BUTTON, &BusMonitorPanel::onResetStreamClicked, this);
    m_milStd1553Tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &BusMonitorPanel::onTreeItemClicked, this);
    m_logToFileCheckBox->Bind(wxEVT_CHECKBOX, &BusMonitorPanel::onLogToFileToggled, this);
    
    m_uiRecentMessageCount = 2000;
    
    BM::getInstance().setUpdateMessagesCallback(
        [this](const std::string& messages) {
            wxTheApp->CallAfter([this, messages] {
                appendMessagesToUi(wxString::FromUTF8(messages.c_str()));
            });
        }
    );
    BM::getInstance().setUpdateTreeItemCallback(
        [this](char bus, int rt, int sa, bool isActive) {
            wxTheApp->CallAfter([this, bus, rt, sa, isActive] {
                updateTreeItemVisualState(bus, rt, sa, isActive);
            });
        }
    );

    m_statusTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &BusMonitorPanel::onStatusTimer, this);
}

BusMonitorPanel::~BusMonitorPanel() {
    if (m_statusTimer->IsRunning()) m_statusTimer->Stop();
    delete m_statusTimer;
    if (BM::getInstance().isMonitoring()) {
        BM::getInstance().stop();
    }
}

void BusMonitorPanel::InitializeHardware(const wxString& host, uint16_t port) {
    m_host = host;
    m_port = port;
    setStatusText(wxString::Format("BM Panel Ready for Passive Sniffing on %s:%u.", m_host, m_port));

    if (BM::getInstance().isMonitoring()) {
        m_startStopButton->SetLabelText("Stop");
        m_startStopButton->SetBackgroundColour(wxColour("#ff4545")); // AIM Red
        m_startStopButton->SetForegroundColour(wxColour("white"));
    }
}

void BusMonitorPanel::setStatusText(const wxString& text) {
    if (m_mainFrame) {
        m_mainFrame->SetStatusText(text);
    }
}

void BusMonitorPanel::onStartStopClicked(wxCommandEvent &) {
    if (BM::getInstance().isMonitoring()) {
        setStatusText("Stopping monitoring...");
        m_statusTimer->Stop();
        BM::getInstance().stop();
        m_startStopButton->SetLabelText("Start");
        m_startStopButton->SetBackgroundColour(wxColour("#ffcc00"));
        m_startStopButton->SetForegroundColour(*wxBLACK);
        setStatusText("Monitoring stopped.");
    } else {
        bool shouldLogData = m_logToFileCheckBox->IsChecked();
        BM::getInstance().enableDataLogging(shouldLogData);
        resetTreeVisualState();
        m_messageList->Clear();

        setStatusText(wxString::Format("Starting Passive Sniffer on %s port %u...", m_host, m_port));
        
        if (BM::getInstance().start(m_host.ToStdString(), m_port)) {
            m_statusTimer->Start(2000);
            setStatusText(wxString::Format("Sniffer Active on %s:%u", m_host, m_port));
            m_startStopButton->SetLabelText("Stop");
            m_startStopButton->SetBackgroundColour(wxColour("#ff4545"));
            m_startStopButton->SetForegroundColour(wxColour("white"));
        } else {
            setStatusText("Error: Sniffer init failed (Check permissions).");
            wxMessageBox("Failed to initialize Passive Sniffer.\nEnsure you have CAP_NET_RAW or run with sudo.", "Error", wxOK | wxICON_ERROR, this);
        }
    }
}

void BusMonitorPanel::onResetStreamClicked(wxCommandEvent &event) {
    if (BM::getInstance().isMonitoring()) {
        wxMessageBox("Please stop the monitor before resetting.", "Warning", wxOK | wxICON_WARNING, this);
        return;
    }
    m_messageList->Clear();
    resetTreeVisualState();
    setStatusText("View and connection status reset.");
}

void BusMonitorPanel::onClearFilterClicked(wxCommandEvent &) {
    if (!BM::getInstance().isFilterEnabled()) return;
    BM::getInstance().enableFilter(false);
    m_filterButton->SetLabelText("No filter set. Click a tree item to filter.");
    m_filterButton->Enable(false);
    resetTreeVisualState();
    setStatusText("Filter cleared.");
}

void BusMonitorPanel::onClearClicked(wxCommandEvent &) {
    m_messageList->Clear();
    resetTreeVisualState();
    setStatusText("Messages cleared.");
}

void BusMonitorPanel::appendMessagesToUi(const wxString& newMessagesChunk) {
    m_messageList->AppendText(newMessagesChunk);
    
    static int checkCounter = 0;
    if (++checkCounter % 10 != 0) return; 

    int lines = m_messageList->GetNumberOfLines();
    if (lines > m_uiRecentMessageCount + 200) {
        int linesToRemove = lines - m_uiRecentMessageCount;
        long pos = m_messageList->XYToPosition(0, linesToRemove);
        if (pos > 0) {
            m_messageList->Remove(0, pos);
            m_messageList->SetInsertionPointEnd();
        }
    }
}

void BusMonitorPanel::onTreeItemClicked(wxTreeEvent &event) {
    wxTreeItemId clickedId = event.GetItem();
    if (!clickedId.IsOk()) return;

    char filterBusChar = 0;
    int filterRt = -1;
    int filterSa = -1;
    int filterMc = -1;
    bool found = false;
    auto& model = MilStd1553::getInstance();
    
    // Check if it's a Mode Code item
    auto it = m_treeItemToMcMap.find(clickedId);
    if (it != m_treeItemToMcMap.end()) {
        filterMc = it->second;
        // Find parent RT and Bus
        wxTreeItemId mcRootId = m_milStd1553Tree->GetItemParent(clickedId);
        wxTreeItemId rtId = m_milStd1553Tree->GetItemParent(mcRootId);
        wxTreeItemId busId = m_milStd1553Tree->GetItemParent(rtId);
        
        for(int i = 0; i < BUS_COUNT; ++i) {
            if (model.busList.at(i).getTreeObject() == busId) {
                filterBusChar = (i == 0) ? 'A' : 'B';
                for (int j = 0; j < RT_COUNT; ++j) {
                    if (model.busList.at(i).rtList.at(j).getTreeObject() == rtId) {
                        filterRt = j;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
        }
    } else {
        // Find Bus/RT/SA
        for (int i = 0; i < BUS_COUNT && !found; ++i) {
            auto& bus = model.busList.at(i);
            if (bus.getTreeObject() == clickedId) { filterBusChar = (i == 0) ? 'A' : 'B'; found = true; break; }
            for (size_t j = 0; j < bus.rtList.size() && !found; ++j) {
                auto& rt = bus.rtList.at(j);
                if (rt.getTreeObject() == clickedId) { filterBusChar = (i == 0) ? 'A' : 'B'; filterRt = (int)j; found = true; break; }
                for (size_t k = 0; k < rt.saList.size() && !found; ++k) {
                    auto& sa = rt.saList.at(k);
                    if (sa.getTreeObject() == clickedId) { filterBusChar = (i == 0) ? 'A' : 'B'; filterRt = (int)j; filterSa = (int)k; found = true; break; }
                }
            }
        }
    }

    if (found) {
        BM::getInstance().setFilterCriteria(filterBusChar, filterRt, filterSa, filterMc);
        BM::getInstance().enableFilter(true);
        wxString filterLabel = "Filtering by: ";
        if(filterBusChar != 0) filterLabel += wxString::Format("Bus %c", filterBusChar);
        if(filterRt != -1) filterLabel += wxString::Format(", RT %d", filterRt);
        if(filterSa != -1) filterLabel += wxString::Format(", SA %d", filterSa);
        if(filterMc != -1) filterLabel += wxString::Format(", MC %d", filterMc);
        m_filterButton->SetLabelText(filterLabel);
        m_filterButton->Enable(true);
        resetTreeVisualState();
        m_milStd1553Tree->SetItemBold(clickedId, true);
        m_milStd1553Tree->EnsureVisible(clickedId);
        setStatusText(filterLabel);
    }
}

void BusMonitorPanel::onStatusTimer(wxTimerEvent &) {
    if (BM::getInstance().isMonitoring()) {
        int sniffedCount = BM::getInstance().getTcpProxy().getConnectedClientCount();
        setStatusText(wxString::Format("Passive Monitoring Active. Sniffing traffic on %s:%u", m_host, m_port));
    }
}

void BusMonitorPanel::resetTreeVisualState() {
    auto& model = MilStd1553::getInstance();
    wxColour defaultColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    for (const auto& bus : model.busList) {
        if(bus.getTreeObject().IsOk()) { 
            m_milStd1553Tree->SetItemBold(bus.getTreeObject(), false); 
            m_milStd1553Tree->SetItemTextColour(bus.getTreeObject(), defaultColour); 
        }
        for (const auto& rt : bus.rtList) {
            if(rt.getTreeObject().IsOk()) { 
                m_milStd1553Tree->SetItemBold(rt.getTreeObject(), false); 
                m_milStd1553Tree->SetItemTextColour(rt.getTreeObject(), defaultColour); 
            }
            for (const auto& sa : rt.saList) {
                if(sa.getTreeObject().IsOk()) { 
                    m_milStd1553Tree->SetItemBold(sa.getTreeObject(), false); 
                    m_milStd1553Tree->SetItemTextColour(sa.getTreeObject(), defaultColour); 
                }
            }
        }
    }
}

void BusMonitorPanel::updateTreeItemVisualState(char bus, int rt, int sa, bool isActive) {
    auto& model = MilStd1553::getInstance();
    int bus_idx = (toupper(bus) == 'A') ? 0 : 1;
    if (bus_idx >= BUS_COUNT || rt >= RT_COUNT || sa >= SA_COUNT) return;

    wxTreeItemId busTreeId = model.busList.at(bus_idx).getTreeObject();
    wxTreeItemId rtTreeId  = model.busList.at(bus_idx).rtList.at(rt).getTreeObject();
    wxTreeItemId saTreeId  = model.busList.at(bus_idx).rtList.at(rt).saList.at(sa).getTreeObject();

    if (isActive) {
        if (saTreeId.IsOk())  m_milStd1553Tree->SetItemTextColour(saTreeId, *wxGREEN);
        if (rtTreeId.IsOk())  m_milStd1553Tree->SetItemTextColour(rtTreeId, *wxGREEN);
        if (busTreeId.IsOk()) m_milStd1553Tree->SetItemTextColour(busTreeId, *wxGREEN);
        
        if (saTreeId.IsOk())  m_milStd1553Tree->SetItemBold(saTreeId, true);
        if (rtTreeId.IsOk())  m_milStd1553Tree->SetItemBold(rtTreeId, true);
        if (busTreeId.IsOk()) m_milStd1553Tree->SetItemBold(busTreeId, true);
    } 
}

void BusMonitorPanel::onLogToFileToggled(wxCommandEvent &event) {
    BM::getInstance().enableDataLogging(event.IsChecked());
}
