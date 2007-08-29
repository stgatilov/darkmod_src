/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mar 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: EscapePointEvaluator.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "EscapePointEvaluator.h"
#include "EscapePointManager.h"

/**
 * AnyEscapePointFinder
 */
AnyEscapePointFinder::AnyEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions.findNearest ? 1 : -1),
	_conditions(conditions),
	_bestTime(0)
{
	// The location of the threat
	_threatOrigin = conditions.fromEntity.GetEntity()->GetPhysics()->GetOrigin();

	// Get the starting area number
	_startAreaNum = conditions.aas->PointAreaNum(conditions.fromPosition);
}

bool AnyEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	int travelTime;
	int travelFlags(TFL_WALK|TFL_AIR|TFL_DOOR);

	// Calculate the traveltime
	idReachability* reach;
	_conditions.aas->RouteToGoalArea(_startAreaNum, _conditions.fromPosition, escapePoint.areaNum, travelFlags, travelTime, &reach);
	
	DM_LOG(LC_AI, LT_INFO).LogString("Traveltime to point %d = %d\r", escapePoint.id, travelTime);

	// Take this if no point has been found yet or if this one is better
	if (_bestId == -1 || travelTime*_distanceMultiplier < _bestTime*_distanceMultiplier)
	{
		// Yes, this is a better flee point
		_bestId = escapePoint.id;
		_bestTime = travelTime;
	}

	return true; // TRUE = continue search
}

/**
 * GuardedEscapePointFinder
 */
GuardedEscapePointFinder::GuardedEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions.findNearest ? 1 : -1),
	_conditions(conditions),
	_bestTime(conditions.findNearest ? 1000000 : -1)
{
	// The location of the threat
	_threatOrigin = conditions.fromEntity.GetEntity()->GetPhysics()->GetOrigin();

	// Get the starting area number
	_startAreaNum = conditions.aas->PointAreaNum(conditions.fromPosition);
}

bool GuardedEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!escapePoint.isGuarded)
	{
		// Not guarded, continue the search
		DM_LOG(LC_AI, LT_DEBUG).LogString("Escape point %d is not guarded.\r", escapePoint.id);
		return true;
	}

	// Escape point is guarded, now calculate the walk distance
	int travelTime;
	int travelFlags(TFL_WALK|TFL_AIR|TFL_DOOR);

	// Calculate the traveltime
	idReachability* reach;
	_conditions.aas->RouteToGoalArea(_startAreaNum, _conditions.fromPosition, escapePoint.areaNum, travelFlags, travelTime, &reach);
	
	DM_LOG(LC_AI, LT_INFO).LogString("Traveltime to point %d = %d\r", escapePoint.id, travelTime);

	if (_bestId == -1 || travelTime*_distanceMultiplier < _bestTime*_distanceMultiplier)
	{
		// Yes, this is a better flee point
		_bestId = escapePoint.id;
		_bestTime = travelTime;
	}

	return true; // TRUE = continue search
}