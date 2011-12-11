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
#ifndef DEBUGGERBREAKPOINT_H_
#define DEBUGGERBREAKPOINT_H_

class rvDebuggerBreakpoint 
{
public:

	rvDebuggerBreakpoint ( const char* filename, int linenumber, int id = -1 );
	rvDebuggerBreakpoint ( rvDebuggerBreakpoint& bp );
	~rvDebuggerBreakpoint ( void );

	const char*		GetFilename		( void );
	int				GetLineNumber	( void );
	int				GetID			( void );

protected:

	bool	mEnabled;
	int		mID;
	int		mLineNumber;
	idStr	mFilename;
	
private:

	static int	mNextID;
};

ID_INLINE const char* rvDebuggerBreakpoint::GetFilename ( void )
{
	return mFilename;
}

ID_INLINE int rvDebuggerBreakpoint::GetLineNumber ( void )
{
	return mLineNumber;
}

ID_INLINE int rvDebuggerBreakpoint::GetID ( void )
{
	return mID;
}

#endif // DEBUGGERBREAKPOINT_H_
