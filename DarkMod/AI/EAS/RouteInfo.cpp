/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-12 10:15:28 +0200 (Mo, 12 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: RouteInfo.cpp 1435 2008-05-12 10:15:28Z greebo $", init_version);

#include "RouteInfo.h"

namespace eas {

RouteInfo::RouteInfo() :
	routeType(ROUTE_TO_CLUSTER),
	target(-1)
{}

RouteInfo::RouteInfo(RouteType type, int targetNum) :
	routeType(type),
	target(targetNum)
{}

// Copy constructor
RouteInfo::RouteInfo(const RouteInfo& other) :
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

bool RouteInfo::operator==(const RouteInfo& other) const
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

bool RouteInfo::operator!=(const RouteInfo& other) const
{
	return !operator==(other);
}

} // namespace eas
