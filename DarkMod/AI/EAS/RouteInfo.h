/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-12 18:53:28 +0200 (Mo, 12 May 2008) $
 * $Author: greebo $
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

	RouteInfo() :
		routeType(ROUTE_TO_CLUSTER),
		target(-1)
	{}

	RouteInfo(RouteType type, int targetNum) :
		routeType(type),
		target(targetNum)
	{}

	// Copy constructor
	RouteInfo(const RouteInfo& other) :
		routeType(other.routeType),
		target(other.target)
	{
		// Copy the RouteNodes of the other list, one by one
		for (RouteNodeList::const_iterator otherNode = other.routeNodes.begin();
			otherNode != other.routeNodes.end(); otherNode++)
		{
			RouteNodePtr newNode(new RouteNode(**otherNode));
			routeNodes.push_back(newNode);
		}
	}

	bool operator==(const RouteInfo& other) const
	{
		if (routeType == other.routeType && target == other.target && routeNodes.size() == other.routeNodes.size())
		{
			for (RouteNodeList::const_iterator i = routeNodes.begin(), j = other.routeNodes.begin(); i != routeNodes.end(); i++, j++)
			{
				if (*i != *j)
				{
					return false; // RouteNode mismatch
				}
			}

			return true; // everything matched
		}

		return false; // routeType, routeNodes.size() or target mismatched
	}

	bool operator!=(const RouteInfo& other) const
	{
		return !operator==(other);
	}
};
typedef boost::shared_ptr<RouteInfo> RouteInfoPtr;
typedef std::list<RouteInfoPtr> RouteInfoList;
typedef std::vector<RouteInfoList> RouteInfoListVector;

} // namespace eas

#endif /* __AI_EAS_ROUTEINFO_H__ */
