/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#include "../game/game_local.h"
#pragma hdrstop
#include "AbsenceMarker.h"

CLASS_DECLARATION( idEntity, CAbsenceMarker )
END_CLASS

//##################################################################
// Constructor, Init
//##################################################################

CAbsenceMarker::CAbsenceMarker()
{
	referenced_entityDefNumber = -1;
	referenced_entityDefName.Empty();
	referenced_entityName.Empty();
}

//-----------------------------------------------------------------------------------

bool CAbsenceMarker::initAbsenceReference(idEntity* owner, idBounds& startBounds)
{
	if (owner == NULL)
	{
		return false;
	}
    
	referenced_entityDefNumber = owner->entityDefNumber;
	referenced_entityName = owner->name;
	referenced_entityDefName = owner->GetEntityDefName();

	GetPhysics()->SetClipBox( owner->GetPhysics()->GetBounds(), 1.0f ); // grayman #2853 - set marker's bounds to missing item's bounds

	// Fill with spawnargs of referenced entity
	referenced_spawnArgs = owner->spawnArgs;

	// Move to position where missing entity should have been
	// angua: place marker at the center of the original bounds
	idVec3 markerOrg = startBounds.GetCenter();
	SetOrigin (markerOrg);

	team = owner->team;
	
	// Done
	return true;
}


const idDict& CAbsenceMarker::GetRefSpawnargs() const
{
	return referenced_spawnArgs;
}


//-----------------------------------------------------------------------------------

void CAbsenceMarker::Save( idSaveGame *savefile ) const
{
	// Get current position and orientation of the marker
	idVec3 refPosition = GetPhysics()->GetOrigin();
	idMat3 refOrientation = GetPhysics()->GetAxis();

	savefile->WriteDict( &referenced_spawnArgs );

	// Additional fields
	savefile->WriteInt (ownerTeam);
	savefile->WriteInt (referenced_entityDefNumber);
	savefile->WriteString (referenced_entityName);
	savefile->WriteString (referenced_entityDefName);
	savefile->WriteVec3 (refPosition);
	savefile->WriteMat3 (refOrientation);
}

//-----------------------------------------------------------------------------------

void CAbsenceMarker::Restore( idRestoreGame *savefile )
{
	idVec3 refPosition;
	idMat3 refOrientation;

	savefile->ReadDict(&referenced_spawnArgs);

	// Get the referenced entity def number
	savefile->ReadInt (ownerTeam);
	savefile->ReadInt (referenced_entityDefNumber);
	savefile->ReadString (referenced_entityName);
	savefile->ReadString (referenced_entityDefName);
	savefile->ReadVec3 (refPosition);
	savefile->ReadMat3 (refOrientation);

	// Set position and orientation of marker
	SetOrigin(refPosition);
	SetAxis(refOrientation);
}
