/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod Updater (http://www.thedarkmod.com/)
 
******************************************************************************/


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
	try
    {
		RegisterLogWriters();
	} 
	catch (FileOpenException& )
    {
		// basic error-checking - most common cause of failure at this point is a read-only directory or log file.
		// if other issues are detected by the users, this will have to be expanded to cater for other types of errors
		AfxMessageBox(L"TDM Updater Error:\nUnable to open log file and start updater. Please ensure that the current directory is not set to 'Read-only'.\n\n"
					  L"If tdm_update.log already exists, please ensure that it is not 'Read-only'. This may also be caused by the write protections "
					  L"placed on the contents of the 'Program Files' and 'Program Files (x86)' directories.", MB_OK | MB_ICONSTOP);
		return EXIT_FAILURE;
	}

	TraceLog::WriteLine(LOG_STANDARD, 
        (boost::format("TDM Updater v%s/%d (c) 2009-2017 by tels & greebo. Part of The Dark Mod (http://www.thedarkmod.com).") % LIBTDM_UPDATE_VERSION % (sizeof(void*) * 8)).str());
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

	fs::path executableName = fs::path(str).filename();

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
