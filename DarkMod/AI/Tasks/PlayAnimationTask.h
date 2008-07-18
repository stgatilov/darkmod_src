/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_PLAY_ANIMATION_TASK_H__
#define __AI_PLAY_ANIMATION_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PLAY_ANIMATION "PlayAnimation"

class PlayAnimationTask;
typedef boost::shared_ptr<PlayAnimationTask> PlayAnimationTaskPtr;

class PlayAnimationTask :
	public Task
{
	idStr _animName;

	int _blendFrames;

	bool _playCycle;

	// Private constructor
	PlayAnimationTask();

public:
	// Pass the animation name directly here (like "idle_armwipe")
	// Set playCycle to TRUE if this animation task should play continuously
	PlayAnimationTask(const idStr& animName, int blendFrames, bool playCycle = false);

	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	virtual void OnFinish(idAI* owner);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Creates a new Instance of this task
	static PlayAnimationTaskPtr CreateInstance();

private:
	// Private helper
	void StartAnim(idAI* owner);
};

} // namespace ai

#endif /* __AI_PLAY_ANIMATION_TASK_H__ */
