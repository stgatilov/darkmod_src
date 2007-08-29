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

FarthestEscapePointFinder::FarthestEscapePointFinder(const EscapeConditions& conditions) :
	_conditions(conditions),
	_bestId(-1),
	_bestDistance(-1.0f)
{
	// The location of the threat
	_threatOrigin = conditions.fromEntity.GetEntity()->GetPhysics()->GetOrigin();
}

bool FarthestEscapePointFinder::Evaluate(EscapePoint& escapePoint)
{
	// Is this point nearer than the currently best candidate?
	float distance = (_threatOrigin - escapePoint.origin).LengthFast();

	if (_bestId == -1) 
	{
		// No candidate found so far, take this one
		_bestId = escapePoint.id;
		_bestDistance = distance;
		return true; // continue search
	}

	if (distance > _bestDistance)
	{
		// Yes, this is a better flee point
		_bestId = escapePoint.id;
		_bestDistance = distance;
	}

	return true; // TRUE = continue search
}

int FarthestEscapePointFinder::GetBestEscapePoint()
{
	return _bestId;
}
