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
#ifndef DEBUGGERSERVER_H_
#define DEBUGGERSERVER_H_

#include "sys/sys_threading.h"
#include "sys/sys_public.h"
#include "idlib/Str.h"
#include "DebuggerMessages.h"
#include "DebuggerBreakpoint.h"
#include "game/Game.h"

class function_t;
typedef struct prstack_s prstack_t;

class rvDebuggerServer
{
public:

	rvDebuggerServer ( );
	~rvDebuggerServer ( );

	bool		Initialize				( void );
	void		Shutdown				( void );

	bool		ProcessMessages			( void );

	bool		IsConnected				( void );

	void		CheckBreakpoints		( idInterpreter *interpreter, idProgram *program, int instructionPointer );

	void		Print					( const char *text );

	void		OSPathToRelativePath	( const char *osPath, idStr &qpath );

	bool		GameSuspended			( void );
private:

	void		ClearBreakpoints		( void );

	void		Break					( idInterpreter *interpreter, idProgram *program, int instructionPointer );
	void		Resume					( void );

	void		SendMessage				( EDebuggerMessage dbmsg );
	void		SendPacket				( void* data, int datasize );

	// Message handlers
	void		HandleAddBreakpoint		( idBitMsg *msg );
	void		HandleRemoveBreakpoint	( idBitMsg *msg );
	void		HandleResume			( idBitMsg *msg );
	void		HandleInspectVariable	( idBitMsg *msg );
	void		HandleInspectCallstack	( idBitMsg *msg );
	void		HandleInspectThreads	( idBitMsg *msg );
	void		HandleInspectScripts	( idBitMsg *msg );
	void		HandleExecCommand		( idBitMsg *msg );
	////

	bool							mConnected;
	netadr_t						mClientAdr;
	idPort							mPort;
	idList<rvDebuggerBreakpoint*>	mBreakpoints;
	mutexHandle_t					mCriticalSection;

	signalHandle_t					mGameThreadBreakSignal;
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
	uintptr_t						mGameDLLHandle;
	idStrList						mScriptFileList;

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
ID_INLINE void rvDebuggerServer::SendPacket ( void *data, int size )
{
	mPort.SendPacket ( mClientAdr, data, size );
}

/*
================
rvDebuggerServer::GameSuspended
================
*/
ID_INLINE bool rvDebuggerServer::GameSuspended( void )
{
	return mBreak;
}

#endif // DEBUGGERSERVER_H_
