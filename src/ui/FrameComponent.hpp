#pragma once

#include "common.hpp"
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/tglbtn.h>
#include <wx/button.h>

class FrameComponent : public wxPanel {
public:
    FrameComponent(wxWindow* parent, FrameConfig config);
    
    bool isActive() const { return m_activeToggle->GetValue(); }
    void sendFrame();
    void updateValues(const FrameConfig& config);
    
    const FrameConfig& getFrameConfig() const { return m_config; }
    
    // AIM Compatibility (Placeholders)
    uint16_t getAimTransferId() const { return m_xferId; }
    uint16_t getAimHeaderId() const { return m_hdrId; }
    uint16_t getAimBufferId() const { return m_bufId; }
    void setAimIds(uint16_t xfer, uint16_t hdr, uint16_t buf) { m_xferId = xfer; m_hdrId = hdr; m_bufId = buf; }

private:
    void onEdit(wxCommandEvent& event);
    void onDelete(wxCommandEvent& event);
    void onToggle(wxCommandEvent& event);

    FrameConfig m_config;
    uint16_t m_xferId = 0, m_hdrId = 0, m_bufId = 0;

    wxStaticText* m_label;
    wxStaticText* m_summaryText;
    std::array<wxStaticText*, 32> m_dataLabels;
    wxToggleButton* m_activeToggle;
    wxButton* m_editButton;
    wxButton* m_deleteButton;
};
