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
#ifndef DEBUGGERFINDDLG_H_
#define DEBUGGERFINDDLG_H_

class rvDebuggerWindow;

class rvDebuggerFindDlg
{
public:

	rvDebuggerFindDlg ( );

	bool	DoModal				( rvDebuggerWindow* window );

	const char*		GetFindText	( void );

protected:

	HWND	mWnd;

private:

	static char		mFindText[ 256 ];

	static INT_PTR	CALLBACK DlgProc ( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
};

ID_INLINE const char* rvDebuggerFindDlg::GetFindText ( void )
{
	return mFindText;
}

#endif // DEBUGGERFINDDLG_H_