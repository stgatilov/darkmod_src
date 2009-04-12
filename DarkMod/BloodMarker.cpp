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
#include "StimResponse/Stim.h"

const idEventDef EV_GenerateBloodSplat("TDM_GenerateBloodSplat", NULL);

#define BLOODSPLAT_RESPAWN_INTERVAL 1000 // in msec, must coincide with the fadetime in the material definition

CLASS_DECLARATION( idEntity, CBloodMarker )
	EVENT( EV_GenerateBloodSplat, CBloodMarker::Event_GenerateBloodSplat )
END_CLASS

void CBloodMarker::Event_GenerateBloodSplat()
{
	idVec3 dir = gameLocal.GetGravity();
	dir.Normalize();

	if (!_isFading)
	{
		gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), dir, 3, false, _size, _bloodSplat, _angle);
		PostEventMS(&EV_GenerateBloodSplat, BLOODSPLAT_RESPAWN_INTERVAL);
	}
	else
	{
		// We're fading, just spawn one last decal and schedule our removal
		gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), dir, 3, false, _size, _bloodSplatFading, _angle);

		PostEventMS(&EV_Remove, 1000);
	}
}

void CBloodMarker::Init(const idStr& splat, const idStr& splatFading, float size)
{
	_bloodSplat = splat;
	_bloodSplatFading = splatFading;

	// randomly rotate the decal winding
	_angle = gameLocal.random.RandomFloat() * idMath::TWO_PI;
	_size = size;
	_isFading = false;

	AddResponse(ST_WATER);
	ResponseEnable(ST_WATER, 1);
}

void CBloodMarker::OnStim(CStim* stim, idEntity* stimSource)
{
	// Call the base class in any case
	idEntity::OnStim(stim, stimSource);

	if (stim->m_StimTypeId == ST_WATER)
	{
		_isFading = true;
	}
}

//-----------------------------------------------------------------------------------

void CBloodMarker::Save( idSaveGame *savefile ) const
{
	savefile->WriteString(_bloodSplat);
	savefile->WriteString(_bloodSplatFading);
	savefile->WriteFloat(_angle);
	savefile->WriteFloat(_size);
	savefile->WriteBool(_isFading);
}

//-----------------------------------------------------------------------------------

void CBloodMarker::Restore( idRestoreGame *savefile )
{
	savefile->ReadString(_bloodSplat);
	savefile->ReadString(_bloodSplatFading);
	savefile->ReadFloat(_angle);
	savefile->ReadFloat(_size);
	savefile->ReadBool(_isFading);
}
