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

static bool init_version = FileVersionList("$Id: RouteNode.cpp 1435 2008-05-12 10:15:28Z greebo $", init_version);

#include "RouteNode.h"

namespace eas {

RouteNode::RouteNode() :
	type(ACTION_WALK),
	toArea(0),
	toCluster(0),
	elevator(-1)
{}

RouteNode::RouteNode(ActionType t, int goalArea, int goalCluster, int elevatorNum) :
	type(t),
	toArea(goalArea),
	toCluster(goalCluster),
	elevator(elevatorNum)
{}

// Copy constructor
RouteNode::RouteNode(const RouteNode& other) :
	type(other.type),
	toArea(other.toArea),
	toCluster(other.toCluster),
	elevator(other.elevator)
{}

bool RouteNode::operator==(const RouteNode& other) const
{
	return (type == other.type && toArea == other.toArea && toCluster == other.toCluster && elevator == other.elevator);
}

bool RouteNode::operator!=(const RouteNode& other) const
{
	return !operator==(other);
}

} // namespace eas
