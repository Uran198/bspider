#ifndef _MAIN_FRAME_H_
#define _MAIN_FRAME_H_

#include "wx/wx.h"

// to use STL
#ifdef new
#undef new
#endif
#include <thread>
#include <mutex>

class wxGrid;
class Spider;

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);
    ~MainFrame();

private:
    wxStaticText* m_url_label;
    wxStaticText* m_threads_count_label;
    wxStaticText* m_search_text_label;
    wxStaticText* m_max_url_scan_label;

    wxTextCtrl* m_url_entry;
    wxTextCtrl* m_threads_count_entry;
    wxTextCtrl* m_search_text_entry;
    wxTextCtrl* m_max_url_scan_entry;

    wxButton* m_start_button;
    wxButton* m_stop_button;

    std::mutex m_result_grid_lock;
    wxGrid* m_result_grid;
    Spider* m_spider;

    std::thread m_grid_updater;

private:
    void grid_update();

private:
    void on_stop(wxCommandEvent& event);
    void on_start(wxCommandEvent& event);

private:
    DECLARE_EVENT_TABLE()

    enum { BUTTON_Start = 1,
           BUTTON_Stop };
};

#endif
