/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_SEARCH_TASK_H__
#define __AI_SEARCH_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_SEARCH "Search"

class SearchTask;
typedef boost::shared_ptr<SearchTask> SearchTaskPtr;

class SearchTask :
	public Task
{
public:
	// Get the name of this task
	virtual const idStr& GetName() const;

	// Override the base Init method
	virtual void Init(idAI* owner, Subsystem& subsystem);

	virtual bool Perform(Subsystem& subsystem);

	// Creates a new Instance of this task
	static SearchTaskPtr CreateInstance();

private:
	// Gets called when a new hiding spot should be acquired for searching.
	// Stores the result in the AI's Memory (hiding spot indices)
	// return TRUE when a hiding spot is available, FALSE if not.
	bool ChooseNextHidingSpotToSearch(idAI* owner);
};

} // namespace ai

#endif /* __AI_SEARCH_TASK_H__ */
