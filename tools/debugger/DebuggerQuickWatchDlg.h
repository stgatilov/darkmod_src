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
#ifndef DEBUGGERQUICKWATCHDLG_H_
#define DEBUGGERQUICKWATCHDLG_H_

class rvDebuggerWindow;

class rvDebuggerQuickWatchDlg
{
public:

	rvDebuggerQuickWatchDlg ( );

	bool	DoModal				( rvDebuggerWindow* window, int callstackDepth, const char* variable = NULL );

protected:

	HWND				mWnd;
	int					mCallstackDepth;
	idStr				mVariable;
	rvDebuggerWindow*	mDebuggerWindow;

	void				SetVariable	( const char* varname, bool force = false );

private:

	int					mEditFromRight;
	int					mButtonFromRight;
	int					mEditFromBottom;

	static INT_PTR	CALLBACK DlgProc ( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
};

#endif // DEBUGGERQUICKWATCHDLG_H_