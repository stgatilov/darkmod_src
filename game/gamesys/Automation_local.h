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

#ifndef AUTOMATION_LOCAL_H
#define AUTOMATION_LOCAL_H

#include "Automation.h"
#include "../Http/MessageTcp.h"

/**
 * Allows controlling TDM game from external tool.
 * Used in automated testing framework.
 */
class Automation {
public:
	void PushInputEvents();
	void ExecuteInGameConsole(const char *command, bool blocking = true);

	void Think();
private:
	idTCP listenTcp;
	MessageTcp connection;

	//info about current input message being parsed
	struct ParseIn {
		const char *message;
		int len;
		idLexer lexer;
		int seqno;
	};

	void ParseMessage(const char *message, int len);
	void ParseMessage(ParseIn &parseIn);
	void ParseAction(ParseIn &parseIn);
	void ParseQuery(ParseIn &parseIn);
	void WriteResponse(int seqno, const char *response);
};

#endif
