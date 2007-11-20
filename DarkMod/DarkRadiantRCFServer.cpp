/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-11-19 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "DarkRadiantRCFServer.h"

#include "RCFServiceDeclaration.h"

#include <RCF/RcfServer.hpp>
#include <RCF/TcpEndpoint.hpp>

#include "../idlib/precompiled.h"

class D3ConsoleWriterImpl
{
public:
    void writeToConsole(const std::string& text)
    {
        gameLocal.Printf(text.c_str());
    }
};

D3ConsoleWriterImpl doom3ConsoleWriterInstance;

void TestStartRCFServer()
{
    RCF::RcfServer server(RCF::TcpEndpoint(50001));
    server.bind<D3ConsoleWriter>(doom3ConsoleWriterInstance);
    server.start(false);
	for (int i = 0; i < 5000; i++) 
	{
		Sleep(1);
		server.cycle();
	}
	server.stop();
}
