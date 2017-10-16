/***************************************************************
 * Name:      CramfsTool2Main.cpp
 * Purpose:   Code for Application Frame
 * Author:     ()
 * Created:   2017-10-15
 * Copyright:  ()
 * License:
 **************************************************************/

#include "CramfsTool2Main.h"
#include <wx/msgdlg.h>

//(*InternalHeaders(CramfsTool2Frame)
#include <wx/intl.h>
#include <wx/string.h>
//*)

#include "CramfsFile.h"
#include <iostream>
using namespace std;

//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}

//(*IdInit(CramfsTool2Frame)
const long CramfsTool2Frame::idMenuOpen = wxNewId();
const long CramfsTool2Frame::idMenuQuit = wxNewId();
const long CramfsTool2Frame::idMenuAbout = wxNewId();
const long CramfsTool2Frame::ID_STATUSBAR1 = wxNewId();
//*)

BEGIN_EVENT_TABLE(CramfsTool2Frame,wxFrame)
    //(*EventTable(CramfsTool2Frame)
    //*)
END_EVENT_TABLE()

CramfsTool2Frame::CramfsTool2Frame(wxWindow* parent,wxWindowID id)
{
    //(*Initialize(CramfsTool2Frame)
    wxMenu* Menu1;
    wxMenu* Menu2;
    wxMenuBar* MenuBar1;
    wxMenuItem* MenuItem1;
    wxMenuItem* MenuItem2;

    Create(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("id"));
    MenuBar1 = new wxMenuBar();
    Menu1 = new wxMenu();
    MenuItem3 = new wxMenuItem(Menu1, idMenuOpen, _("Open\tCtrl+O"), _("Open a cramfs binary file"), wxITEM_NORMAL);
    Menu1->Append(MenuItem3);
    MenuItem1 = new wxMenuItem(Menu1, idMenuQuit, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
    Menu1->Append(MenuItem1);
    MenuBar1->Append(Menu1, _("&File"));
    Menu2 = new wxMenu();
    MenuItem2 = new wxMenuItem(Menu2, idMenuAbout, _("About\tF1"), _("Show info about this application"), wxITEM_NORMAL);
    Menu2->Append(MenuItem2);
    MenuBar1->Append(Menu2, _("Help"));
    SetMenuBar(MenuBar1);
    StatusBar1 = new wxStatusBar(this, ID_STATUSBAR1, 0, _T("ID_STATUSBAR1"));
    int __wxStatusBarWidths_1[1] = { -1 };
    int __wxStatusBarStyles_1[1] = { wxSB_NORMAL };
    StatusBar1->SetFieldsCount(1,__wxStatusBarWidths_1);
    StatusBar1->SetStatusStyles(1,__wxStatusBarStyles_1);
    SetStatusBar(StatusBar1);

    Connect(idMenuOpen, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&CramfsTool2Frame::OnOpen);
    Connect(idMenuQuit,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&CramfsTool2Frame::OnQuit);
    Connect(idMenuAbout,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&CramfsTool2Frame::OnAbout);
    //*)
}

CramfsTool2Frame::~CramfsTool2Frame()
{
    //(*Destroy(CramfsTool2Frame)
    //*)
}

void CramfsTool2Frame::OnOpen(wxCommandEvent& event)
{
    CramfsFile file("/home/kezhh/GPLSRC/Dev/Lib/test.cramfs");
    //bool ret = file.Exists("/.git/objects/ff/6e63a6f864e67f60aff3586419d017f9abfd08");
    size_t size;
    uint8_t* buffer;
    bool ret = file.ExtractFile("/mkcramfs.c", buffer, size);
    cout<<ret<<endl;
}

void CramfsTool2Frame::OnQuit(wxCommandEvent& event)
{
    Close();
}

void CramfsTool2Frame::OnAbout(wxCommandEvent& event)
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}
