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

static bool versioned = RegisterVersionedFile("$Id$");

//
// This file implements the low-level keyboard hook that traps the task keys.
//

//#define _WIN32_WINNT 0x0500 // for KBDLLHOOKSTRUCT
#include <afxwin.h>         // MFC core and standard components
#include "win_local.h"

#define DLLEXPORT __declspec(dllexport)

// Magic registry key/value for "Remove Task Manager" policy.
LPCTSTR KEY_DisableTaskMgr = "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
LPCTSTR VAL_DisableTaskMgr = "DisableTaskMgr";

// The section is SHARED among all instances of this DLL.
// A low-level keyboard hook is always a system-wide hook.
#pragma data_seg (".mydata")
HHOOK g_hHookKbdLL = NULL;	// hook handle
BOOL  g_bBeep = FALSE;		// beep on illegal key
#pragma data_seg ()
#pragma comment(linker, "/SECTION:.mydata,RWS") // tell linker: make it shared

/*
================
MyTaskKeyHookLL

  Low-level keyboard hook:
  Trap task-switching keys by returning without passing along.
================
*/
LRESULT CALLBACK MyTaskKeyHookLL( int nCode, WPARAM wp, LPARAM lp ) {
	KBDLLHOOKSTRUCT *pkh = (KBDLLHOOKSTRUCT *) lp;

	if ( nCode == HC_ACTION ) {
		BOOL bCtrlKeyDown = GetAsyncKeyState( VK_CONTROL)>>((sizeof(SHORT) * 8) - 1 );

		if (	( pkh->vkCode == VK_ESCAPE && bCtrlKeyDown )				// Ctrl+Esc
			 || ( pkh->vkCode == VK_TAB && pkh->flags & LLKHF_ALTDOWN )		// Alt+TAB
			 || ( pkh->vkCode == VK_ESCAPE && pkh->flags & LLKHF_ALTDOWN )	// Alt+Esc
			 || ( pkh->vkCode == VK_LWIN || pkh->vkCode == VK_RWIN )		// Start Menu
			 ) {

			if ( g_bBeep && ( wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN ) ) {
				MessageBeep( 0 ); // beep on downstroke if requested
			}
			return 1; // return without processing the key strokes
		}
	}
	return CallNextHookEx( g_hHookKbdLL, nCode, wp, lp );
}

/*
================
AreTaskKeysDisabled

  Are task keys disabled--ie, is hook installed?
  Note: This assumes there's no other hook that does the same thing!
================
*/
BOOL AreTaskKeysDisabled() {
	return g_hHookKbdLL != NULL;
}

/*
================
IsTaskMgrDisabled
================
*/
BOOL IsTaskMgrDisabled() {
	HKEY hk;

	if ( RegOpenKey( HKEY_CURRENT_USER, KEY_DisableTaskMgr, &hk ) != ERROR_SUCCESS ) {
		return FALSE; // no key ==> not disabled
	}

	DWORD val = 0;
	DWORD len = 4;
	return RegQueryValueEx( hk, VAL_DisableTaskMgr, NULL, NULL, (BYTE*)&val, &len ) == ERROR_SUCCESS && val == 1;
}

/*
================
DisableTaskKeys
================
*/
void DisableTaskKeys( BOOL bDisable, BOOL bBeep, BOOL bTaskMgr ) {

	// task keys (Ctrl+Esc, Alt-Tab, etc.)
	if ( bDisable ) {
		if ( !g_hHookKbdLL ) {
			g_hHookKbdLL = SetWindowsHookEx( WH_KEYBOARD_LL, MyTaskKeyHookLL, win32.hInstance, 0 );
		}
	} else if ( g_hHookKbdLL != NULL ) {
		UnhookWindowsHookEx( g_hHookKbdLL );
		g_hHookKbdLL = NULL;
	}
	g_bBeep = bBeep;

	// task manager (Ctrl+Alt+Del)
	if ( bTaskMgr ) {
		HKEY hk;
		if ( RegOpenKey( HKEY_CURRENT_USER, KEY_DisableTaskMgr, &hk ) != ERROR_SUCCESS ) {
			RegCreateKey( HKEY_CURRENT_USER, KEY_DisableTaskMgr, &hk );
		}
		if ( bDisable ) {
			// disable TM: set policy = 1
			DWORD val = 1;
			RegSetValueEx( hk, VAL_DisableTaskMgr, NULL, REG_DWORD, (BYTE*)&val, sizeof(val) );
		} else {
			// enable TM: remove policy 
			RegDeleteValue( hk,VAL_DisableTaskMgr );
		}
	}
}
