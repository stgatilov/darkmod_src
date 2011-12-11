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
#ifndef DEBUGGERMESSAGES_H_
#define DEBUGGERMESSAGES_H_

enum EDebuggerMessage
{
	DBMSG_UNKNOWN,
	DBMSG_CONNECT,
	DBMSG_CONNECTED,
	DBMSG_DISCONNECT,
	DBMSG_ADDBREAKPOINT,
	DBMSG_REMOVEBREAKPOINT,
	DBMSG_HITBREAKPOINT,
	DBMSG_RESUME,
	DBMSG_RESUMED,
	DBMSG_BREAK,
	DBMSG_PRINT,
	DBMSG_INSPECTVARIABLE,
	DBMSG_INSPECTCALLSTACK,
	DBMSG_INSPECTTHREADS,
	DBMSG_STEPOVER,
	DBMSG_STEPINTO,
};

#endif // DEBUGGER_MESSAGES_H_