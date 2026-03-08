#pragma once

#include "common.hpp"
#include <wx/wx.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>

class BusControllerPanel;
class FrameComponent;

class FrameCreationFrame : public wxFrame {
public:
    FrameCreationFrame(BusControllerPanel* parent, FrameComponent* source = nullptr);

private:
    void onSave(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onModeChanged(wxCommandEvent& event);
    void updateControlStates();
    void onRandomize(wxCommandEvent& event);

    BusControllerPanel* m_parentPanel;
    FrameComponent* m_sourceComponent;

    wxTextCtrl* m_label;
    wxChoice* m_busChoice;
    wxChoice* m_modeChoice;
    wxComboBox* m_rt1;
    wxComboBox* m_sa1;
    wxComboBox* m_rt2;
    wxComboBox* m_sa2;
    wxStaticText* m_sa1Label;
    wxStaticText* m_rt2Label;
    wxStaticText* m_sa2Label;
    wxBoxSizer* m_cmdWord2Sizer;
    wxComboBox* m_wc;
    std::vector<wxTextCtrl*> m_dataFields;
    wxBoxSizer* m_dataSizer;
};
