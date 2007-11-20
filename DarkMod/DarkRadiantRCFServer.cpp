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
    _server.start(false);

	instance = this;
}

DarkRadiantRCFServer::~DarkRadiantRCFServer()
{
	_server.stop();
	instance = NULL;
}

void DarkRadiantRCFServer::writeToConsole(const std::string& text)
{
    gameLocal.Printf(text.c_str());
}

void DarkRadiantRCFServer::executeConsoleCommand(const std::string& command)
{
	assert(cmdSystem != NULL);

	// Issue the command to the idCmdSystem
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, command.c_str());
}

void DarkRadiantRCFServer::startConsoleBuffering(int dummy) 
{
	// Let the console output be redirected to the given buffer
	_client = ClientPtr(
		new RcfClient<DarkRadiantRCFService>(RCF::TcpEndpoint("localhost", 50002))
	);
	common->BeginRedirect(_buffer, sizeof(_buffer), FlushBuffer);
}

void DarkRadiantRCFServer::endConsoleBuffering(int dummy)
{
	common->EndRedirect();
	_client = ClientPtr();
}

std::string DarkRadiantRCFServer::readConsoleBuffer(int dummy) {
	//std::string temp(_largeBuffer);
	//_largeBuffer.clear();
	return "";
}

void DarkRadiantRCFServer::Cycle() 
{
	_server.cycle();
}

void DarkRadiantRCFServer::FlushBuffer(const char* text)
{
	if (_client != NULL)
	{
		try	{
			std::string textStr(text);
			_client->writeToConsole(textStr);
		}
		catch (const std::exception&)	{
			// Disable the client to avoid application block
			_client = ClientPtr();
			common->EndRedirect();
		}
	}
}

void DarkRadiantRCFServer::Frame()
{
	//DM_LOG(LC_AI, LT_INFO).LogString("RunFrame!\r");
	if (instance != NULL)
	{
		instance->Cycle();
	}
	/*if (_client != NULL)
	{
		try	{
			std::string textStr(text);
			_client->writeToConsole(textStr);
		}
		catch (const std::exception&)	{
			std::cout << "caught";
		}
	}*/
}

// Define the static member
DarkRadiantRCFServer::ClientPtr DarkRadiantRCFServer::_client;
DarkRadiantRCFServer* DarkRadiantRCFServer::instance = NULL;
