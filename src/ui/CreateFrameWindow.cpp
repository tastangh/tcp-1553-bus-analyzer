#include "CreateFrameWindow.hpp"
#include "BusControllerPanel.hpp"
#include "FrameComponent.hpp"
#include <wx/valnum.h>

FrameCreationFrame::FrameCreationFrame(BusControllerPanel* parent, FrameComponent* source)
    : wxFrame(parent, wxID_ANY, source ? "Edit Frame" : "Add New Frame", wxDefaultPosition, wxSize(500, 600)),
      m_parentPanel(parent), m_sourceComponent(source) {
    
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* gridSizer = new wxFlexGridSizer(2, 10, 10);
    gridSizer->AddGrowableCol(1);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Label:"));
    m_label = new wxTextCtrl(this, wxID_ANY, source ? source->getFrameConfig().label : "New Frame");
    gridSizer->Add(m_label, 1, wxEXPAND);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Bus:"));
    wxArrayString buses; buses.Add("A"); buses.Add("B");
    m_busChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, buses);
    m_busChoice->SetSelection(source && source->getFrameConfig().bus == 'B' ? 1 : 0);
    gridSizer->Add(m_busChoice, 0);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Mode:"));
    wxArrayString modes; 
    modes.Add("BC to RT"); modes.Add("RT to BC"); modes.Add("RT to RT");
    modes.Add("Mode Code (No Data)"); modes.Add("Mode Code (With Data)");
    m_modeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, modes);
    m_modeChoice->SetSelection(source ? (int)source->getFrameConfig().mode : 0);
    gridSizer->Add(m_modeChoice, 0);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, "RT address:"));
    m_rt1 = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 31, source ? source->getFrameConfig().rt : 1);
    gridSizer->Add(m_rt1, 0);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Subaddress:"));
    m_sa1 = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 31, source ? source->getFrameConfig().sa : 1);
    gridSizer->Add(m_sa1, 0);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Word Count:"));
    m_wc = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 32, source ? source->getFrameConfig().wc : 1);
    gridSizer->Add(m_wc, 0);

    mainSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 15);

    auto* dataLabel = new wxStaticText(this, wxID_ANY, "Data Words (Hex):");
    mainSizer->Add(dataLabel, 0, wxLEFT | wxRIGHT | wxTOP, 15);

    auto* scrolledData = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 200));
    m_dataSizer = new wxBoxSizer(wxVERTICAL);
    for (int i = 0; i < 32; ++i) {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(scrolledData, wxID_ANY, wxString::Format("Word %02d: ", i + 1)), 0, wxALIGN_CENTER_VERTICAL);
        auto* text = new wxTextCtrl(scrolledData, wxID_ANY, source ? source->getFrameConfig().data[i] : "0000");
        m_dataFields.push_back(text);
        row->Add(text, 1, wxEXPAND | wxLEFT, 5);
        m_dataSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 2);
    }
    scrolledData->SetSizer(m_dataSizer);
    scrolledData->SetScrollRate(0, 20);
    mainSizer->Add(scrolledData, 1, wxEXPAND | wxALL, 15);

    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* saveBtn = new wxButton(this, wxID_ANY, "Save");
    auto* cancelBtn = new wxButton(this, wxID_ANY, "Cancel");
    buttonSizer->Add(saveBtn, 0, wxALL, 10);
    buttonSizer->Add(cancelBtn, 0, wxALL, 10);
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT);

    SetSizer(mainSizer);

    saveBtn->Bind(wxEVT_BUTTON, &FrameCreationFrame::onSave, this);
    cancelBtn->Bind(wxEVT_BUTTON, &FrameCreationFrame::onCancel, this);
    m_modeChoice->Bind(wxEVT_CHOICE, &FrameCreationFrame::onModeChanged, this);

    Centre();
}

void FrameCreationFrame::onSave(wxCommandEvent& event) {
    FrameConfig cfg;
    cfg.label = m_label->GetValue().ToStdString();
    cfg.bus = (char)m_busChoice->GetStringSelection()[0];
    cfg.mode = (BcMode)m_modeChoice->GetSelection();
    cfg.rt = m_rt1->GetValue();
    cfg.sa = m_sa1->GetValue();
    cfg.wc = m_wc->GetValue();
    
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

void FrameCreationFrame::onCancel(wxCommandEvent& event) {
    Close();
}

void FrameCreationFrame::onModeChanged(wxCommandEvent& event) {
    // Show/hide RT2/SA2 if RT-to-RT mode is selected (to be implemented)
}
