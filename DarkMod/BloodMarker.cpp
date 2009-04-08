/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3079 $
 * $Date: 2008-12-06 09:28:50 +0100 (Sa, 06 Dez 2008) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#include "../game/game_local.h"
#pragma hdrstop
#include "BloodMarker.h"

const idEventDef EV_GenerateBloodSplat("TDM_GenerateBloodSplat", NULL);

CLASS_DECLARATION( idEntity, CBloodMarker )
	EVENT( EV_GenerateBloodSplat, CBloodMarker::Event_GenerateBloodSplat )
END_CLASS

void CBloodMarker::Event_GenerateBloodSplat()
{
	idVec3 dir(0, 0, -1);
	gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), dir, 3, false, _size, _bloodSplat, _angle);

	PostEventMS(&EV_GenerateBloodSplat, 5000);
}

void CBloodMarker::Init(const idStr& splat, float size)
{
	_bloodSplat = splat;
	// randomly rotate the decal winding
	_angle = gameLocal.random.RandomFloat() * idMath::TWO_PI;
	_size = size;
}

//-----------------------------------------------------------------------------------

void CBloodMarker::Save( idSaveGame *savefile ) const
{
	savefile->WriteString(_bloodSplat);
	savefile->WriteFloat(_angle);
	savefile->WriteFloat(_size);
}

//-----------------------------------------------------------------------------------

void CBloodMarker::Restore( idRestoreGame *savefile )
{
	savefile->ReadString(_bloodSplat);
	savefile->ReadFloat(_angle);
	savefile->ReadFloat(_size);
}
