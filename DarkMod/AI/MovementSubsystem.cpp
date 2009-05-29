/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2822 $
 * $Date: 2008-09-13 06:50:55 +0200 (Sa, 13 Sep 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: Subsystem.cpp 2822 2008-09-13 04:50:55Z greebo $", init_version);

#include "MovementSubsystem.h"
#include "Library.h"
#include "States/State.h"

namespace ai
{

MovementSubsystem::MovementSubsystem(SubsystemId subsystemId, idAI* owner) :
	Subsystem(subsystemId, owner)
{}


// Called regularly by the Mind to run the currently assigned routine.
bool MovementSubsystem::PerformTask()
{
	idAI* owner = _owner.GetEntity();
	
	return Subsystem::PerformTask();
}

void MovementSubsystem::CheckBlocked()
{
	
}

} // namespace ai
