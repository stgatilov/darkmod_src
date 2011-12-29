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
#ifndef DEBUGGERCLIENT_H_
#define DEBUGGERCLIENT_H_

class rvDebuggerCallstack
{
public:

	idStr	mFilename;
	int		mLineNumber;
	idStr	mFunction;
};

class rvDebuggerThread
{
public:

	idStr	mName;
	int		mID;
	bool	mCurrent;
	bool	mDying;
	bool	mWaiting;
	bool	mDoneProcessing;
};

#ifndef DEBUGGERBREAKPOINT_H_
#include "DebuggerBreakpoint.h"
#endif

typedef idList<rvDebuggerCallstack*>	rvDebuggerCallstackList;
typedef idList<rvDebuggerThread*>		rvDebuggerThreadList;
typedef idList<rvDebuggerBreakpoint*>	rvDebuggerBreakpointList;

class rvDebuggerClient
{
public:

	rvDebuggerClient ( );
	~rvDebuggerClient ( );

	bool						Initialize				( void );
	void						Shutdown				( void );
	bool						ProcessMessages			( void );
	bool						WaitFor					( EDebuggerMessage msg, int time );
	
	bool						IsConnected				( void );
	bool						IsStopped				( void );
	
	int							GetActiveBreakpointID	( void );
	const char*					GetBreakFilename		( void );
	int							GetBreakLineNumber		( void );
	rvDebuggerCallstackList&	GetCallstack			( void );
	rvDebuggerThreadList&		GetThreads				( void );
	const char*					GetVariableValue		( const char* name, int stackDepth );
	
	void						InspectVariable			( const char* name, int callstackDepth );
	
	void						Break					( void );
	void						Resume					( void );
	void						StepInto				( void );
	void						StepOver				( void );

	// Breakpoints
	int							AddBreakpoint			( const char* filename, int lineNumber, bool onceOnly = false );
	bool						RemoveBreakpoint		( int bpID );
	void						ClearBreakpoints		( void );
	int							GetBreakpointCount		( void );
	rvDebuggerBreakpoint*		GetBreakpoint			( int index );
	rvDebuggerBreakpoint*		FindBreakpoint			( const char* filename, int linenumber );
		
protected:

	void						SendMessage				( EDebuggerMessage dbmsg );
	void						SendBreakpoints			( void );
	void						SendAddBreakpoint		( rvDebuggerBreakpoint& bp, bool onceOnly = false );
	void						SendRemoveBreakpoint	( rvDebuggerBreakpoint& bp );
	void						SendPacket				( void* data, int datasize );

	bool						mConnected;
	netadr_t					mServerAdr;
	idPort						mPort;
	
	bool						mBreak;
	int							mBreakID;
	int							mBreakLineNumber;
	idStr						mBreakFilename;
	
	idDict						mVariables;

	rvDebuggerCallstackList		mCallstack;
	rvDebuggerThreadList		mThreads;	
	rvDebuggerBreakpointList	mBreakpoints;

	EDebuggerMessage			mWaitFor;
	
private:

	void		ClearCallstack				( void );
	void		ClearThreads				( void );
	
	void		UpdateWatches				( void );
	
	// Network message handlers
	void		HandleBreak					( msg_t* msg );
	void		HandleInspectCallstack		( msg_t* msg );
	void		HandleInspectThreads		( msg_t* msg );
	void		HandleInspectVariable		( msg_t* msg );
};

/*
================
rvDebuggerClient::IsConnected
================
*/
ID_INLINE bool rvDebuggerClient::IsConnected ( void )
{
	return mConnected;
}

/*
================
rvDebuggerClient::IsStopped
================
*/
ID_INLINE bool rvDebuggerClient::IsStopped ( void )
{
	return mBreak;
}

/*
================
rvDebuggerClient::GetActiveBreakpointID
================
*/
ID_INLINE int rvDebuggerClient::GetActiveBreakpointID ( void )
{
	return mBreakID;
}

/*
================
rvDebuggerClient::GetBreakFilename
================
*/
ID_INLINE const char* rvDebuggerClient::GetBreakFilename ( void )
{
	return mBreakFilename;
}

/*
================
rvDebuggerClient::GetBreakLineNumber
================
*/
ID_INLINE int rvDebuggerClient::GetBreakLineNumber ( void )
{
	return mBreakLineNumber;
}

/*
================
rvDebuggerClient::GetCallstack
================
*/
ID_INLINE rvDebuggerCallstackList& rvDebuggerClient::GetCallstack ( void )
{
	return mCallstack;
}

/*
================
rvDebuggerClient::GetThreads
================
*/
ID_INLINE rvDebuggerThreadList& rvDebuggerClient::GetThreads ( void )
{
	return mThreads;
}

/*
================
rvDebuggerClient::GetVariableValue
================
*/
ID_INLINE const char* rvDebuggerClient::GetVariableValue ( const char* var, int stackDepth )
{
	return mVariables.GetString ( va("%d:%s",stackDepth,var), "" );
}

/*
================
rvDebuggerClient::GetBreakpointCount
================
*/
ID_INLINE int rvDebuggerClient::GetBreakpointCount ( void )
{
	return mBreakpoints.Num ( );
}

/*
================
rvDebuggerClient::GetBreakpoint
================
*/
ID_INLINE rvDebuggerBreakpoint* rvDebuggerClient::GetBreakpoint ( int index )
{
	return mBreakpoints[index];
}

/*
================
rvDebuggerClient::Break
================
*/
ID_INLINE void rvDebuggerClient::Break ( void )
{
	SendMessage ( DBMSG_BREAK );
}

/*
================
rvDebuggerClient::Resume
================
*/
ID_INLINE void rvDebuggerClient::Resume ( void )
{
	mBreak = false;
	SendMessage ( DBMSG_RESUME );
}

/*
================
rvDebuggerClient::StepOver
================
*/
ID_INLINE void rvDebuggerClient::StepOver ( void )
{
	mBreak = false;
	SendMessage ( DBMSG_STEPOVER );
}

/*
================
rvDebuggerClient::StepInto
================
*/
ID_INLINE void rvDebuggerClient::StepInto ( void )
{
	mBreak = false;
	SendMessage ( DBMSG_STEPINTO );
}

/*
================
rvDebuggerClient::SendPacket
================
*/
ID_INLINE void rvDebuggerClient::SendPacket ( void* data, int size )
{
	mPort.SendPacket ( mServerAdr, data, size );
}

#endif // DEBUGGERCLIENT_H_
