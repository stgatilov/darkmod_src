/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision: 5292 $ (Revision of last commit) 
 $Date: 2012-02-23 17:17:34 +0100 (Do, 23 Feb 2012) $ (Date of last commit)
 $Author: grayman $ (Author of last commit)
 
******************************************************************************/

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id: PathSetMovetypeTask.cpp 5292 2012-02-23 16:17:34Z grayman $");

#include "../Memory.h"
#include "PathSetMovetypeTask.h"
#include "../Library.h"

namespace ai
{

PathSetMovetypeTask::PathSetMovetypeTask() :
	PathTask()
{}

PathSetMovetypeTask::PathSetMovetypeTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}


// Get the name of this task
const idStr& PathSetMovetypeTask::GetName() const
{
	static idStr _name(TASK_PATH_SET_MOVETYPE);

	return _name;
}

void PathSetMovetypeTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	PathTask::Init(owner, subsystem);

	// Check the "movetype" spawnarg of this path entity
	idStr movetype = "MOVETYPE_";
		
	movetype.Append(_path.GetEntity()->spawnArgs.GetString("movetype", "ANIM"));
	owner->SetMoveType(movetype);

//	idPathCorner* nextPath = owner->GetMemory().nextPath.GetEntity();

}

bool PathSetMovetypeTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path Corner Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	return true;

}


// Save/Restore methods
void PathSetMovetypeTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);
}

void PathSetMovetypeTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);
}

PathSetMovetypeTaskPtr PathSetMovetypeTask::CreateInstance()
{
	return PathSetMovetypeTaskPtr(new PathSetMovetypeTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathSetMovetypeTaskRegistrar(
	TASK_PATH_SET_MOVETYPE, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathSetMovetypeTask::CreateInstance) // Instance creation callback
);

} // namespace ai
