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
#include "Automation_local.h"


#ifdef _DEBUG
//allow developers to enable automation persistently
#define AUTOMATION_VARMODE CVAR_ARCHIVE
#else
//make sure all players have automation disabled on TDM releases
//note: they can enable it manually in console, but setting won't be saved
#define AUTOMATION_VARMODE 0
#endif

idCVar com_automation("com_automation", "0", CVAR_BOOL | AUTOMATION_VARMODE, "Enable TDM automation (for tests)");
idCVar com_automation_port("com_automation_port", "3879", CVAR_INTEGER | CVAR_ARCHIVE, "The TCP port number to be listened to");

Automation automationLocal;
Automation *automation = &automationLocal;



void Automation::PushInputEvents() {
	sysEvent_t ev;
	memset(&ev, 0, sizeof(ev));
	ev.evType = SE_MOUSE;
	ev.evValue = 1;
	ev.evValue = 2;
	eventLoop->PushEvent(&ev);
}

void Automation::ExecuteInGameConsole(const char *command, bool blocking) {
	cmdSystem->BufferCommandText(blocking ? CMD_EXEC_NOW : CMD_EXEC_APPEND, command);
}

void Automation::ParseMessage(const char *message, int len) {
	ParseIn parseIn = { message, len, idLexer{message, len, "<automation input>"}, -1 };
	ParseMessage(parseIn);
}

void Automation::ParseMessage(ParseIn &parseIn) {
	idToken token;

	parseIn.lexer.ExpectTokenString("seqno");
	parseIn.lexer.ExpectTokenType(TT_NUMBER, TT_INTEGER, &token);
	parseIn.seqno = token.GetIntValue();

	parseIn.lexer.ExpectTokenString("message");
	parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);
	if (token == "action")
		ParseAction(parseIn);
	else if (token == "query")
		ParseQuery(parseIn);
	else
		parseIn.lexer.Error("Unknown message type '%s'", token.c_str());
}

void Automation::ParseAction(ParseIn &parseIn) {
	idToken token;

	parseIn.lexer.ExpectTokenString("action");
	parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);

	parseIn.lexer.ExpectTokenString("content");

	int pos = parseIn.lexer.GetFileOffset();
	const char *rest = strchr(parseIn.message + pos, '\n');

	if (token == "conexec") {
		int begMarker = common->GetConsoleMarker();
		while (const char *eol = strchr(rest, '\n')) {
			idStr command(rest, 0, eol - rest);
			if (!command.IsEmpty())
				ExecuteInGameConsole(command.c_str());
			rest = eol + 1;
		}
		int endMarker = common->GetConsoleMarker();
		idStr consoleUpdates = common->GetConsoleContents(begMarker, endMarker);
		WriteResponse(parseIn.seqno, consoleUpdates.c_str());
	}
}

extern int showFPS_currentValue;	//for "fps" query
void Automation::ParseQuery(ParseIn &parseIn) {
	idToken token;

	parseIn.lexer.ExpectTokenString("query");
	parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);

	if (token == "fps") {
		char buff[256];
		sprintf(buff, "%d\n", showFPS_currentValue);
		WriteResponse(parseIn.seqno, buff);
	}
}

void Automation::WriteResponse(int seqno, const char *response) {
	char buff[256];
	sprintf(buff, "response %d\n", seqno);
	idStr text = idStr(buff) + response;
	connection.WriteMessage(text.c_str(), text.Length());
}

void Automation::Think() {
	//make sure log file is enabled, so that automation can fetch console messages
	if (com_logFile.GetInteger() == 0) {
		common->Printf("Forcing com_logFile to 1 for automation to work properly");
		com_logFile.SetInteger(1);
	}
	//if started for first time, open tcp port and start listening
	if (!listenTcp.IsAlive()) {
		int port = com_automation_port.GetInteger();
		listenTcp.Listen(port);
		if (!listenTcp.IsAlive())
			common->Error("Automation cannot listen on port %d", port);
		common->Printf("Automation now listens on port %d\n", port);
	}
	//if not connected yet, check for incoming client
	if (!connection.IsAlive()) {
		std::unique_ptr<idTCP> clientTcp(listenTcp.Accept());
		if (!clientTcp)
			return;
		const auto &addr = clientTcp->GetAddress();
		common->Printf("Automation received incoming connection from %d.%d.%d.%d:%d\n",
			int(addr.ip[0]), int(addr.ip[1]), int(addr.ip[2]), int(addr.ip[3]), int(addr.port)
		);
		connection.Init(std::move(clientTcp));
	}
	//push data from output buffer, pull into input buffer
	connection.Think();

	//test: see what we get from python
	idList<char> message;
	while (connection.ReadMessage(message))
		ParseMessage(message.Ptr(), message.Num());

/*		common->Printf("Automation received: %s\n", message.Ptr());
		int num = -1;
		if (sscanf(message.Ptr(), "Hello %d", &num) == 1) {
			bool isprime = num > 1;
			for (int q = 2; q < num; q++) if (num % q == 0) isprime = false;
			if (isprime) {
				char buff[256];
				sprintf(buff, "%d is prime", num);
				connection.WriteMessage(buff, (int)strlen(buff));
			}
		}
	}*/
}

void Auto_Think() {
    automation->Think();
}
