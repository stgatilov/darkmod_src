/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_EAS_ROUTEINFO_H__
#define __AI_EAS_ROUTEINFO_H__

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "RouteNode.h"

namespace eas {

enum RouteType {
	ROUTE_DUMMY = 0,	// placeholder
	ROUTE_TO_AREA,		// a route to an AAS area
	ROUTE_TO_CLUSTER,	// a route to an AAS cluster
	NUM_ROUTE_TYPES,
};

// A route info contains information of how to get to a specific target
struct RouteInfo
{
	RouteType routeType;		// ROUTE_TO_AREA or ROUTE_TO_CLUSTER, ...
	int target;					// either the target AREA or the target CLUSTER number, depending on routeType
	RouteNodeList routeNodes;	// contains the actual route node chain (WALK, USE_ELEVATOR, WALK, etc.)

	// Default constructor
	RouteInfo();

	// Specialised constructor
	RouteInfo(RouteType type, int targetNum);

	// Copy constructor
	RouteInfo(const RouteInfo& other);

	bool operator==(const RouteInfo& other) const;
	bool operator!=(const RouteInfo& other) const;

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};
typedef boost::shared_ptr<RouteInfo> RouteInfoPtr;
typedef std::list<RouteInfoPtr> RouteInfoList;
typedef std::vector<RouteInfoList> RouteInfoListVector;

} // namespace eas

#endif /* __AI_EAS_ROUTEINFO_H__ */
