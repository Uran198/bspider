#include "MainFrame.h"

#include <wx/grid.h>
#include <wx/protocol/http.h>

#include <string>
#include <sstream>

#include "Spider.h"

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_BUTTON(BUTTON_Start, MainFrame::on_start)
EVT_BUTTON(BUTTON_Stop, MainFrame::on_stop)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame((wxFrame*)NULL, wxID_ANY, title, wxPoint(wxID_ANY, wxID_ANY), wxSize(340, 450), wxSYSTEM_MENU | wxMINIMIZE_BOX | wxCLOSE_BOX | wxCAPTION | wxCLIP_CHILDREN)
{
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* url_box = new wxBoxSizer(wxHORIZONTAL);
    m_url_label = new wxStaticText(panel, wxID_ANY, wxT("Start url: "), wxDefaultPosition);
    m_url_entry = new wxTextCtrl(panel, wxID_ANY);

    url_box->Add(m_url_label, 0);
    url_box->Add(m_url_entry, 1);

    wxBoxSizer* thread_count_box = new wxBoxSizer(wxHORIZONTAL);
    m_threads_count_label = new wxStaticText(panel, wxID_ANY, wxT("Number of threads: "), wxDefaultPosition);
    m_threads_count_entry = new wxTextCtrl(panel, wxID_ANY);

    thread_count_box->Add(m_threads_count_label, 0);
    thread_count_box->Add(m_threads_count_entry, 1);

    wxBoxSizer* search_text_box = new wxBoxSizer(wxHORIZONTAL);
    m_search_text_label = new wxStaticText(panel, wxID_ANY, wxT("Search text: "), wxDefaultPosition);
    m_search_text_entry = new wxTextCtrl(panel, wxID_ANY);

    search_text_box->Add(m_search_text_label, 0);
    search_text_box->Add(m_search_text_entry, 1);

    wxBoxSizer* max_url_scan_box = new wxBoxSizer(wxHORIZONTAL);
    m_max_url_scan_label = new wxStaticText(panel, wxID_ANY, wxT("Max urls to scan: "), wxDefaultPosition);
    m_max_url_scan_entry = new wxTextCtrl(panel, wxID_ANY);

    max_url_scan_box->Add(m_max_url_scan_label, 0);
    max_url_scan_box->Add(m_max_url_scan_entry, 1);

    wxBoxSizer* buttons_box = new wxBoxSizer(wxHORIZONTAL);
    m_start_button = new wxButton(panel, BUTTON_Start, wxT("Start"));
    m_stop_button = new wxButton(panel, BUTTON_Stop, wxT("Stop"));

    buttons_box->Add(m_start_button, 0);
    buttons_box->Add(m_stop_button, 1);

    vbox->Add(url_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP);
    vbox->Add(thread_count_box, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP);
    vbox->Add(search_text_box, 2, wxEXPAND | wxLEFT | wxRIGHT | wxTOP);
    vbox->Add(max_url_scan_box, 3, wxEXPAND | wxLEFT | wxRIGHT | wxTOP);
    vbox->Add(buttons_box, 4, wxEXPAND | wxLEFT | wxRIGHT | wxTOP);

    m_result_grid = new wxGrid(panel, wxID_ANY, wxDefaultPosition, wxSize(500, 500));

    m_result_grid->CreateGrid(0, 2);

    m_result_grid->SetRowSize(0, 30);
    m_result_grid->SetColSize(0, 150);
    m_result_grid->SetColSize(1, 80);

    m_result_grid->SetColLabelValue(0, "URL");
    m_result_grid->SetColLabelValue(1, "Status");

    vbox->Add(m_result_grid, 5, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM);

    panel->SetSizer(vbox);
    Centre();

    wxHTTP::Initialize();
    m_spider = new Spider;
    m_grid_updater = std::thread(&MainFrame::grid_update, this);
}

MainFrame::~MainFrame()
{
    delete m_spider;
    m_spider = NULL;
}

#include <iostream>

void MainFrame::on_start(wxCommandEvent& event)
{
    if (!m_spider || m_spider->is_running())
        return;
    std::stringstream ss;
    ss << m_threads_count_entry->GetValue().c_str();
    int thread_count = 0;
    ss >> thread_count;
    if (thread_count <= 0)
        return;

    ss.clear();
    ss << m_max_url_scan_entry->GetValue().c_str();
    int max_url_scan = 0;
    ss >> max_url_scan;
    if (max_url_scan <= 0)
        return;

    {
        std::lock_guard<std::mutex> lock(m_result_grid_lock);
        m_result_grid->DeleteRows(0, m_result_grid->GetNumberRows());
    }
    m_spider->stop();
    m_spider->set_start_url(m_url_entry->GetValue().ToStdString());
    m_spider->set_search_text(m_search_text_entry->GetValue().ToStdString());
    m_spider->set_thread_count(thread_count);
    m_spider->set_max_scan_urls(max_url_scan);
    m_spider->start();
}

void MainFrame::on_stop(wxCommandEvent& event)
{
    if (!m_spider || !m_spider->is_running())
        return;
    m_spider->stop();
}

void MainFrame::grid_update()
{
    const std::chrono::duration<double, std::milli> sleep_time(300);
    while (true) {
        while (!m_spider || !m_spider->has_results()) {
            std::this_thread::sleep_for(sleep_time);
        }
        ScanURL ret = m_spider->get_next_result();
        int row = ret.id;
        std::lock_guard<std::mutex> lock(m_result_grid_lock);
        if (m_result_grid->GetNumberRows() <= row) {
            m_result_grid->AppendRows(row - m_result_grid->GetNumberRows() + 1);
        }
        m_result_grid->SetCellValue(row, 0, ret.url);
        m_result_grid->SetCellValue(row, 1, ret.message);
        m_result_grid->SetReadOnly(row, 1);
    }
}
