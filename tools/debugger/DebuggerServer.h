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
#ifndef DEBUGGERSERVER_H_
#define DEBUGGERSERVER_H_

#ifndef DEBUGGERMESSAGES_H_
#include "DebuggerMessages.h"
#endif

#ifndef DEBUGGERBREAKPOINT_H_
#include "DebuggerBreakpoint.h"
#endif

#ifndef __GAME_LOCAL_H__
#include "../../game/Game.h"
#endif

class idInterpreter;
class idProgram;

class rvDebuggerServer
{
public:

	rvDebuggerServer ( );
	~rvDebuggerServer ( );

	bool		Initialize			( void );
	void		Shutdown			( void );
	
	bool		ProcessMessages		( void );
	
	bool		IsConnected			( void );
	
	void		CheckBreakpoints	( idInterpreter* interpreter, idProgram* program, int instructionPointer );
	
	void		Print				( const char* text );

	void		OSPathToRelativePath( const char *osPath, idStr &qpath );
		
protected:

	// protected member variables
	bool							mConnected;
	netadr_t						mClientAdr;
	idPort							mPort;
	idList<rvDebuggerBreakpoint*>	mBreakpoints;
	CRITICAL_SECTION				mCriticalSection;
	
	HANDLE							mGameThread;
	
	bool							mBreak;
	bool							mBreakNext;
	bool							mBreakStepOver;
	bool							mBreakStepInto;
	int								mBreakStepOverDepth;
	const function_t*				mBreakStepOverFunc1;
	const function_t*				mBreakStepOverFunc2;
	idProgram*						mBreakProgram;
	int								mBreakInstructionPointer;
	idInterpreter*					mBreakInterpreter;
	
	idStr							mLastStatementFile;
	int								mLastStatementLine;	
	
private:

	void		ClearBreakpoints				( void );

	void		Break							( idInterpreter* interpreter, idProgram* program, int instructionPointer );
	void		Resume							( void );

	void		SendMessage						( EDebuggerMessage dbmsg );
	void		SendPacket						( void* data, int datasize );

	// Message handlers
	void		HandleAddBreakpoint				( msg_t* msg );
	void		HandleRemoveBreakpoint			( msg_t* msg );
	void		HandleResume					( msg_t* msg );
	void		HandleInspectVariable			( msg_t* msg );
	void		HandleInspectCallstack			( msg_t* msg );
	void		HandleInspectThreads			( msg_t* msg );
	
	// MSG helper routines
	void		MSG_WriteCallstackFunc			( msg_t* msg, const prstack_t* stack );
};

/*
================
rvDebuggerServer::IsConnected
================
*/
ID_INLINE bool rvDebuggerServer::IsConnected ( void )
{
	return mConnected;
}

/*
================
rvDebuggerServer::SendPacket
================
*/
ID_INLINE void rvDebuggerServer::SendPacket ( void* data, int size )
{
	mPort.SendPacket ( mClientAdr, data, size );
}

#endif // DEBUGGERSERVER_H_
