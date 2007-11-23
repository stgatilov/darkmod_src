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
	_server.bind<D3DarkModInterface>(*this);
    _server.start(false);

	instance = this;
}

DarkRadiantRCFServer::~DarkRadiantRCFServer()
{
	instance = NULL;
	_server.stop();
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

void DarkRadiantRCFServer::startConsoleBuffering(int portNum) 
{
	// Let the console output be redirected to the given buffer
	_client = ClientPtr(
		new RcfClient<DarkRadiantRCFService>(RCF::TcpEndpoint("localhost", portNum))
	);
	common->BeginRedirect(_buffer, sizeof(_buffer), FlushBuffer);
}

void DarkRadiantRCFServer::endConsoleBuffering(int dummy)
{
	common->EndRedirect();
	_client = ClientPtr();
}

void DarkRadiantRCFServer::SignalCommandDone() 
{
	if (_client != NULL)
	{
		ClientPtr client = _client;
		endConsoleBuffering(1);

		try	{
			// Emit the command done signal to DarkRadiant
			client->signalCommandDone();
		}
		catch (const std::exception&)	{}
	}
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

// Intercepts idCommon::Frame(), lets the server think
void DarkRadiantRCFServer::Frame()
{
	if (instance != NULL)
	{
		instance->Cycle();
	}
}

// Define the static member
DarkRadiantRCFServer::ClientPtr DarkRadiantRCFServer::_client;
DarkRadiantRCFServer* DarkRadiantRCFServer::instance = NULL;
