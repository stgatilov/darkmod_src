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
	EVENT( EV_Activate,		CMultiStateMover::Event_Activate )
	EVENT( EV_PostSpawn,	CMultiStateMover::Event_PostSpawn )
END_CLASS

CMultiStateMover::CMultiStateMover()
{}

void CMultiStateMover::Spawn() 
{
	forwardDirection = spawnArgs.GetVector("forward_direction", "0 0 1");
	forwardDirection.Normalize();

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
	savefile->WriteInt(positionInfo.Num());
	for (int i = 0; i < positionInfo.Num(); i++)
	{
		positionInfo[i].positionEnt.Save(savefile);
		savefile->WriteString(positionInfo[i].name);
	}

	savefile->WriteVec3(forwardDirection);
}

void CMultiStateMover::Restore(idRestoreGame *savefile)
{
	int num;
	savefile->ReadInt(num);
	positionInfo.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		positionInfo[i].positionEnt.Restore(savefile);
		savefile->ReadString(positionInfo[i].name);
	}

	savefile->ReadVec3(forwardDirection);
}

void CMultiStateMover::Activate(idEntity* activator)
{
	if (activator == NULL) return;

	// Get the "position" spawnarg from the activator
	idStr targetPosition;
	if (!activator->spawnArgs.GetString("position", "", targetPosition))
	{
		return;
	}

	int positionIdx = GetPositionInfoIndex(targetPosition);

	if (positionIdx == -1) 
	{
		gameLocal.Warning("Multistate mover is targetted by an entity with unknown 'position': %s", targetPosition.c_str());
		return;
	}

	// We appear to have a valid position index, start moving
	idEntity* positionEnt = positionInfo[positionIdx].positionEnt.GetEntity();
	const idVec3& targetPos = positionEnt->GetPhysics()->GetOrigin();
	assert(positionEnt != NULL);

	// We're done moving if the velocity is very close to zero
	bool isDoneMoving = GetPhysics()->GetLinearVelocity().Length() <= VECTOR_EPSILON;

	if (isDoneMoving && spawnArgs.GetBool("trigger_on_leave", "0")) 
	{
		// We're leaving our position, trigger targets
		ActivateTargets(this);
	}

	// greebo: Look if we need to control the rotation of the targetted rotators
	if (spawnArgs.GetBool("control_gear_direction", "0")) 
	{
		// Check if we're moving forward or backward
		idVec3 moveDir = targetPos - GetPhysics()->GetOrigin();
		moveDir.NormalizeFast();

		// The dot product (== angle) shows whether we're moving forward or backwards
		bool movingForward = (moveDir * forwardDirection >= 0);

		for (int i = 0; i < targets.Num(); i++)
		{
			idEntity* target = targets[i].GetEntity();
			if (target->IsType(idRotater::Type))
			{
				idRotater* rotater = static_cast<idRotater*>(target);
				rotater->SetDirection(movingForward);
			}
		}
	}

	// Finally start moving (this will update the "stage" members of the mover)
	MoveToPos(targetPos);
}

void CMultiStateMover::DoneMoving()
{
	idMover::DoneMoving();

	if (spawnArgs.GetBool("trigger_on_reached", "0")) 
	{
		// Trigger targets now that we've reached our goal position
		ActivateTargets(this);
	}
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
