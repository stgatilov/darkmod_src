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
 * FarthestEscapePointFinder
 */
FarthestEscapePointFinder::FarthestEscapePointFinder(const EscapeConditions& conditions) :
	_conditions(conditions),
	_bestTime(-1)
{
	// The location of the threat
	_threatOrigin = conditions.fromEntity.GetEntity()->GetPhysics()->GetOrigin();

	// Get the starting area number
	_startAreaNum = conditions.aas->PointAreaNum(conditions.fromPosition);
}

bool FarthestEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	int travelTime;
	int travelFlags(TFL_WALK|TFL_AIR|TFL_DOOR);

	// Calculate the traveltime
	idReachability* reach;
	_conditions.aas->RouteToGoalArea(_startAreaNum, _conditions.fromPosition, escapePoint.areaNum, travelFlags, travelTime, &reach);
	
	DM_LOG(LC_AI, LT_INFO).LogString("Traveltime to point %d = %d\r", escapePoint.id, travelTime);

	if (travelTime > _bestTime)
	{
		// Yes, this is a better flee point
		_bestId = escapePoint.id;
		_bestTime = travelTime;
	}

	return true; // TRUE = continue search
}

/**
 * NearestGuardedEscapePointFinder
 */
NearestGuardedEscapePointFinder::NearestGuardedEscapePointFinder(const EscapeConditions& conditions) :
	_conditions(conditions),
	_bestTime(-1)
{
	// The location of the threat
	_threatOrigin = conditions.fromEntity.GetEntity()->GetPhysics()->GetOrigin();

	// Get the starting area number
	_startAreaNum = conditions.aas->PointAreaNum(conditions.fromPosition);
}

bool NearestGuardedEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	int travelTime;
	int travelFlags(TFL_WALK|TFL_AIR|TFL_DOOR);

	// Calculate the traveltime
	idReachability* reach;
	_conditions.aas->RouteToGoalArea(_startAreaNum, _conditions.fromPosition, escapePoint.areaNum, travelFlags, travelTime, &reach);
	
	DM_LOG(LC_AI, LT_INFO).LogString("Traveltime to point %d = %d\r", escapePoint.id, travelTime);

	if (travelTime > _bestTime)
	{
		// Yes, this is a better flee point
		_bestId = escapePoint.id;
		_bestTime = travelTime;
	}

	return true; // TRUE = continue search
}