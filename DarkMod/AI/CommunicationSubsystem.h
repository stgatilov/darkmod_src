/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_COMMUNICATION_SUBSYSTEM_H__
#define __AI_COMMUNICATION_SUBSYSTEM_H__

#include <boost/shared_ptr.hpp>
#include <list>

#include "Tasks/CommunicationTask.h"
#include "Subsystem.h"
#include "Queue.h"

namespace ai
{

class CommunicationSubsystem : 
	public Subsystem 
{
protected:

	enum EActionTypeOnConflict
	{
		EDefault,	// default behaviour
		EOverride,	// override the existing sound
		EQueue,		// queue after the current sound
		EDiscard,	// discard the new sound
		EPush,		// push on top of existing sound
	};

public:
	CommunicationSubsystem(SubsystemId subsystemId, idAI* owner);

	/**
	 * greebo: Handle a new incoming communication task and decide whether
	 * to push or queue it or do whatever action is defined according to the
	 * settings in the entityDef. 
	 *
	 * Note: Does not accept NULL pointers.
	 *
	 * @returns: TRUE if the new bark has been accepted, FALSE if it has been ignored.
	 */
	bool AddCommTask(const CommunicationTaskPtr& communicationTask);

	// returns the priority of the currently active communication task
	int GetCurrentPriority();

	/**
	 * greebo: Queues a silence task at the end of the queue. The task
	 * gets the same priority assigned as the last one of the queue.
	 */
	void AddSilence(int duration);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Returns some debug text for console or renderworld display
	virtual idStr GetDebugInfo();

protected:
	// Priority difference is "new snd prio - current snd prio"
	EActionTypeOnConflict GetActionTypeForSound(const CommunicationTaskPtr& communicationTask);

	// Returns the currently active commtask or NULL if no commtask is active
	CommunicationTaskPtr GetCurrentCommTask();
};
typedef boost::shared_ptr<CommunicationSubsystem> CommunicationSubsystemPtr;

} // namespace ai

#endif /* __AI_COMMUNICATION_SUBSYSTEM_H__ */
