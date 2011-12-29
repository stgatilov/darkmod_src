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
#ifndef DEBUGGERAPP_H_
#define DEBUGGERAPP_H_

#include "../../sys/win32/win_local.h"
#include "../../framework/sync/Msg.h"

#ifndef REGISTRYOPTIONS_H_
#include "../common/RegistryOptions.h"
#endif

#ifndef DEBUGGERWINDOW_H_
#include "DebuggerWindow.h"
#endif

#ifndef DEBUGGERMESSAGES_H_
#include "DebuggerMessages.h"
#endif

#ifndef DEBUGGERCLIENT_H_
#include "DebuggerClient.h"
#endif

// These were changed to static by ID so to make it easy we just throw them
// in this header
const int MAX_MSGLEN = 1400;

class rvDebuggerApp
{
public:

	rvDebuggerApp ( );

	bool				Initialize				( HINSTANCE hInstance );
	int					Run						( void );
	
	rvRegistryOptions&	GetOptions				( void );
	rvDebuggerClient&	GetClient				( void );
	rvDebuggerWindow&	GetWindow				( void );
	
	HINSTANCE			GetInstance				( void );

	bool				TranslateAccelerator	( LPMSG msg );
		
protected:

	rvRegistryOptions	mOptions;
	rvDebuggerWindow*	mDebuggerWindow;
	HINSTANCE			mInstance;
	rvDebuggerClient	mClient;
	HACCEL				mAccelerators;
	
private:

	bool	ProcessNetMessages		( void );
	bool	ProcessWindowMessages	( void );
};

ID_INLINE HINSTANCE rvDebuggerApp::GetInstance ( void )
{
	return mInstance;
}

ID_INLINE rvDebuggerClient& rvDebuggerApp::GetClient ( void )
{
	return mClient;
}

ID_INLINE rvRegistryOptions& rvDebuggerApp::GetOptions ( void )
{
	return mOptions;
}

ID_INLINE rvDebuggerWindow& rvDebuggerApp::GetWindow ( void )
{
	assert ( mDebuggerWindow );
	return *mDebuggerWindow;
}

extern rvDebuggerApp gDebuggerApp;

#endif // DEBUGGERAPP_H_
