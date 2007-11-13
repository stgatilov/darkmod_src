/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 864 $
 * $Date: 2007-03-22 19:04:56 +0100 (Do, 22 Mrz 2007) $
 * $Author: sparhawk $
 *
 ***************************************************************************/

#ifndef __STATEQUEUE_H__
#define __STATEQUEUE_H__

#include <list>
#include <sstream>

#include "../DarkModGlobals.h"
#include "Library.h"

namespace ai 
{

/**
 * greebo: Templated queue containing shared ptrs of a certain type (Tasks and States).
 *
 * This is not a priority queue, no sorting happens. It derives from the std::list 
 * class, as this implements most of the methods we need for this queue.
 *
 * The interface is extended by a few convenience methods.
 *
 * The Save/Restore methods work together with the templated Library<> class.
 */
template <class Element>
class Queue :
	public std::list< boost::shared_ptr<Element> >
{
	// Parent list type
	typedef std::list<boost::shared_ptr<Element> > ListType;
		
	// greebo: Don't define data members in a class deriving from an STL container
	// (std::list destructor is non-virtual)

	// Shortcut typedef
	typedef boost::shared_ptr<Element> ElementPtr;

public:
	/**
	* Returns the entire contents of this queue as a string, for debugging purposes.
	* Relies on the Elements having a member method GetName()
	*/
	const std::string DebuggingInfo() const
	{
		std::stringstream debugInfo;
		for (typename ListType::const_iterator i = ListType::begin(); 
			 i != ListType::end(); 
			 ++i)
		{
			debugInfo << (*i)->GetName();
			debugInfo << "\n";
		}
		return debugInfo.str();
	}
	
	/**
	* Save this data structure to a savefile
	*/
	void Save(idSaveGame *savefile) const
	{
		savefile->WriteInt(ListType::size());
		for (typename ListType::const_iterator i = ListType::begin(); 
			 i != ListType::end(); 
			 ++i)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Saving element %s.\r", (*i)->GetName().c_str());
			savefile->WriteString((*i)->GetName().c_str());

			// Write the Element data into the savefile
			(*i)->Save(savefile);
		}
	}

	/**
	* Restore this data structure from a savefile
	*/
	void Restore(idRestoreGame *savefile)
	{
		// Clear the queue before restoring
		ListType::clear();
		
		int elements;
		savefile->ReadInt(elements);
		for (int i = 0; i < elements; i++)
		{
			idStr str;
			savefile->ReadString(str);
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Restoring task %s.\r", str.c_str());

			ElementPtr element = Library<Element>::Instance().CreateInstance(str.c_str());
			assert(element != NULL); // the element must be found

			// Restore the element's state
			element->Restore(savefile);

			// Add the element to the queue
			push_back(element);
		}
	}
};

// Convenience typedefs
class State;
typedef Queue<State> StateQueue;

class Task;
typedef Queue<Task> TaskQueue;

} // namespace ai

#endif /* !__STATEQUEUE_H__ */
