/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-11-19 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "DarkRadiantRCFServer.h"
#include "DarkRadiantRCFInterface.h"
#include <RCF/TcpEndpoint.hpp>

#include "../idlib/precompiled.h"

extern idCmdSystem*	 cmdSystem;
extern idCommon* common;

DarkRadiantRCFServer::DarkRadiantRCFServer() :
	_server(RCF::TcpEndpoint(50001))
{
	_server.bind<D3ConsoleWriter>(*this);
    _server.start(true);
}

DarkRadiantRCFServer::~DarkRadiantRCFServer()
{
	_server.stop();
}

void DarkRadiantRCFServer::writeToConsole(const std::string& text)
{
    gameLocal.Printf(text.c_str());
}

void DarkRadiantRCFServer::executeConsoleCommand(const std::string& command)
{
	assert(cmdSystem != NULL);

	_largeBuffer.clear();

	// Issue the command to the idCmdSystem
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, command.c_str());
	//cmdSystem->ExecuteCommandBuffer();

	//common->EndRedirect();

	//_largeBuffer.clear();
}

void DarkRadiantRCFServer::startConsoleBuffering(int dummy) 
{
	// Let the console output be redirected to the given buffer
	instance = this;
	common->BeginRedirect(_buffer, sizeof(_buffer), FlushBuffer);
}

void DarkRadiantRCFServer::endConsoleBuffering(int dummy)
{
	common->EndRedirect();
	instance = NULL;
}

std::string DarkRadiantRCFServer::readConsoleBuffer(int dummy) {
	DM_LOG(LC_AI, LT_INFO).LogString("readConsoleBuffer called, size is %d\r", _largeBuffer.size());
	std::string temp(_largeBuffer);
	_largeBuffer.clear();
	return temp;
}

void DarkRadiantRCFServer::FlushBuffer(const char* text)
{
	if (instance != NULL)
	{
		instance->_largeBuffer.append(text);
	}
}

DarkRadiantRCFServer* DarkRadiantRCFServer::instance = NULL;
