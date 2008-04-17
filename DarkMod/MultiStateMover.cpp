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

void CMultiStateMover::Event_Activate(idEntity* activator)
{
	Activate(activator);
}
