/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_THROW_OBJECT_TASK_H__
#define __AI_THROW_OBJECT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_THROW_OBJECT "Throw_Object"

class ThrowObjectTask;
typedef boost::shared_ptr<ThrowObjectTask> ThrowObjectTaskPtr;

class ThrowObjectTask :
	public Task
{
	int _projectileDelayMin;
	int _projectileDelayMax;
	int _nextThrowObjectTime;

public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static ThrowObjectTaskPtr CreateInstance();

};

} // namespace ai

#endif /* __AI_THROW_OBJECT_TASK_H__ */
