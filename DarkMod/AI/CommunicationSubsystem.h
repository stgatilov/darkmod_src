/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2822 $
 * $Date: 2008-09-13 06:50:55 +0200 (Sa, 13 Sep 2008) $
 * $Author: greebo $
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

public:
	CommunicationSubsystem(SubsystemId subsystemId, idAI* owner);

	bool AddCommTask(const CommunicationTaskPtr& communicationTask);

	// returns the priority of the currently active communication task
	int GetCurrentPriority();


	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Returns some debug text for console or renderworld display
	virtual idStr GetDebugInfo();

protected:
	// Returns the currently active commtask or NULL if no commtask is active
	CommunicationTaskPtr GetCurrentCommTask();
};
typedef boost::shared_ptr<CommunicationSubsystem> CommunicationSubsystemPtr;

} // namespace ai

#endif /* __AI_COMMUNICATION_SUBSYSTEM_H__ */
