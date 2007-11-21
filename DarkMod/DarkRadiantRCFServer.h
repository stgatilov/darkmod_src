/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1643 $
 * $Date: 2007-11-19 08:00:36 +0100 (Fr, 02 Nov 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef __DARKRADIANT_RCF_SERVER_H__
#define __DARKRADIANT_RCF_SERVER_H__

#include <boost/shared_ptr.hpp>
#include <RCF/RcfServer.hpp>
#include "DarkRadiantRCFInterface.h"

#define MAX_CONSOLE_BUFFER_SIZE 64

/**
 * greebo: This class encapsulates an RCF Server instance listening
 *         for incoming requests on localhost, port 50001.
 *
 * The server instance is held by idGameLocal and is managed by a boost 
 * smart pointer, which handles the destruction of this object.
 *
 * Instantiation of this object is sufficient for starting the RCF server.
 *
 * Note: the RCF interface functions must exactly match the ones defined in 
 * the declaration file within the DarkRadiant plugin (RCFInterface.h)
 */
class DarkRadiantRCFServer
{
	// The actual RCF Server instance
	RCF::RcfServer _server;

	// The client for accessing DarkRadiant's interface (static
	typedef boost::shared_ptr< RcfClient<DarkRadiantRCFService> > ClientPtr;
	static ClientPtr _client;

	char _buffer[MAX_CONSOLE_BUFFER_SIZE];

	static DarkRadiantRCFServer* instance;

public:
	// Constructor starts the server thread
	DarkRadiantRCFServer();

	// Destructor stops the thread
	~DarkRadiantRCFServer();

	// Lets the server think
	void Cycle();

	// Sends the "COMMAND DONE" signal back to DarkRadiant
	void SignalCommandDone();

	// SourceHook entry point
	static void Frame();

	// --- DarkRadiant RCF interface goes below ----
	void writeToConsole(const std::string& text);
	void executeConsoleCommand(const std::string& command);
	void startConsoleBuffering(int portNum);
	void endConsoleBuffering(int dummy);

private:
	static void FlushBuffer(const char* text);
};
typedef boost::shared_ptr<DarkRadiantRCFServer> DarkRadiantRCFServerPtr;

#endif /* __DARKRADIANT_RCF_SERVER_H__ */
