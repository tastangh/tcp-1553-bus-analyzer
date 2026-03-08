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

    BusControllerPanel* m_parentPanel;
    FrameComponent* m_sourceComponent;

    wxTextCtrl* m_label;
    wxChoice* m_busChoice;
    wxChoice* m_modeChoice;
    wxSpinCtrl* m_rt1;
    wxSpinCtrl* m_sa1;
    wxSpinCtrl* m_rt2;
    wxSpinCtrl* m_sa2;
    wxSpinCtrl* m_wc;
    std::vector<wxTextCtrl*> m_dataFields;
    wxBoxSizer* m_dataSizer;
};
