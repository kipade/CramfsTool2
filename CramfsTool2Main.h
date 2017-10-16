/***************************************************************
 * Name:      CramfsTool2Main.h
 * Purpose:   Defines Application Frame
 * Author:     ()
 * Created:   2017-10-15
 * Copyright:  ()
 * License:
 **************************************************************/

#ifndef CRAMFSTOOL2MAIN_H
#define CRAMFSTOOL2MAIN_H

//(*Headers(CramfsTool2Frame)
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
//*)

class CramfsTool2Frame: public wxFrame
{
    public:

        CramfsTool2Frame(wxWindow* parent,wxWindowID id = -1);
        virtual ~CramfsTool2Frame();

    private:

        //(*Handlers(CramfsTool2Frame)
        void OnOpen(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        //*)

        //(*Identifiers(CramfsTool2Frame)
        static const long idMenuOpen;
        static const long idMenuQuit;
        static const long idMenuAbout;
        static const long ID_STATUSBAR1;
        //*)

        //(*Declarations(CramfsTool2Frame)
        wxMenuItem* MenuItem3;
        wxStatusBar* StatusBar1;
        //*)

        DECLARE_EVENT_TABLE()
};

#endif // CRAMFSTOOL2MAIN_H
