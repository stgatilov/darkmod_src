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
	EscapePointEvaluator((conditions.distanceOption == DIST_FARTHEST) ? -1 : 1),
	_conditions(conditions),
	_bestTime(0)
{
	// Get the starting area number
	_startAreaNum = conditions.aas->PointAreaNum(conditions.fromPosition);
}

bool AnyEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (_conditions.distanceOption == DIST_DONT_CARE)
	{
		_bestId = escapePoint.id;
		return false; // we have a point, we don't care about distance, end the search
	}

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
	EscapePointEvaluator((conditions.distanceOption == DIST_FARTHEST) ? -1 : 1),
	_conditions(conditions),
	_bestTime(0)
{
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

	if (_conditions.distanceOption == DIST_DONT_CARE)
	{
		_bestId = escapePoint.id;
		return false; // we have a point, we don't care about distance, end the search
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

/**
 * FriendlyEscapePointFinder
 */
FriendlyEscapePointFinder::FriendlyEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator((conditions.distanceOption == DIST_FARTHEST) ? -1 : 1),
	_conditions(conditions),
	_bestTime(0),
	_team(conditions.self.GetEntity()->team)
{
	// Get the starting area number
	_startAreaNum = conditions.aas->PointAreaNum(conditions.fromPosition);
}

bool FriendlyEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!gameLocal.m_RelationsManager->IsFriend(escapePoint.team, _team))
	{
		// Not guarded, continue the search
		DM_LOG(LC_AI, LT_DEBUG).LogString("Escape point %d is not friendly.\r", escapePoint.id);
		return true;
	}

	if (_conditions.distanceOption == DIST_DONT_CARE)
	{
		_bestId = escapePoint.id;
		return false; // we have a point, we don't care about distance, end the search
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

/**
 * FriendlyGuardedEscapePointFinder
 */
FriendlyGuardedEscapePointFinder::FriendlyGuardedEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator((conditions.distanceOption == DIST_FARTHEST) ? -1 : 1),
	_conditions(conditions),
	_bestTime(0),
	_team(conditions.self.GetEntity()->team)
{
	// Get the starting area number
	_startAreaNum = conditions.aas->PointAreaNum(conditions.fromPosition);
}

bool FriendlyGuardedEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!escapePoint.isGuarded || !gameLocal.m_RelationsManager->IsFriend(escapePoint.team, _team))
	{
		// Not guarded, continue the search
		DM_LOG(LC_AI, LT_DEBUG).LogString("Escape point %d is either not friendly or not guarded.\r", escapePoint.id);
		return true;
	}

	if (_conditions.distanceOption == DIST_DONT_CARE)
	{
		_bestId = escapePoint.id;
		return false; // we have a point, we don't care about distance, end the search
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