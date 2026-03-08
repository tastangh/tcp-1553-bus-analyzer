#include "FrameComponent.hpp"
#include "BusControllerPanel.hpp"
#include "CreateFrameWindow.hpp"
#include "bc.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/tglbtn.h>
#include <sstream>
#include <iomanip>

FrameComponent::FrameComponent(wxWindow* parent, FrameConfig config)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_RAISED), m_config(config) {
    
    SetBackgroundColour(wxColour("#ffffff"));
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left Section: Summary Text & Command Word (Vertical Sizer)
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);
    
    m_label = new wxStaticText(this, wxID_ANY, config.label.empty() ? "Untitled Frame" : wxString::FromUTF8(config.label.c_str()));
    m_label->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    leftSizer->Add(m_label, 0, wxBOTTOM, 2);

    m_summaryText = new wxStaticText(this, wxID_ANY, "");
    m_summaryText->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    leftSizer->Add(m_summaryText, 0, wxBOTTOM, 5);

    mainSizer->Add(leftSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);

    // Center Section: Data Word Grid (8 columns x 4 rows)
    auto* dataGridSizer = new wxGridSizer(4, 8, 5, 10);
    for (int i = 0; i < 32; ++i) {
        m_dataLabels[i] = new wxStaticText(this, wxID_ANY, "----");
        m_dataLabels[i]->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        m_dataLabels[i]->SetForegroundColour(wxColour("#008800")); // Green for data
        dataGridSizer->Add(m_dataLabels[i], 0, wxALIGN_CENTER);
    }
    mainSizer->Add(dataGridSizer, 1, wxEXPAND | wxALL, 10);

    // Right Section: Action Buttons (Vertical)
    auto* actionSizer = new wxBoxSizer(wxVERTICAL);
    
    m_activeToggle = new wxToggleButton(this, wxID_ANY, "Activate", wxDefaultPosition, wxSize(100, 28));
    m_activeToggle->SetValue(true);
    m_activeToggle->SetLabel("Active"); // Default to Active/Enable since we just added it
    
    m_editButton = new wxButton(this, wxID_ANY, "Edit", wxDefaultPosition, wxSize(100, 28));
    auto* sendBtn = new wxButton(this, wxID_ANY, "Send Once", wxDefaultPosition, wxSize(100, 28));
    m_deleteButton = new wxButton(this, wxID_ANY, "Remove", wxDefaultPosition, wxSize(100, 28));

    actionSizer->Add(m_activeToggle, 0, wxBOTTOM, 2);
    actionSizer->Add(m_editButton, 0, wxBOTTOM, 2);
    actionSizer->Add(sendBtn, 0, wxBOTTOM, 2);
    actionSizer->Add(m_deleteButton, 0, 0);
    
    mainSizer->Add(actionSizer, 0, wxALIGN_TOP | wxALL, 5);

    SetSizer(mainSizer);

    m_editButton->Bind(wxEVT_BUTTON, &FrameComponent::onEdit, this);
    m_deleteButton->Bind(wxEVT_BUTTON, &FrameComponent::onDelete, this);
    m_activeToggle->Bind(wxEVT_TOGGLEBUTTON, &FrameComponent::onToggle, this);
    sendBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->sendFrame(); });

    updateValues(config);
}

void FrameComponent::updateValues(const FrameConfig& config) {
    m_config = config;
    
    // Update Label
    m_label->SetLabel(config.label.empty() ? "Untitled Frame" : wxString::FromUTF8(config.label.c_str()));

    // Update Summary
    std::stringstream ss;
    ss << "Bus: " << config.bus << " | ";
    bool isMc = (config.mode == BcMode::MODE_CODE_NO_DATA || config.mode == BcMode::MODE_CODE_WITH_DATA);
    
    switch (config.mode) {
        case BcMode::BC_TO_RT: 
            ss << "BC -> RT " << config.rt << " | SA " << config.sa << " | WC " << config.wc; break;
        case BcMode::RT_TO_BC: 
            ss << "RT " << config.rt << " -> BC | SA " << config.sa << " | WC " << config.wc; break;
        case BcMode::RT_TO_RT: 
            ss << "RT " << config.rt << "(SA" << config.sa << ") -> RT " << config.rt2 << "(SA" << config.sa2 << ") | WC " << config.wc; break;
        default: 
            ss << "BC -> RT " << config.rt << " | MC " << config.sa << (config.mode == BcMode::MODE_CODE_WITH_DATA ? " (w/ data)" : ""); break;
    }
    
    // Add Command Word visualization in brackets
    uint16_t cmdVal = (config.rt << 11) | (config.sa << 5) | (config.wc & 0x1F);
    ss << " [" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << cmdVal << "]";
    
    m_summaryText->SetLabel(ss.str());

    // Update Data Grid
    int wordsToShow = 0;
    if (!isMc) {
        wordsToShow = (config.wc == 0) ? 32 : config.wc;
    } else if (config.mode == BcMode::MODE_CODE_WITH_DATA) {
        wordsToShow = 1;
    }

    for (int i = 0; i < 32; ++i) {
        if (i < wordsToShow) {
            m_dataLabels[i]->SetLabel(config.data[i]);
            m_dataLabels[i]->Show(true);
        } else {
            m_dataLabels[i]->SetLabel("");
            m_dataLabels[i]->Show(false);
        }
    }

    Layout();
}

void FrameComponent::sendFrame() {
    auto& bc = BusController::getInstance();
    if (!bc.isInitialized()) {
        auto* parent = dynamic_cast<BusControllerPanel*>(GetParent()->GetParent());
        if (parent) {
            bc.initialize(parent->getHost().ToStdString(), parent->getPort());
        }
    }
    
    std::array<uint16_t, BC_MAX_DATA_WORDS> received;
    int ret = bc.sendAcyclicFrame(this, received);
    
    // If it's a receive mode, update the UI with received data
    if (ret == 0 && (m_config.mode == BcMode::RT_TO_BC || m_config.mode == BcMode::RT_TO_RT)) {
        for (int i = 0; i < 32; ++i) {
            std::stringstream ss;
            ss << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << received[i];
            m_config.data[i] = ss.str();
        }
        updateValues(m_config);
    }
}

void FrameComponent::onEdit(wxCommandEvent& event) {
    auto* parentPanel = dynamic_cast<BusControllerPanel*>(GetParent()->GetParent());
    if (parentPanel) {
        auto* frame = new FrameCreationFrame(parentPanel, this);
        frame->Show(true);
    }
}

void FrameComponent::onDelete(wxCommandEvent& event) {
    auto* parentPanel = dynamic_cast<BusControllerPanel*>(GetParent()->GetParent());
    if (parentPanel) {
        parentPanel->removeFrame(this);
    }
}

void FrameComponent::onToggle(wxCommandEvent& event) {
    m_activeToggle->SetLabel(m_activeToggle->GetValue() ? "Active" : "Activate");
}
