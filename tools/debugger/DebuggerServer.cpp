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
#if defined( ID_ALLOW_TOOLS )
#include "DebuggerServer.h"
#include "DebuggerApp.h"
#else
#include "DebuggerServer.h"
#include "debugger_common.h"
// we need a lot to be able to list all threads in mars_city1
const int MAX_MSGLEN = 8600;
#endif

/*
================
rvDebuggerServer::rvDebuggerServer
================
*/
rvDebuggerServer::rvDebuggerServer ( )
{
	mConnected			= false;
	mBreakNext			= false;
	mBreak				= false;
	mBreakStepOver		= false;
	mBreakStepInto		= false;
	// TODO: clear mGameThreadBreakSignal ?
	mLastStatementLine	= -1;
	mBreakStepOverFunc1 = NULL;
	mBreakStepOverFunc2 = NULL;
	mBreakInstructionPointer = 0;
	mBreakInterpreter = NULL;
	mBreakProgram = NULL;
	mGameDLLHandle = 0;
	mBreakStepOverDepth = 0;
	//mCriticalSection = 0; - TODO: clear somehow?
}

/*
================
rvDebuggerServer::~rvDebuggerServer
================
*/
rvDebuggerServer::~rvDebuggerServer ( )
{
}

/*
================
rvDebuggerServer::Initialize

Initialize the debugger server.  This function should be called before the
debugger server is used.
================
*/
bool rvDebuggerServer::Initialize ( void )
{
	// Initialize the network connection
	if ( !mPort.InitForPort ( 27980 ) )
	{
		return false;
	}

	// we're using a signal to pause the game thread in rbDebuggerServer::Break()
	// until rvDebuggerServer::Resume() is called (from another thread)
	Sys_SignalCreate( mGameThreadBreakSignal, true );

	// Create a critical section to ensure that the shared thread
	// variables are protected
	Sys_MutexCreate( mCriticalSection );

	// Server must be running on the local host on port 28980
	Sys_StringToNetAdr ( com_dbgClientAdr.GetString( ), &mClientAdr, true );
	mClientAdr.port = 27981;

	// Attempt to let the server know we are here.  The server may not be running so this
	// message will just get ignored.
	SendMessage ( DBMSG_CONNECT );

	return true;
}

void rvDebuggerServer::OSPathToRelativePath( const char *osPath, idStr &qpath )
{
	if ( strchr( osPath, ':' ) ) // XXX: what about linux?
	{
		qpath = fileSystem->OSPathToRelativePath( osPath );
	}
	else
	{
		qpath = osPath;
	}
}

/*
================
rvDebuggerServer::Shutdown

Shutdown the debugger server.
================
*/
void rvDebuggerServer::Shutdown ( void )
{
	// Let the debugger client know we are shutting down
	if ( mConnected )
	{
		SendMessage ( DBMSG_DISCONNECT );
		mConnected = false;
	}

	mPort.Close();

	Resume(); // just in case we're still paused

	// don't need the crit section anymore
	Sys_MutexDestroy( mCriticalSection );
	//mCriticalSection = NULL; TODO clear?


	Sys_SignalDestroy( mGameThreadBreakSignal );
	// TODO: clear mGameThreadBreakSignal ?
}

/*
================
rvDebuggerServer::ProcessMessages

Process all incoming network messages from the debugger client
================
*/
bool rvDebuggerServer::ProcessMessages ( void )
{
	netadr_t adrFrom;
	idBitMsg	 msg;
	byte	 buffer[MAX_MSGLEN];

	// Check for pending udp packets on the debugger port
	int msgSize;
	while ( mPort.GetPacket ( adrFrom, buffer, msgSize, MAX_MSGLEN) )
	{
		short command;
		msg.Init(buffer, sizeof(buffer));
		msg.SetSize(msgSize);
		msg.BeginReading();
		
		if ( adrFrom.type != NA_LOOPBACK ) {
			// Only accept packets from the debugger server for security reasons
			if ( !Sys_CompareNetAdrBase( adrFrom, mClientAdr ) )
				continue;
		}

		command = msg.ReadShort( );

		switch ( command )
		{
			case DBMSG_CONNECT:
				mConnected = true;
				SendMessage ( DBMSG_CONNECTED );
				HandleInspectScripts ( NULL );
				com_editors |= EDITOR_DEBUGGER;
				break;

			case DBMSG_CONNECTED:
				mConnected = true;
				HandleInspectScripts( NULL );
				com_editors |= EDITOR_DEBUGGER;
				break;

			case DBMSG_DISCONNECT:
				ClearBreakpoints ( );
				Resume ( );
				mConnected = false;
				com_editors &= ~EDITOR_DEBUGGER;
				break;

			case DBMSG_ADDBREAKPOINT:
				HandleAddBreakpoint ( &msg );
				break;

			case DBMSG_REMOVEBREAKPOINT:
				HandleRemoveBreakpoint ( &msg );
				break;

			case DBMSG_RESUME:
				HandleResume ( &msg );
				break;

			case DBMSG_BREAK:
				mBreakNext = true;
				break;

			case DBMSG_STEPOVER:
				mBreakStepOver = true;
				mBreakStepOverDepth = gameEdit->GetInterpreterCallStackDepth(mBreakInterpreter);
				mBreakStepOverFunc1 = gameEdit->GetInterpreterCallStackFunction(mBreakInterpreter);
				if (mBreakStepOverDepth)
				{
					mBreakStepOverFunc2 = gameEdit->GetInterpreterCallStackFunction(mBreakInterpreter,mBreakStepOverDepth - 1);
				}
				else
				{
					mBreakStepOverFunc2 = NULL;
				}
				Resume ( );
				break;

			case DBMSG_STEPINTO:
				mBreakStepInto = true;
				Resume ( );
				break;

			case DBMSG_INSPECTVARIABLE:
				HandleInspectVariable ( &msg );
				break;

			case DBMSG_INSPECTCALLSTACK:
				HandleInspectCallstack ( &msg );
				break;

			case DBMSG_INSPECTTHREADS:
				HandleInspectThreads ( &msg );
				break;

			case DBMSG_INSPECTSCRIPTS:
				HandleInspectScripts( &msg );
				break;

			case DBMSG_EXECCOMMAND:
				HandleExecCommand( &msg );
				break;
		}
	}

	return true;
}

/*
================
rvDebuggerServer::SendMessage

Send a message with no data to the debugger server.
================
*/
void rvDebuggerServer::SendMessage ( EDebuggerMessage dbmsg )
{
	idBitMsg	 msg;
	byte	 buffer[MAX_MSGLEN];

	msg.Init( buffer, sizeof( buffer ) );
	msg.BeginWriting();
	msg.WriteShort ( (short)dbmsg );

	SendPacket ( msg.GetData(), msg.GetSize() );
}

/*
================
rvDebuggerServer::HandleAddBreakpoint

Handle the DBMSG_ADDBREAKPOINT message being sent by the debugger client.  This
message is handled by first checking if it is valid
and is added as a new breakpoint to the breakpoint list with the
data supplied in the message.
================
*/
void rvDebuggerServer::HandleAddBreakpoint ( idBitMsg* msg )
{
	bool onceOnly = false;
	long lineNumber;
	long id;
	char filename[2048]; // DG: randomly chose this size

	// Read the breakpoint info
	onceOnly = msg->ReadBits( 1 ) ? true : false;
	lineNumber = msg->ReadInt ( );
	id		   = msg->ReadInt ( );

	msg->ReadString ( filename, sizeof(filename) );

	//check for statement on requested breakpoint location 
	if (!gameEdit->IsLineCode(filename, lineNumber))
	{
		idBitMsg	msgOut;
		byte		buffer[MAX_MSGLEN];

		msgOut.Init(buffer, sizeof(buffer));
		msgOut.BeginWriting();
		msgOut.WriteShort((short)DBMSG_REMOVEBREAKPOINT);
		msgOut.WriteInt(lineNumber);
		msgOut.WriteString(filename);
		SendPacket(msgOut.GetData(), msgOut.GetSize());
		return;
	}


	Sys_MutexLock( mCriticalSection, true );
	mBreakpoints.Append ( new rvDebuggerBreakpoint ( filename, lineNumber, id, onceOnly ) );
	Sys_MutexUnlock( mCriticalSection );
}

/*
================
rvDebuggerServer::HandleRemoveBreakpoint

Handle the DBMSG_REMOVEBREAKPOINT message being sent by the debugger client.  This
message is handled by removing the breakpoint that matches the given id from the
list.
================
*/
void rvDebuggerServer::HandleRemoveBreakpoint ( idBitMsg* msg )
{
	int i;
	int id;

	// ID that we are to remove
	id = msg->ReadInt ( );

	// Since breakpoints are used by both threads we need to
	// protect them with a crit section
	Sys_MutexLock( mCriticalSection, true );

	// Find the breakpoint that matches the given id and remove it from the list
	for ( i = 0; i < mBreakpoints.Num(); i ++ )
	{
		if ( mBreakpoints[i]->GetID ( ) == id )
		{
			delete mBreakpoints[i];
			mBreakpoints.RemoveIndex ( i );
			break;
		}
	}

	Sys_MutexUnlock( mCriticalSection );
}

/*
================
rvDebuggerServer::HandleResume

Resume the game thread.
================

*/
void rvDebuggerServer::HandleResume(idBitMsg* msg)
{
	//Empty msg
	Resume();
}

/*
================
rvDebuggerServer::HandleInspectCallstack

Handle an incoming inspect callstack message by sending a message
back to the client with the callstack data.
================
*/
void rvDebuggerServer::HandleInspectCallstack ( idBitMsg* msg )
{
	idBitMsg	 msgOut;
	byte		 buffer[MAX_MSGLEN];

	msgOut.Init(buffer, sizeof( buffer ) );
	msgOut.BeginWriting();
	msgOut.WriteShort ( (short)DBMSG_INSPECTCALLSTACK );

	gameEdit->MSG_WriteInterpreterInfo(&msgOut, mBreakInterpreter, mBreakProgram, mBreakInstructionPointer);

	SendPacket (msgOut.GetData(), msgOut.GetSize() );
}

/*
================
rvDebuggerServer::HandleInspectThreads

Send the list of the current threads in the interpreter back to the debugger client
================
*/
void rvDebuggerServer::HandleInspectThreads ( idBitMsg* msg )
{
	idBitMsg	msgOut;
	byte		buffer[MAX_MSGLEN];

	// Initialize the message
	msgOut.Init( buffer, sizeof( buffer ) );
	msgOut.SetAllowOverflow(true);
	msgOut.BeginWriting();
	msgOut.WriteShort ( (short)DBMSG_INSPECTTHREADS );

	// Write the number of threads to the message
	msgOut.WriteShort ((short)gameEdit->GetTotalScriptThreads() );

	// Loop through all of the threads and write their name and number to the message
	for ( int i = 0, n=gameEdit->GetTotalScriptThreads(); i < n; i++ )
	{
		const idThread* t = gameEdit->GetThreadByIndex(i);
		if ( t != NULL ) {
			gameEdit->MSG_WriteThreadInfo(&msgOut, t, mBreakInterpreter);
		}
	}

	// Send off the inspect threads packet to the debugger client
	SendPacket (msgOut.GetData(), msgOut.GetSize() );
}

/*
================
rvDebuggerServer::HandleExecCommand

Send the list of the current loaded scripts in the interpreter back to the debugger client
================
*/
void rvDebuggerServer::HandleExecCommand( idBitMsg *msg ) {
	char cmdStr[2048]; // HvG: randomly chose this size

	msg->ReadString( cmdStr, sizeof( cmdStr ) );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, cmdStr );	// valid command
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );
}


/*
================
rvDebuggerServer::HandleInspectScripts

Send the list of the current loaded scripts in the interpreter back to the debugger client
================
*/
void rvDebuggerServer::HandleInspectScripts( idBitMsg* msg )
{
	idBitMsg	 msgOut;
	byte		 buffer[MAX_MSGLEN];

	// Initialize the message
	msgOut.Init(buffer, sizeof(buffer));
	msgOut.BeginWriting();
	msgOut.WriteShort((short)DBMSG_INSPECTSCRIPTS);

	gameEdit->MSG_WriteScriptList( &msgOut );

	SendPacket(msgOut.GetData(), msgOut.GetSize());
}

/*
================
rvDebuggerServer::HandleInspectVariable

Respondes to a request from the debugger client to inspect the value of a given variable
================
*/
void rvDebuggerServer::HandleInspectVariable ( idBitMsg* msg )
{
	char varname[256];
	int  scopeDepth;

	if ( !mBreak )
	{
		return;
	}

	scopeDepth = (short)msg->ReadShort ( );
	msg->ReadString ( varname, 256 );

	idStr varvalue;

	idBitMsg	 msgOut;
	byte		 buffer[MAX_MSGLEN];

	// Initialize the message
	msgOut.Init( buffer, sizeof( buffer ) );
	msgOut.BeginWriting();
	msgOut.WriteShort ( (short)DBMSG_INSPECTVARIABLE );

	if (!gameEdit->GetRegisterValue(mBreakInterpreter, varname, varvalue, scopeDepth ) )
	{
		varvalue = "???";
	}

	msgOut.WriteShort ( (short)scopeDepth );
	msgOut.WriteString ( varname );
	msgOut.WriteString ( varvalue );

	SendPacket (msgOut.GetData(), msgOut.GetSize() );
}

/*
================
rvDebuggerServer::CheckBreakpoints

Check to see if any breakpoints have been hit.  This includes "break next",
"step into", and "step over" break points
================
*/
void rvDebuggerServer::CheckBreakpoints	( idInterpreter* interpreter, idProgram* program, int instructionPointer )
{
	const char*			filename;
	int					i;

	if ( !mConnected ) {
		return;
	}

	
	// Grab the current statement and the filename that it came from
	filename = gameEdit->GetFilenameForStatement(program, instructionPointer);
	int linenumber = gameEdit->GetLineNumberForStatement(program, instructionPointer);

	// Operate on lines, not statements
	if ( mLastStatementLine == linenumber && mLastStatementFile == filename)
	{
		return;
	}
	
	// Save the last visited line and file so we can prevent
	// double breaks on lines with more than one statement
	mLastStatementFile = idStr(filename);
	mLastStatementLine = linenumber;

	// Reset stepping when the last function on the callstack is returned from
	if ( gameEdit->ReturnedFromFunction(program, interpreter,instructionPointer))
	{
		mBreakStepOver = false;
		mBreakStepInto = false;
	}

	// See if we are supposed to break on the next script line
	if ( mBreakNext )
	{
		HandleInspectScripts(NULL);
		Break ( interpreter, program, instructionPointer );
		return;
	}

	// Only break on the same callstack depth and thread as the break over
	if ( mBreakStepOver )
	{
		//virtual bool CheckForBreakpointHit(interpreter,function1,function2,depth)
		if ( gameEdit->CheckForBreakPointHit(interpreter, mBreakStepOverFunc1, mBreakStepOverFunc2, mBreakStepOverDepth) )
		{
			Break ( interpreter, program, instructionPointer );
			return;
		}
	}

	// See if we are supposed to break on the next line
	if ( mBreakStepInto )
	{
		HandleInspectScripts(NULL);
		// Break
		Break ( interpreter, program, instructionPointer );
		return;
	}

	idStr qpath;
	OSPathToRelativePath(filename,qpath);
	qpath.BackSlashesToSlashes ( );

	Sys_MutexLock( mCriticalSection, true );

	// Check all the breakpoints
	for ( i = 0; i < mBreakpoints.Num ( ); i ++ )
	{
		rvDebuggerBreakpoint* bp = mBreakpoints[i];

		// Skip if not match of the line number
		if ( linenumber != bp->GetLineNumber ( ) )
		{
			continue;
		}

		// Skip if no match of the filename
		if ( idStr::Icmp ( bp->GetFilename(), qpath.c_str() ) )
		{
			continue;
		}

		// DG: onceOnly support
		if ( bp->GetOnceOnly() ) {
			// we'll do the one Break() a few lines below; remove it here while mBreakpoints is unmodified
			// (it can be modifed from the client while in Break() below)
			mBreakpoints.RemoveIndex( i );
			delete bp;

			// also tell client to remove the breakpoint
			idBitMsg	msgOut;
			byte		buffer[MAX_MSGLEN];
			msgOut.Init( buffer, sizeof( buffer ) );
			msgOut.BeginWriting();
			msgOut.WriteShort( (short)DBMSG_REMOVEBREAKPOINT );
			msgOut.WriteInt( linenumber );
			msgOut.WriteString( qpath.c_str() );
			SendPacket( msgOut.GetData(), msgOut.GetSize() );
		}
		// DG end

		// Pop out of the critical section so we dont get stuck
		Sys_MutexUnlock( mCriticalSection );

		HandleInspectScripts(NULL);
		// We hit a breakpoint, so break
		Break ( interpreter, program, instructionPointer );

		// Back into the critical section since we are going to have to leave it
		Sys_MutexLock( mCriticalSection, true );

		break;
	}

	Sys_MutexUnlock( mCriticalSection );
}

/*
================
rvDebuggerServer::Break

Halt execution of the game threads and inform the debugger client that
the game has been halted
================
*/
void rvDebuggerServer::Break ( idInterpreter* interpreter, idProgram* program, int instructionPointer )
{
	idBitMsg			msg;
	byte				buffer[MAX_MSGLEN];
	const char*			filename;

	// Clear all the break types
	mBreakStepOver = false;
	mBreakStepInto = false;
	mBreakNext     = false;

	// Grab the current statement and the filename that it came from
	filename = gameEdit->GetFilenameForStatement(program,instructionPointer);
	int linenumber = gameEdit->GetLineNumberForStatement(program, instructionPointer);
	idStr fileStr = filename;
	fileStr.BackSlashesToSlashes();

	// Give the mouse cursor back to the world
	Sys_GrabMouseCursor( false );

	// Set the break variable so we know the main thread is stopped
	mBreak = true;
	mBreakProgram = program;
	mBreakInterpreter = interpreter;
	mBreakInstructionPointer = instructionPointer;

	// Inform the debugger of the breakpoint hit
	msg.Init( buffer, sizeof( buffer ) );
	msg.BeginWriting();
	msg.WriteShort ( (short)DBMSG_BREAK );
	msg.WriteInt ( linenumber );
	msg.WriteString ( fileStr.c_str() );

	//msg.WriteInt64( (int64_t)mBreakProgram );

	SendPacket ( msg.GetData(), msg.GetSize() );

	// Suspend the game thread.  Since this will be called from within the main game thread
	// execution wont return until after the thread is resumed
	// DG: the original code used Win32 SuspendThread() here, but as we have no equivalent
	//     function in sys_threading.h and as this is only called within the main game thread anyway,
	//     just use a signal to put this thread to sleep until Resume() has set mBreak
	// first make sure the signal is cleared (so wait actually blocks)
	Sys_SignalClear( mGameThreadBreakSignal );
	// now wait for Resume() to raise the signal (from the debugger thread)
	Sys_SignalWait( mGameThreadBreakSignal, idSysSignal::WAIT_INFINITE );

	// Let the debugger client know that we have started back up again
	SendMessage ( DBMSG_RESUMED );

	// this should be platform specific
#if defined( ID_ALLOW_TOOLS )
	// This is to give some time between the keypress that
	// told us to resume and the setforeground window.  Otherwise the quake window
	// would just flash
	Sleep ( 150 );

	// Bring the window back to the foreground
	SetForegroundWindow ( win32.hWnd );
	SetActiveWindow ( win32.hWnd );
	UpdateWindow ( win32.hWnd );
	SetFocus ( win32.hWnd );
#endif

	// Give the mouse cursor back to the game
	// HVG_Note : there be dragons here. somewhere.
	Sys_GrabMouseCursor( true );

	// Clear all commands that were generated before we went into suspended mode.  This is
	// to ensure we dont have mouse downs with no ups because the context was changed.
	idKeyInput::ClearStates();
}

/*
================
rvDebuggerServer::Resume

Resume execution of the game.
================
*/
void rvDebuggerServer::Resume ( void )
{
	// Cant resume if not paused
	if ( !mBreak )
	{
		return;
	}

	// Start the game thread back up
	mBreak = false;
	Sys_SignalRaise( mGameThreadBreakSignal );
}

/*
================
rvDebuggerServer::ClearBreakpoints

Remove all known breakpoints
================
*/
void rvDebuggerServer::ClearBreakpoints	( void )
{
	int i;

	for ( i = 0; i < mBreakpoints.Num(); i ++ )
	{
		delete mBreakpoints[i];
	}

	mBreakpoints.Clear ( );
}

/*
================
rvDebuggerServer::Print

Sends a console print message over to the debugger client
================
*/
void rvDebuggerServer::Print ( const char* text )
{
	if ( !mConnected )
	{
		return;
	}

	idBitMsg msg;
	byte	 buffer[MAX_MSGLEN];

	msg.Init( buffer, sizeof( buffer ) );
	msg.BeginWriting();
	msg.WriteShort ( (short)DBMSG_PRINT );
	msg.WriteString ( text );

	SendPacket ( msg.GetData(), msg.GetSize() );
}
