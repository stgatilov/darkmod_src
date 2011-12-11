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
#ifndef DEBUGGERWINDOW_H_
#define DEBUGGERWINDOW_H_

#ifndef DEBUGGERSCRIPT_H_
#include "DebuggerScript.h"
#endif

class rvDebuggerWatch
{
public:

	idStr	mVariable;
	idStr	mValue;
	bool	mModified;
};

typedef idList<rvDebuggerWatch*>		rvDebuggerWatchList;

class rvDebuggerClient;

class rvDebuggerWindow
{
public:
	
	rvDebuggerWindow ( );
	~rvDebuggerWindow ( );

	bool			Create				( HINSTANCE hInstance );	
		
	static bool		Activate			( void );

	void			ProcessNetMessage	( msg_t* msg );

	void			Printf				( const char* format, ... );
	
	HWND			GetWindow			( void );

	void			AddWatch			( const char* name, bool update = true );
	
	HINSTANCE		GetInstance			( void );
		
protected:		

	bool					FindPrev			( const char* text = NULL );
	bool					FindNext			( const char* text = NULL );

	void					UpdateWatch			( void );
	void					UpdateWindowMenu	( void );
	void					UpdateScript		( void );
	void					UpdateToolbar		( void );
	void					UpdateTitle			( void );
	void					UpdateCallstack		( void );
	void					UpdateRecentFiles	( void );
	bool					OpenScript			( const char* filename, int lineNumber = -1  );
	void					EnableWindows		( bool state );

	int						GetSelectedText		( idStr& text );

	void					ToggleBreakpoint	( void );
	
	HWND							mWnd;
	HWND							mWndScript;
	HWND							mWndOutput;
	HWND							mWndMargin;
	HWND							mWndTabs;
	HWND							mWndBorder;
	HWND							mWndConsole;
	HWND							mWndCallstack;
	HWND							mWndWatch;
	HWND							mWndThreads;
	HWND							mWndToolTips;
	HWND							mWndToolbar;
	
	HMENU							mRecentFileMenu;
	int								mRecentFileInsertPos;
		
	WNDPROC							mOldWatchProc;
	WNDPROC							mOldScriptProc;
	idStr							mTooltipVar;
	idStr							mTooltipValue;
	
	HINSTANCE						mInstance;
	HIMAGELIST						mImageList;
	
	RECT							mSplitterRect;
	bool							mSplitterDrag;

	idList<rvDebuggerScript*>		mScripts;
	int								mActiveScript;
	int								mLastActiveScript;
	int								mCurrentStackDepth;
	
	HMENU							mWindowMenu;
	int								mWindowMenuPos;
		
	int								mZoomScaleNum;
	int								mZoomScaleDem;
	int								mMarginSize;
	
	idStr							mFind;
	
	rvDebuggerClient*				mClient;

	rvDebuggerWatchList				mWatches;
	
private:

	bool		RegisterClass				( void );	
	void		CreateToolbar				( void );
	bool		InitRecentFiles				( void );

	int			HandleInitMenu				( WPARAM wParam, LPARAM lParam );
	int			HandleCommand				( WPARAM wParam, LPARAM lParam );
	int			HandleCreate				( WPARAM wparam, LPARAM lparam );
	int			HandleActivate				( WPARAM wparam, LPARAM lparam );
	int			HandleDrawItem				( WPARAM wparam, LPARAM lparam );
	void		HandleTooltipGetDispInfo	( WPARAM wparam, LPARAM lparam );	

	static LRESULT CALLBACK WndProc				( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static LRESULT CALLBACK MarginWndProc		( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static LRESULT CALLBACK ScriptWndProc		( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static INT_PTR CALLBACK AboutDlgProc		( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam );
	static int     CALLBACK ScriptWordBreakProc ( LPTSTR text, int current, int max, int action );
};

/*
================
rvDebuggerWindow::GetWindow
================
*/
ID_INLINE HWND rvDebuggerWindow::GetWindow ( void )
{
	return mWnd;
}

/*
================
rvDebuggerWindow::UpdateToolbar
================
*/
ID_INLINE void rvDebuggerWindow::UpdateToolbar ( void )
{
	HandleInitMenu ( (WPARAM)GetMenu ( mWnd ), 0 );
}

/*
================
rvDebuggerWindow::GetInstance
================
*/
ID_INLINE HINSTANCE rvDebuggerWindow::GetInstance ( void )
{
	return mInstance;
}

#endif // DEBUGGERWINDOW_H_

