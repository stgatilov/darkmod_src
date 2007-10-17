/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_SUBSYSTEM_H__
#define __AI_SUBSYSTEM_H__

#include <boost/shared_ptr.hpp>

namespace ai
{

enum SubsystemId {
	SubsysMovement = 0,
	SubsysSenses,
	SubsysCommunication,
	SubsysAction,
	SubsystemCount,
};

class Subsystem
{
protected:
	idEntityPtr<idAI> _owner;

public:
	Subsystem(idAI* owner) {
		assert(owner != NULL);
		_owner = owner;
	}

	// Called regularly by the Mind to run the currently assigned routine.
	virtual void PerformTask() = 0;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const
	{
		_owner.Save(savefile);
	}

	virtual void Restore(idRestoreGame* savefile)
	{
		_owner.Restore(savefile);
	}
};
typedef boost::shared_ptr<Subsystem> SubsystemPtr;

} // namespace ai

#endif /* __AI_SUBSYSTEM_H__ */
