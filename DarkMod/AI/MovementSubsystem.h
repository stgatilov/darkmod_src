/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3354 $
 * $Date: 2009-04-04 13:41:43 +0200 (Sa, 04 Apr 2009) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_MOVEMENTSUBSYSTEM_H__
#define __AI_MOVEMENTSUBSYSTEM_H__

#include <boost/shared_ptr.hpp>
#include <list>

#include "Tasks/Task.h"
#include "Subsystem.h"
#include "Queue.h"

namespace ai
{


class MovementSubsystem :
	public Subsystem 

{

public:
	MovementSubsystem(SubsystemId subsystemId, idAI* owner);

	// Called regularly by the Mind to run the currently assigned routine.
	// @returns: TRUE if the subsystem is enabled and the task was performed, 
	// @returns: FALSE if the subsystem is disabled and nothing happened.
	virtual bool PerformTask();

	virtual void CheckBlocked();


};
typedef boost::shared_ptr<MovementSubsystem> MovementSubsystemPtr;

} // namespace ai

#endif /* __AI_MOVEMENTSUBSYSTEM_H__ */
