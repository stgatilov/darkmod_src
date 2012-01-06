/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "precompiled_engine.h"
#pragma hdrstop

#include "../../sys/win32/rc/debugger_resource.h"
#include "DebuggerApp.h"

/*
================
rvDebuggerApp::rvDebuggerApp
================
*/
rvDebuggerApp::rvDebuggerApp ( ) :
	mOptions ( "Software\\id Software\\DOOM3\\Tools\\Debugger" )
{
	mInstance		= NULL;
	mDebuggerWindow = NULL;
	mAccelerators   = NULL;
}

/*
================
rvDebuggerApp::~rvDebuggerApp
================
*/
rvDebuggerApp::~rvDebuggerApp ( )
{
	if ( mAccelerators )
	{
		DestroyAcceleratorTable ( mAccelerators );
	}
}

/*
================
rvDebuggerApp::Initialize

Initializes the debugger application by creating the debugger window
================
*/
bool rvDebuggerApp::Initialize ( HINSTANCE instance )
{
	INITCOMMONCONTROLSEX ex;
	ex.dwICC = ICC_USEREX_CLASSES | ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES;
	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);

	mInstance = instance;

	mOptions.Load ( );

	mDebuggerWindow = new rvDebuggerWindow;
		
	if ( !mDebuggerWindow->Create ( instance ) )
	{
		delete mDebuggerWindow;
		return false;
	}

	// Initialize the network connection for the debugger
	if ( !mClient.Initialize ( ) )
	{
		return false;
	}	

	mAccelerators = LoadAccelerators ( mInstance, MAKEINTRESOURCE(IDR_DBG_ACCELERATORS) );

	return true;
}

/*
================
rvDebuggerApp::ProcessWindowMessages

Process windows messages
================
*/
bool rvDebuggerApp::ProcessWindowMessages ( void )
{
	MSG	msg;

	while ( PeekMessage ( &msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		if ( !GetMessage (&msg, NULL, 0, 0) ) 
		{
			return false;
		}
		
		if ( !TranslateAccelerator ( &msg ) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return true;
}

/*
================
rvDebuggerApp::TranslateAccelerator

Translate any accelerators destined for this window
================
*/
bool rvDebuggerApp::TranslateAccelerator ( LPMSG msg )
{
	if ( mDebuggerWindow && ::TranslateAccelerator ( mDebuggerWindow->GetWindow(), mAccelerators, msg ) )
	{
		return true;
	}
		
	return false;
}

/*
================
rvDebuggerApp::Run

Main Loop for the debugger application
================
*/
int rvDebuggerApp::Run ( void )
{		
	// Main message loop:
	while ( ProcessWindowMessages ( ) )
	{
		mClient.ProcessMessages ( );
		
		Sleep ( 0 );
	}
	
	mClient.Shutdown ( );
	mOptions.Save ( );
	
	delete mDebuggerWindow;
	
	return 1;
}

