/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/


// tdm_update_mfc.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "tdm_update_mfc.h"
#include "UpdaterDialog.h"
#include "LogWriters.h"
#include "CommandLineInfo.h"

#include "Constants.h"
#include <boost/format.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// UpdaterApplication

BEGIN_MESSAGE_MAP(UpdaterApplication, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// UpdaterApplication construction

UpdaterApplication::UpdaterApplication()
{}

// The one and only UpdaterApplication object

UpdaterApplication theApp;

using namespace tdm;

// UpdaterApplication initialization
BOOL UpdaterApplication::InitInstance()
{
	// Start logging
	RegisterLogWriters();

	TraceLog::WriteLine(LOG_STANDARD, 
		(boost::format("TDM Updater v%s (c) 2009-2010 by tels & greebo. Part of The Dark Mod (http://www.thedarkmod.com).") % LIBTDM_UPDATE_VERSION).str());
	TraceLog::WriteLine(LOG_STANDARD, "");

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need

	WCHAR szFileName[MAX_PATH];
    HINSTANCE hInstance = GetModuleHandle(NULL);

    GetModuleFileName(hInstance, szFileName, MAX_PATH);

	std::wstring ws(szFileName);
	std::string str;
	str.assign(ws.begin(), ws.end()); 

	fs::path executableName = fs::path(str).leaf();

	// Parse the command line into our program options
	tdm::updater::CommandLineInfo commandLine;
	ParseCommandLine(commandLine);

	UpdaterDialog dlg(executableName, commandLine.options);
	m_pMainWnd = &dlg;

	dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
