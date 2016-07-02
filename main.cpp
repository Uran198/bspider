#include "wx/wx.h"

// to use STL
#ifdef new
#undef new
#endif
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <queue>

#include "MainFrame.h"

class MainApp : public wxApp {
public:
    virtual bool OnInit();
};

bool MainApp::OnInit()
{
    MainFrame* frame = new MainFrame(wxT("Spider"));
    frame->Show(true);
    return true;
}

wxIMPLEMENT_APP(MainApp);
