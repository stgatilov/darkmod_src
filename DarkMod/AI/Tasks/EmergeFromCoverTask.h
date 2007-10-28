/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __AI_EMERGE_FROM_COVER_TASK_H__
#define __AI_EMERGE_FROM_COVER_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_EMERGE_FROM_COVER "Emerge_From_Cover"

class EmergeFromCoverTask;
typedef boost::shared_ptr<EmergeFromCoverTask> EmergeFromCoverTaskPtr;

class EmergeFromCoverTask :
	public Task
{
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
	static EmergeFromCoverTaskPtr CreateInstance();

};

} // namespace ai

#endif /* __AI_EMERGE_FROM_COVER_TASK_H__ */
