#include "FrameComponent.hpp"
#include "BusControllerPanel.hpp"
#include "CreateFrameWindow.hpp"
#include "bc.hpp"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

FrameComponent::FrameComponent(wxWindow* parent, FrameConfig config)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_RAISED), m_config(config) {
    
    SetBackgroundColour(wxColour("#ffffff"));
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Status/Toggle Section
    m_activeToggle = new wxToggleButton(this, wxID_ANY, "ENABLE", wxDefaultPosition, wxSize(80, 40));
    m_activeToggle->SetValue(true);
    m_activeToggle->SetBackgroundColour(wxColour("#eeeeee"));
    m_activeToggle->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    mainSizer->Add(m_activeToggle, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);

    // Summary/Word Section
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);
    m_label = new wxStaticText(this, wxID_ANY, config.label.empty() ? wxString::Format("RT:%02d SA:%02d", config.rt, config.sa) : wxString::FromUTF8(config.label.c_str()));
    m_label->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    infoSizer->Add(m_label, 0, wxBOTTOM, 2);

    // Word Visualization (AIM Style)
    auto* wordsSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Info details
    wxString info;
    info.Printf("Bus %c | WC %d | ", config.bus, config.wc);
    auto* infoText = new wxStaticText(this, wxID_ANY, info);
    infoText->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    wordsSizer->Add(infoText, 0, wxALIGN_CENTER_VERTICAL);

    // Calculate Command Word (Simplified)
    uint16_t cmdVal = (config.rt << 11) | (config.sa << 5) | (config.wc & 0x1F);
    auto* cmdText = new wxStaticText(this, wxID_ANY, wxString::Format("[%04X]", cmdVal));
    cmdText->SetForegroundColour(wxColour("#0000ff")); // Blue for Command
    cmdText->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    wordsSizer->Add(cmdText, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 8);

    // Data Words (First 8)
    int wcToShow = (config.wc == 0) ? 32 : config.wc;
    for (int i = 0; i < wcToShow && i < 8; ++i) {
        auto* dataText = new wxStaticText(this, wxID_ANY, wxString::Format("%s", config.data[i]));
        dataText->SetForegroundColour(wxColour("#008800")); // Green for Data
        dataText->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        wordsSizer->Add(dataText, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
    }
    if (wcToShow > 8) {
        wordsSizer->Add(new wxStaticText(this, wxID_ANY, "..."), 0, wxALIGN_CENTER_VERTICAL);
    }
    
    infoSizer->Add(wordsSizer, 0, wxTOP, 2);
    mainSizer->Add(infoSizer, 1, wxEXPAND | wxALL, 10);

    // Actions
    auto* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    m_editButton = new wxButton(this, wxID_ANY, "Edit", wxDefaultPosition, wxSize(60, 30));
    m_deleteButton = new wxButton(this, wxID_ANY, "X", wxDefaultPosition, wxSize(30, 30));
    m_deleteButton->SetForegroundColour(*wxRED);

    actionSizer->Add(m_editButton, 0, wxRIGHT, 5);
    actionSizer->Add(m_deleteButton, 0);
    mainSizer->Add(actionSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    SetSizer(mainSizer);

    m_editButton->Bind(wxEVT_BUTTON, &FrameComponent::onEdit, this);
    m_deleteButton->Bind(wxEVT_BUTTON, &FrameComponent::onDelete, this);
}

void FrameComponent::sendFrame() {
    std::array<uint16_t, BC_MAX_DATA_WORDS> received;
    int ret = BusController::getInstance().sendAcyclicFrame(this, received);
}

void FrameComponent::onEdit(wxCommandEvent& event) {
    auto* parent = dynamic_cast<BusControllerPanel*>(GetParent()->GetParent());
    if (parent) {
        auto* frame = new FrameCreationFrame(parent, this);
        frame->Show(true);
    }
}

void FrameComponent::onDelete(wxCommandEvent& event) {
    auto* parent = dynamic_cast<BusControllerPanel*>(GetParent()->GetParent());
    if (parent) {
        parent->removeFrame(this);
    }
}
