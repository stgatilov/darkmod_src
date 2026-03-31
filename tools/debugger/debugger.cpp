/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"

#if defined( ID_ALLOW_TOOLS )
#include "DebuggerServer.h"
#include "../../sys/win32/rc/debugger_resource.h"
#include "DebuggerApp.h"
#else
#include "DebuggerServer.h"
#include "debugger_common.h"
#endif

#if defined( ID_ALLOW_TOOLS )
rvDebuggerApp					gDebuggerApp; // this is also used in other source files
static HWND						gDebuggerWindow = NULL;
#endif

static rvDebuggerServer*		gDebuggerServer			= NULL;
static uintptr_t				gDebuggerServerThread = 0;
static bool						gDebuggerServerQuit     = false;

#if defined( ID_ALLOW_TOOLS )
/*
================
DebuggerMain

Main entry point for the debugger application
================
*/
void DebuggerClientInit( const char *cmdline )
{
	// See if the debugger is already running
	if ( rvDebuggerWindow::Activate ( ) )
	{
		goto DebuggerClientInitDone;
	}

	if ( !gDebuggerApp.Initialize ( win32.hInstance ) )
	{
		goto DebuggerClientInitDone;
	}
	
	// hide the doom window by default
	::ShowWindow( win32.hWnd, SW_HIDE );

	gDebuggerApp.Run ( );

DebuggerClientInitDone:

	common->Quit();
}

/*
================
DebuggerLaunch

Launches another instance of the running executable with +debugger appended
to the end to indicate that the debugger should start up.
================
*/
void DebuggerClientLaunch ( void )
{
	if ( renderSystem->IsFullScreen() ) {
		common->Printf( "Cannot run the script debugger in fullscreen mode.\n"
					"Set r_fullscreen to 0 and vid_restart.\n" );
		return;
	}

	// See if the debugger is already running
	if ( rvDebuggerWindow::Activate ( ) ) {
		return;
	}

	char exeFile[MAX_PATH];
	char curDir[MAX_PATH];

	STARTUPINFO			startup;
	PROCESS_INFORMATION	process;

	ZeroMemory ( &startup, sizeof(startup) );
	startup.cb = sizeof(startup);

	GetCurrentDirectory ( MAX_PATH, curDir );

	GetModuleFileName ( NULL, exeFile, MAX_PATH );
	const char* s = va("%s +set fs_game %s +set fs_basepath %s +debugger", exeFile, cvarSystem->GetCVarString( "fs_game" ), cvarSystem->GetCVarString( "fs_basepath" ) );
	CreateProcess ( NULL, (LPSTR)s,
					NULL, NULL, FALSE, 0, NULL, curDir, &startup, &process );

	CloseHandle ( process.hThread );
	CloseHandle ( process.hProcess );
}
#endif // #if defined( ID_ALLOW_TOOLS )

/*
================
DebuggerServerThread

Thread proc for the debugger server
================
*/
static unsigned int DebuggerServerThread ( void *param )
{
	assert ( gDebuggerServer );

	while ( !gDebuggerServerQuit )
	{
		gDebuggerServer->ProcessMessages ( );
		Sys_Sleep(1);
	}

	return 0;
}

/*
================
DebuggerServerInit

Starts up the debugger server
================
*/
bool DebuggerServerInit ( void )
{
	com_enableDebuggerServer.ClearModified( );

	// Dont do this if we are in the debugger already
	if ( gDebuggerServer != NULL 
		|| ( com_editors & EDITOR_DEBUGGER ) )
	{
		return false;
	}

	// Allocate the new debugger server
	gDebuggerServer = new rvDebuggerServer;
	if ( !gDebuggerServer )
	{
		return false;
	}

	// Initialize the debugger server
	if ( !gDebuggerServer->Initialize ( ) )
	{
		delete gDebuggerServer;
		gDebuggerServer = NULL;
		return false;
	}
	
	// Start the debugger server thread
	gDebuggerServerThread = Sys_CreateThread(DebuggerServerThread, NULL, THREAD_NORMAL, "DebuggerServer");

	return true;
}

/*
================
DebuggerServerShutdown

Shuts down the debugger server
================
*/
void DebuggerServerShutdown ( void )
{
	if ( gDebuggerServerThread != 0 )
	{
		// Signal the debugger server to quit
		gDebuggerServerQuit = true;

		// Wait for the thread to finish
		Sys_DestroyThread( gDebuggerServerThread );
		gDebuggerServerThread = 0;

		// Shutdown the server now
		gDebuggerServer->Shutdown();

		delete gDebuggerServer;
		gDebuggerServer = NULL;

		com_editors &= ~EDITOR_DEBUGGER;
	}

	com_enableDebuggerServer.ClearModified( );
}

/*
================
DebuggerServerCheckBreakpoint

Check to see if there is a breakpoint associtated with this statement
================
*/
void DebuggerServerCheckBreakpoint ( idInterpreter* interpreter, idProgram* program, int instructionPointer )
{
	if ( !gDebuggerServer )
	{
		return;
	}

	gDebuggerServer->CheckBreakpoints ( interpreter, program, instructionPointer );
}

/*
================
DebuggerServerPrint

Sends a print message to the debugger client
================
*/
void DebuggerServerPrint ( const char* text )
{
	if ( !gDebuggerServer )
	{
		return;
	}

	gDebuggerServer->Print ( text );
}
