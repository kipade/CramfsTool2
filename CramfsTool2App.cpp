/***************************************************************
 * Name:      CramfsTool2App.cpp
 * Purpose:   Code for Application Class
 * Author:     ()
 * Created:   2017-10-15
 * Copyright:  ()
 * License:
 **************************************************************/

#include "CramfsTool2App.h"

//(*AppHeaders
#include "CramfsTool2Main.h"
#include <wx/image.h>
//*)

IMPLEMENT_APP(CramfsTool2App);

bool CramfsTool2App::OnInit()
{
    //(*AppInitialize
    bool wxsOK = true;
    wxInitAllImageHandlers();
    if ( wxsOK )
    {
    	CramfsTool2Frame* Frame = new CramfsTool2Frame(0);
    	Frame->Show();
    	SetTopWindow(Frame);
    }
    //*)
    return wxsOK;

}
