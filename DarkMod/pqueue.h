/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 864 $
 * $Date: 2007-03-22 19:04:56 +0100 (Do, 22 Mrz 2007) $
 * $Author: sparhawk $
 *
 ***************************************************************************/

#ifndef __PQUEUE_H__
#define __PQUEUE_H__

// Priority queue containing strings
// This isn't implemented as a template because it would need to call different
// methods on idSaveGame/idRestoreGame for different types.

#include <algorithm>
#include <vector>
#include <string>
#include <sstream>

#include "DarkModGlobals.h"

// The highest possible priority value
#define PQUEUE_HIGHEST_PRIORITY ((1 << sizeof(int)) - 1)

class CPriorityQueue
{
protected:
	// The internal queue structure is an unsorted list of int => string pairs
	typedef std::vector<std::pair<int, std::string*> > Queue;
	Queue* data;

public:
	CPriorityQueue()
	{
		data = new Queue;
	}

	~CPriorityQueue()
	{
		for (unsigned int i=0; i < data->size(); i++)
		{
			delete (*data)[i].second; // Delete std::string objects
		}
		delete data; // Delete std::vector
	}

	/**
	* Add a string to the heap, with associated priority
	*/
	void Push(int priority, const char *str)
	{
		data->push_back(std::make_pair(priority, new std::string(str)));
		push_heap(data->begin(), data->end());
	}

	/**
	* Return the element with the highest priority, and remove it from the priority queue.
	* The pointer will remain valid until this CPriorityQueue instance is destroyed.
	* Returns an empty string if the queue is empty.
	*/
	const char* Pop()
	{
		if (!data->size()) return "";
		pop_heap(data->begin(), data->end());
		const char* str = data->begin()->second->c_str();
		data->pop_back();
		return str;
	}

	/**
	 * greebo: Removes the given <element> from the queue.
	 *
	 * @returns: TRUE if the element was found and removed, FALSE otherwise
	 */
	bool Remove(const std::string& element)
	{
		for (Queue::iterator i = data->begin(); i != data->end(); i++)
		{
			// Compare the visited string to the given <element>
			if (*i->second == element)
			{
				data->erase(i);
				return true;
			}
		}

		return false;
	}

	/**
	* Return the element with the highest priority.
	* The pointer will remain valid until this CPriorityQueue instance is destroyed.
	* Returns an empty string if the queue is empty.
	*/
	const char* Peek() const
	{
		if (!data->size()) return "";
		else return data->begin()->second->c_str();
	}

	/**
	* Return the priority of the element with the highest priority.
	* Returns 0 if the queue is empty.
	*/
	int PeekPriority() const
	{
		if (!data->size()) return 0;
		else return data->begin()->first;
	}

	/**
	* Returns the entire contents of the priority queue as a string, for debugging purposes.
	*/
	const std::string DebuggingInfo() const
	{
		std::stringstream debugInfo;
		for (Queue::const_iterator i = data->begin(); i != data->end(); i++) {
			debugInfo << (*i).first;
			debugInfo << "   ";
			debugInfo << (*i).second->c_str();
			debugInfo << "\n";
		}
		return debugInfo.str();
	}
	
	/**
	* Save this data structure to a savefile
	*/
	void Save( idSaveGame *savefile ) const
	{
		savefile->WriteInt(data->size());
		for (Queue::const_iterator i = data->begin(); i != data->end(); i++)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Saving task %s with priority %d.\r", i->second->c_str(), i->first);
			savefile->WriteInt(i->first);
			savefile->WriteString(i->second->c_str());
		}
	}

	/**
	* Restore this data structure from a savefile
	*/
	void Restore( idRestoreGame *savefile )
	{
		data->clear();
		
		int elements;
		savefile->ReadInt(elements);
		for (int i=0; i<elements; i++)
		{
			int priority;
			idStr str;
			savefile->ReadInt(priority);
			savefile->ReadString(str);
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Restoring task %s with priority %d.\r", str.c_str(), priority);
			data->push_back(std::make_pair(priority, new std::string(str.c_str())));
		}
		
		// Make sure it's a heap
		make_heap(data->begin(), data->end());
	}
};

#endif /* !__PQUEUE_H__ */
