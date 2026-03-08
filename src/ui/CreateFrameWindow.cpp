#include "CreateFrameWindow.hpp"
#include "BusControllerPanel.hpp"
#include "FrameComponent.hpp"
#include <wx/valnum.h>
#include <random>
#include <iomanip>
#include <sstream>

FrameCreationFrame::FrameCreationFrame(BusControllerPanel* parent, FrameComponent* source)
    : wxFrame(parent, wxID_ANY, source ? "Edit 1553 Frame" : "Create New 1553 Frame", wxDefaultPosition, wxSize(680, 520)),
      m_parentPanel(parent), m_sourceComponent(source) {
    
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Preparation for ComboBoxes
    wxArrayString rtOptions;
    for (int i = 0; i < 32; ++i) rtOptions.Add(wxString::Format("%d", i));
    wxArrayString wcOptions;
    for (int i = 0; i <= 32; ++i) wcOptions.Add(wxString::Format("%d", i));

    // Top Row: Bus, Mode, RT, SA, WC
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    
    topSizer->Add(new wxStaticText(this, wxID_ANY, "Bus:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    wxArrayString buses; buses.Add("A"); buses.Add("B");
    m_busChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(60, -1), buses);
    m_busChoice->SetSelection(source && source->getFrameConfig().bus == 'B' ? 1 : 0);
    topSizer->Add(m_busChoice, 0, wxLEFT, 5);

    topSizer->Add(new wxStaticText(this, wxID_ANY, "Mode:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15);
    wxArrayString modes; 
    modes.Add("BC to RT"); modes.Add("RT to BC"); modes.Add("RT to RT");
    modes.Add("Mode Code (No Data)"); modes.Add("Mode Code (With Data)");
    m_modeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, modes);
    m_modeChoice->SetSelection(source ? (int)source->getFrameConfig().mode : 0);
    topSizer->Add(m_modeChoice, 0, wxLEFT, 5);

    topSizer->Add(new wxStaticText(this, wxID_ANY, "RT:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15);
    m_rt1 = new wxComboBox(this, wxID_ANY, source ? wxString::Format("%d", source->getFrameConfig().rt) : "1", wxDefaultPosition, wxSize(75, -1), rtOptions, wxCB_READONLY);
    topSizer->Add(m_rt1, 0, wxLEFT, 5);

    m_sa1Label = new wxStaticText(this, wxID_ANY, "SA:");
    topSizer->Add(m_sa1Label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15);
    m_sa1 = new wxComboBox(this, wxID_ANY, source ? wxString::Format("%d", source->getFrameConfig().sa) : "1", wxDefaultPosition, wxSize(75, -1), rtOptions, wxCB_READONLY);
    topSizer->Add(m_sa1, 0, wxLEFT, 5);

    topSizer->Add(new wxStaticText(this, wxID_ANY, "WC:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15);
    m_wc = new wxComboBox(this, wxID_ANY, source ? wxString::Format("%d", source->getFrameConfig().wc) : "1", wxDefaultPosition, wxSize(75, -1), wcOptions, wxCB_READONLY);
    topSizer->Add(m_wc, 0, wxLEFT | wxRIGHT, 5);

    mainSizer->Add(topSizer, 0, wxEXPAND | wxTOP, 15);

    // RT-to-RT Row (Optional)
    m_cmdWord2Sizer = new wxBoxSizer(wxHORIZONTAL);
    m_rt2Label = new wxStaticText(this, wxID_ANY, "RT2:");
    m_cmdWord2Sizer->Add(m_rt2Label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    m_rt2 = new wxComboBox(this, wxID_ANY, source ? wxString::Format("%d", source->getFrameConfig().rt2) : "0", wxDefaultPosition, wxSize(75, -1), rtOptions, wxCB_READONLY);
    m_cmdWord2Sizer->Add(m_rt2, 0, wxLEFT, 5);

    m_sa2Label = new wxStaticText(this, wxID_ANY, "SA2:");
    m_cmdWord2Sizer->Add(m_sa2Label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 15);
    m_sa2 = new wxComboBox(this, wxID_ANY, source ? wxString::Format("%d", source->getFrameConfig().sa2) : "0", wxDefaultPosition, wxSize(75, -1), rtOptions, wxCB_READONLY);
    m_cmdWord2Sizer->Add(m_sa2, 0, wxLEFT, 5);
    
    mainSizer->Add(m_cmdWord2Sizer, 0, wxEXPAND | wxTOP, 10);

    // Data Word Grid (4x8)
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Data Words (Hex):"), 0, wxLEFT | wxTOP, 15);
    auto* dataGridSizer = new wxGridSizer(4, 8, 8, 8);
    for (int i = 0; i < 32; ++i) {
        auto* text = new wxTextCtrl(this, wxID_ANY, source ? source->getFrameConfig().data[i] : "0000", wxDefaultPosition, wxSize(65, -1));
        text->SetMaxLength(4);
        m_dataFields.push_back(text);
        dataGridSizer->Add(text, 0, wxEXPAND);
    }
    mainSizer->Add(dataGridSizer, 0, wxEXPAND | wxALL, 15);

    // Label Row
    auto* labelSizer = new wxBoxSizer(wxHORIZONTAL);
    labelSizer->Add(new wxStaticText(this, wxID_ANY, "Label:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    m_label = new wxTextCtrl(this, wxID_ANY, source ? source->getFrameConfig().label : "New Frame");
    labelSizer->Add(m_label, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
    mainSizer->Add(labelSizer, 0, wxEXPAND | wxTOP, 5);

    // Bottom Action Row
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* randomizeBtn = new wxButton(this, wxID_ANY, "Randomize Data");
    auto* saveBtn = new wxButton(this, wxID_OK, source ? "Save Changes" : "Add Frame");
    auto* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    bottomSizer->Add(randomizeBtn, 0, wxALL, 10);
    bottomSizer->AddStretchSpacer();
    bottomSizer->Add(cancelBtn, 0, wxALL, 10);
    bottomSizer->Add(saveBtn, 0, wxALL, 10);
    mainSizer->Add(bottomSizer, 0, wxEXPAND);

    SetSizerAndFit(mainSizer);
    
    randomizeBtn->Bind(wxEVT_BUTTON, &FrameCreationFrame::onRandomize, this);
    saveBtn->Bind(wxEVT_BUTTON, &FrameCreationFrame::onSave, this);
    cancelBtn->Bind(wxEVT_BUTTON, &FrameCreationFrame::onCancel, this);
    m_modeChoice->Bind(wxEVT_CHOICE, &FrameCreationFrame::onModeChanged, this);
    m_wc->Bind(wxEVT_COMBOBOX, &FrameCreationFrame::onModeChanged, this);
    m_wc->Bind(wxEVT_TEXT, &FrameCreationFrame::onModeChanged, this);

    updateControlStates();
    Centre();
}

void FrameCreationFrame::updateControlStates() {
    BcMode mode = (BcMode)m_modeChoice->GetSelection();
    bool isRt2Rt = (mode == BcMode::RT_TO_RT);
    bool isMc = (mode == BcMode::MODE_CODE_NO_DATA || mode == BcMode::MODE_CODE_WITH_DATA);
    
    m_cmdWord2Sizer->ShowItems(isRt2Rt);
    m_sa1Label->SetLabel(isMc ? "MC:" : "SA:");
    m_wc->Enable(!isMc);

    long wc_val = 0;
    m_wc->GetValue().ToLong(&wc_val);
    int wc = isMc ? 1 : (int)wc_val;
    if (wc == 0 && !isMc) wc = 32;

    bool dataVisible = (mode != BcMode::MODE_CODE_NO_DATA);
    for (int i = 0; i < 32; ++i) {
        m_dataFields[i]->Enable(dataVisible && (i < wc));
    }
    
    GetSizer()->Layout();
    // Don't call Fit() here as it might shrink the window too much when toggling RT2/SA2
    this->Layout();
}

void FrameCreationFrame::onSave(wxCommandEvent& event) {
    FrameConfig cfg;
    cfg.label = m_label->GetValue().ToStdString();
    if (cfg.label.empty()) cfg.label = "Untitled Frame";
    cfg.bus = (char)m_busChoice->GetStringSelection()[0];
    cfg.mode = (BcMode)m_modeChoice->GetSelection();
    
    long val;
    m_rt1->GetValue().ToLong(&val); cfg.rt = (int)val;
    m_sa1->GetValue().ToLong(&val); cfg.sa = (int)val;
    m_rt2->GetValue().ToLong(&val); cfg.rt2 = (int)val;
    m_sa2->GetValue().ToLong(&val); cfg.sa2 = (int)val;
    m_wc->GetValue().ToLong(&val); cfg.wc = (int)val;
    
    for (int i = 0; i < 32; ++i) {
        cfg.data[i] = m_dataFields[i]->GetValue().ToStdString();
    }

    if (m_sourceComponent) {
        m_parentPanel->updateFrame(m_sourceComponent, cfg);
    } else {
        m_parentPanel->addFrameToList(cfg);
    }
    Close();
}

void FrameCreationFrame::onRandomize(wxCommandEvent& event) {
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(0, 0xFFFF);
    
    for (auto* field : m_dataFields) {
        if (field->IsEnabled()) {
            std::stringstream ss;
            ss << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << distr(eng);
            field->SetValue(ss.str());
        }
    }
}

void FrameCreationFrame::onCancel(wxCommandEvent& event) {
    Close();
}

void FrameCreationFrame::onModeChanged(wxCommandEvent& event) {
    updateControlStates();
}
