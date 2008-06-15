/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1246 $
 * $Date: 2007-07-29 19:04:18 +0200 (So, 29 Jul 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: tdmAASFindEscape.cpp 1246 2007-08-27 17:04:18Z greebo $", init_version);

#include "tdmAASFindEscape.h"

tdmAASFindEscape::tdmAASFindEscape(
	const idVec3& threatPosition, 
	const idVec3& selfPosition, 
	float minDistToThreat,
	float minDistToSelf
) :
	_threatPosition(threatPosition),
	_selfPosition(selfPosition),
	_minDistThreatSqr(minDistToThreat*minDistToThreat),
	_minDistSelfSqr(minDistToSelf*minDistToSelf),
	_bestDistSqr(0)
{
	_goal.areaNum = -1;
}

bool tdmAASFindEscape::TestArea(const idAAS *aas, int areaNum)
{
	const idVec3 &areaCenter = aas->AreaCenter( areaNum );

	idStr numberStr(areaNum);
	idMat3 viewAngles;
	viewAngles.Identity();
	//gameRenderWorld->DrawText(numberStr.c_str(), aas->AreaCenter(areaNum), 1, colorRed, viewAngles, 1, 5000);

	float distThreatSqr(( _threatPosition.ToVec2() - areaCenter.ToVec2() ).LengthSqr());
	float distSelfSqr(( _selfPosition.ToVec2() - areaCenter.ToVec2() ).LengthSqr());

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING(
		"Testing area: %d, distThreatSqr = %f, distSelfSqr = %f, _minDistThreatSqr = %f\r", 
		areaNum, distThreatSqr, distSelfSqr, _minDistThreatSqr
	);

	// Minimum distances must be obeyed and the distance 
	// to the threat must be greater than the one of the best candidate so far
	if (distThreatSqr > _bestDistSqr && 
		distThreatSqr >= _minDistThreatSqr && 
		distSelfSqr >= _minDistSelfSqr)
	{
		// We've got a new best candidate, which is farther away than the previous candidate
		_bestDistSqr = distThreatSqr;

		// Fill the goal with the new values
		_goal.areaNum = areaNum;
		_goal.origin = areaCenter;

		// There's a 10% chance that the search is truncated here.
		if (gameLocal.random.RandomFloat() < 0.1f)
		{
			return true;
		}
	}

	// Always return false, we're collecting all areas
	return false;
}
