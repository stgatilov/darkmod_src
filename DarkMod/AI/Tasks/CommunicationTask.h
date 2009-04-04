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

class CommunicationTask;
typedef boost::shared_ptr<CommunicationTask> CommunicationTaskPtr;

class CommunicationTask :
	public Task
{
protected:

	idStr _soundName;

	int _priority;

	int _barkStartTime;
	int _barkLength;

	CommunicationTask();

	CommunicationTask(const idStr& soundName);

public:

	int GetPriority();

	bool IsBarking();

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);
};

} // namespace ai

#endif /* __AI_COMMUNICATION_TASK_H__ */
