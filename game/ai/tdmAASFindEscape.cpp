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

tdmAASFindEscape::tdmAASFindEscape(const idVec3& threatPosition, float maxDist) :
	_threatPosition(threatPosition),
	_maxDistSqr(maxDist*maxDist),
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
	gameRenderWorld->DrawTextA(numberStr.c_str(), aas->AreaCenter(areaNum), 1, colorRed, viewAngles, 1, 20000);

	float distSqr(( _threatPosition.ToVec2() - areaCenter.ToVec2() ).LengthSqr());

	DM_LOG(LC_AI, LT_DEBUG).LogString("Testing area: %d, distSqr = %f, maxDistSqr = %f\r", areaNum, distSqr, _maxDistSqr);

	if (distSqr < _maxDistSqr && distSqr > _bestDistSqr)
	{
		// We've got a new best candidate, which is farther away than the previous candidate
		_bestDistSqr = distSqr;

		// Fill the goal with the new values
		_goal.areaNum = areaNum;
		_goal.origin = areaCenter;
	}

	// Always return false, we're collecting all areas
	return false;
}
