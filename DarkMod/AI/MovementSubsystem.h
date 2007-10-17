/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_MOVEMENT_SUBSYSTEM_H__
#define __AI_MOVEMENT_SUBSYSTEM_H__

#include "Subsystem.h"

namespace ai
{

class MovementSubsystem :
	public Subsystem
{
public:
	MovementSubsystem(idAI* owner);

	// Called regularly by the Mind to run the currently assigned routine.
	virtual void PerformTask();

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);
};
typedef boost::shared_ptr<MovementSubsystem> MovementSubsystemPtr;

} // namespace ai

#endif /* __AI_MOVEMENT_SUBSYSTEM_H__ */
