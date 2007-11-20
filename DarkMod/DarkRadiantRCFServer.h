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

/**
 * greebo: This class encapsulates an RCF Server instance listening
 *         for incoming requests on localhost, port 50001.
 *
 * The server instance is held by idGameLocal and is managed by a boost 
 * smart pointer, which handles the destruction of this object.
 *
 * Instantiation of this object is sufficient for starting the RCF server.
 */
class DarkRadiantRCFServer
{
public:
	// Constructor starts the server thread
	DarkRadiantRCFServer();

	// Destructor stops the thread
	~DarkRadiantRCFServer();
};
typedef boost::shared_ptr<DarkRadiantRCFServer> DarkRadiantRCFServerPtr;

#endif /* __DARKRADIANT_RCF_SERVER_H__ */
