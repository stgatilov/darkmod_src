/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2338 $
 * $Date: 2008-05-15 18:23:41 +0200 (Do, 15 Mai 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_COMMUNICATION_TASK_H__
#define __AI_COMMUNICATION_TASK_H__

#include "Task.h"

namespace ai
{

#define BARK_PRIORITY_DEF "atdm:ai_bark_priority"
#define VERY_HIGH_BARK_PRIORITY 9000000

class CommunicationTask;
typedef boost::shared_ptr<CommunicationTask> CommunicationTaskPtr;

/** 
 * A CommunicationTask is a more specialised task, extending
 * the interface by a few methods, which facilitate the handling
 * of concurrent AI barks. A bark sound always has an associated 
 * priority, which the CommunicationSubsystem is using to decide
 * whether a new incoming bark is allowed to override an existing
 * one or not.
 */
class CommunicationTask :
	public Task
{
protected:
	// The sound to play
	idStr _soundName;

	// The priority of this sound
	int _priority;

	int _barkStartTime;
	int _barkLength;

	// Private constructors
	CommunicationTask();

	CommunicationTask(const idStr& soundName);

public:
	// Returns the priority of this bark
	int GetPriority();

	// Returns TRUE if the task is still playing a bark 
	// (this is excluding any possible delay after the actual sound)
	bool IsBarking();

	const idStr& GetSoundName();

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);
};

} // namespace ai

#endif /* __AI_COMMUNICATION_TASK_H__ */
