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

EscapePointEvaluator::EscapePointEvaluator(const EscapeConditions& conditions) :
	_conditions(conditions),
	_bestId(-1), // Set the ID to invalid
	_startAreaNum(conditions.aas->PointAreaNum(conditions.fromPosition)),
	_bestTime(0),
	_distanceMultiplier((conditions.distanceOption == DIST_FARTHEST) ? -1 : 1),
	_threatPosition(conditions.fromEntity.GetEntity()->GetPhysics()->GetOrigin())
{}

bool EscapePointEvaluator::PerformDistanceCheck(EscapePoint& escapePoint)
{
	if (_conditions.distanceOption == DIST_DONT_CARE)
	{
		_bestId = escapePoint.id;
		return false; // we have a valid point, we don't care about distance, end the search
	}

	int travelTime;
	int travelFlags(TFL_WALK|TFL_AIR|TFL_DOOR);

	// Calculate the traveltime
	idReachability* reach;
	_conditions.aas->RouteToGoalArea(_startAreaNum, _conditions.fromPosition, escapePoint.areaNum, travelFlags, travelTime, &reach, _conditions.self.GetEntity());
	
	DM_LOG(LC_AI, LT_INFO).LogString("Traveltime to point %d = %d\r", escapePoint.id, travelTime);

	// Take this if no point has been found yet or if this one is better
	if (_bestId == -1 || travelTime*_distanceMultiplier < _bestTime*_distanceMultiplier)
	{
		// Either the minDistanceToThreat is negative, or the distance has to be larger
		// for the escape point to be considered as better
		if (_conditions.minDistanceToThreat < 0 || 
			(_threatPosition - escapePoint.origin).LengthFast() >= _conditions.minDistanceToThreat)
		{
			// This is a better flee point
			_bestId = escapePoint.id;
			_bestTime = travelTime;
		}
	}

	return true;
}

/**
 * AnyEscapePointFinder
 */
AnyEscapePointFinder::AnyEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions)
{}

bool AnyEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	// Just pass the call to the base class
	return PerformDistanceCheck(escapePoint);
}

/**
 * GuardedEscapePointFinder
 */
GuardedEscapePointFinder::GuardedEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions)
{}

bool GuardedEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!escapePoint.isGuarded)
	{
		// Not guarded, continue the search
		DM_LOG(LC_AI, LT_DEBUG).LogString("Escape point %d is not guarded.\r", escapePoint.id);
		return true;
	}

	return PerformDistanceCheck(escapePoint);
}

/**
 * FriendlyEscapePointFinder
 */
FriendlyEscapePointFinder::FriendlyEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions),
	_team(conditions.self.GetEntity()->team)
{}

bool FriendlyEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!gameLocal.m_RelationsManager->IsFriend(escapePoint.team, _team))
	{
		// Not guarded, continue the search
		DM_LOG(LC_AI, LT_DEBUG).LogString("Escape point %d is not friendly.\r", escapePoint.id);
		return true;
	}

	return PerformDistanceCheck(escapePoint);
}

/**
 * FriendlyGuardedEscapePointFinder
 */
FriendlyGuardedEscapePointFinder::FriendlyGuardedEscapePointFinder(const EscapeConditions& conditions) :
	EscapePointEvaluator(conditions),
	_team(conditions.self.GetEntity()->team)
{}

bool FriendlyGuardedEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	if (!escapePoint.isGuarded || !gameLocal.m_RelationsManager->IsFriend(escapePoint.team, _team))
	{
		// Not guarded, continue the search
		DM_LOG(LC_AI, LT_DEBUG).LogString("Escape point %d is either not friendly or not guarded.\r", escapePoint.id);
		return true;
	}

	return PerformDistanceCheck(escapePoint);
}
