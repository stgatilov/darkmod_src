/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: MovementSubsystem.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "MovementSubsystem.h"

namespace ai
{

MovementSubsystem::MovementSubsystem(idAI* owner) :
	Subsystem(owner)
{}

// Called regularly by the Mind to run the currently assigned routine.
void MovementSubsystem::PerformTask() 
{
	DM_LOG(LC_AI, LT_INFO).LogString("MovementSubsystem performing.\r");
}

// Save/Restore methods
void MovementSubsystem::Save(idSaveGame* savefile) const
{
	// Pass the call to the base class
	Subsystem::Save(savefile);

	// TODO
}

void MovementSubsystem::Restore(idRestoreGame* savefile)
{
	// Pass the call to the base class
	Subsystem::Restore(savefile);

	// TODO
}

} // namespace ai
