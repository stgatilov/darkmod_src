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

static bool init_version = FileVersionList("$Id: MultiStateMoverPosition.cpp 2167 2008-04-06 18:41:22Z greebo $", init_version);

#include "MultiStateMoverPosition.h"
#include "MultiStateMover.h"

CLASS_DECLARATION( idEntity, CMultiStateMoverPosition )
END_CLASS

void CMultiStateMoverPosition::Spawn() 
{
	// nothing to do yet
}

void CMultiStateMoverPosition::OnMultistateMoverArrive(CMultiStateMover* mover)
{
	if (mover == NULL) return;

	// First, check if we should trigger our targets
	if (spawnArgs.GetBool("always_trigger_targets", "1"))
	{
		ActivateTargets(mover);
	}

	// Run the mover event script
	RunMoverEventScript("call_on_arrive", mover);
}

void CMultiStateMoverPosition::OnMultistateMoverLeave(CMultiStateMover* mover)
{
	if (mover == NULL) return;

	// First, check if we should trigger our targets
	if (spawnArgs.GetBool("always_trigger_targets", "1"))
	{
		ActivateTargets(mover);
	}

	// Run the mover event script
	RunMoverEventScript("call_on_leave", mover);
}

void CMultiStateMoverPosition::RunMoverEventScript(const idStr& spawnArg, CMultiStateMover* mover)
{
	idStr scriptFuncName;
	if (!spawnArgs.GetString(spawnArg, "", scriptFuncName))
	{
		return; // no scriptfunction
	}

	// Script function signature is like this: void scriptobj::onMultiStateMover(entity mover)
	idThread* thread = CallScriptFunctionArgs(scriptFuncName, true, 0, "ee", this, mover);
	if (thread != NULL)
	{
		// greebo: Run the thread at once, the script result might be needed below.
		thread->Execute();
	}
}
