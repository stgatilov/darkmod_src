/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2167 $
 * $Date: 2008-04-06 20:41:22 +0200 (So, 06 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: MultiStateMover.cpp 2167 2008-04-06 18:41:22Z greebo $", init_version);

#include "MultiStateMover.h"

CLASS_DECLARATION( idElevator, CMultiStateMover )
	/*EVENT( EV_ReachedPos,	CMultiStateMover::Event_UpdateMove )
	EVENT( EV_ReachedAng,	CMultiStateMover::Event_UpdateRotation )
	EVENT( EV_StopMoving,	CMultiStateMover::Event_StopMoving )
	EVENT( EV_StopRotating,	CMultiStateMover::Event_StopRotating )
	EVENT( EV_MoveToPos,	CMultiStateMover::Event_MoveToPos )*/
	EVENT( EV_Activate,		CMultiStateMover::Event_Activate )
	EVENT( EV_PostSpawn,	CMultiStateMover::Event_PostSpawn )
END_CLASS

CMultiStateMover::CMultiStateMover()
{}

void CMultiStateMover::Spawn() 
{
	// Schedule a post-spawn event to analyse the targets
	PostEventMS(&EV_PostSpawn, 1);
}

void CMultiStateMover::Event_PostSpawn() 
{
	// Go through all the targets and find the PositionEntities
	for (int i = 0; i < targets.Num(); i++) 
	{
		idEntity* target = targets[i].GetEntity();

		if (!target->IsType(CMultiStateMoverPosition::Type)) 
		{
			continue;
		}

		DM_LOG(LC_ENTITY, LT_INFO).LogString("Parsing multistate position entity %s.\r", target->name.c_str());
		
		idStr positionName;
		if (!target->spawnArgs.GetString("position", "", positionName) || positionName.IsEmpty())
		{
			gameLocal.Warning("'position' spawnarg on %s is missing.\n", target->name.c_str());
			continue;
		}

		if (GetPositionInfoIndex(positionName) != -1) 
		{
			gameLocal.Warning("Multiple positions with name %s defined for %s.\n", positionName.c_str(), name.c_str());
			continue;
		}

		// greebo: Seems like the position entity is valid, let's build an info structure
		MoverPositionInfo info;

		info.positionEnt = static_cast<CMultiStateMoverPosition*>(target);
		info.name = positionName;

		positionInfo.Append(info);		
	}

	DM_LOG(LC_ENTITY, LT_INFO).LogString("Found %d multistate position entities.\r", positionInfo.Num());
}

void CMultiStateMover::Save(idSaveGame *savefile) const
{
	// TODO or remove
}

void CMultiStateMover::Restore(idRestoreGame *savefile)
{
	// TODO or remove
}

void CMultiStateMover::Activate(idEntity* activator)
{
	// TODO
}

int CMultiStateMover::GetPositionInfoIndex(const idStr& name) const
{
	for (int i = 0; i < positionInfo.Num(); i++) 
	{
		if (positionInfo[i].name == name) 
		{
			return i;
		}
	}

	return -1; // not found
}

void CMultiStateMover::Event_Activate(idEntity* activator)
{
	Activate(activator);
}
