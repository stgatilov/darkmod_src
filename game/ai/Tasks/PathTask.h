/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __AI_PATH_TASK_H__
#define __AI_PATH_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH "Path"

class PathTask;
typedef std::shared_ptr<PathTask> PathTaskPtr;

class PathTask :
	public Task
{
protected:
	idEntityPtr<idPathCorner> _path;
	float _accuracy;
	bool _activateTargets; // grayman #3670

	PathTask();

public:
	PathTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const = 0;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem) = 0;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);


	// Class-specific methods
	virtual void SetTargetEntity(idPathCorner* path);
};

} // namespace ai

#endif /* __AI_PATH_TASK_H__ */
