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

#define MAX_CONSOLE_BUFFER_SIZE 1

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

	std::string _largeBuffer;

	char _buffer[MAX_CONSOLE_BUFFER_SIZE];

public:
	// Constructor starts the server thread
	DarkRadiantRCFServer();

	// Destructor stops the thread
	~DarkRadiantRCFServer();

	// --- DarkRadiant RCF interface goes below ----
	void writeToConsole(const std::string& text);
	void executeConsoleCommand(const std::string& command);
	std::string readConsoleBuffer(int dummy);
	void startConsoleBuffering(int dummy);
	void endConsoleBuffering(int dummy);

private:
	// Holds the pointer to this instance during executeConsoleCommand()
	static DarkRadiantRCFServer* instance;

	static void FlushBuffer(const char* text);
};
typedef boost::shared_ptr<DarkRadiantRCFServer> DarkRadiantRCFServerPtr;

#endif /* __DARKRADIANT_RCF_SERVER_H__ */
