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
