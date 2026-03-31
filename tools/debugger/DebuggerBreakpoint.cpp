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
#include "DebuggerApp.h"
#else
#include "debugger_common.h"
#endif

#include "DebuggerBreakpoint.h"

int rvDebuggerBreakpoint::mNextID = 1;

rvDebuggerBreakpoint::rvDebuggerBreakpoint ( const char* filename, int linenumber, int id, bool onceOnly )
{
	mFilename = filename;
	mLineNumber = linenumber;
	mEnabled = true;
	mOnceOnly = onceOnly;

	if ( id == -1 )
	{
		mID = mNextID++;
	}
	else
	{
		mID = id;
	}
}

rvDebuggerBreakpoint::rvDebuggerBreakpoint ( rvDebuggerBreakpoint& bp )
{
	mFilename = bp.mFilename;
	mEnabled = bp.mEnabled;
	mLineNumber = bp.mLineNumber;
}

rvDebuggerBreakpoint::~rvDebuggerBreakpoint ( void )
{
}
