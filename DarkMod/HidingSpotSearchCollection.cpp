/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "./HidingSpotSearchCollection.h"

//--------------------------------------------------------------------

// Constructor

CHidingSpotSearchCollection::CHidingSpotSearchCollection() :
	highestSearchId(0),
	numSearchesInUse(0),
	p_firstSearch(NULL)
{}

//--------------------------------------------------------------------

// Destructor

CHidingSpotSearchCollection::~CHidingSpotSearchCollection()
{
	clear();
}

//--------------------------------------------------------------------

CHidingSpotSearchCollection& CHidingSpotSearchCollection::Instance()
{
	static CHidingSpotSearchCollection _instance;
	return _instance;
}

//--------------------------------------------------------------------

void CHidingSpotSearchCollection::clear()
{
	// Destroy all searches
	for (HidingSpotSearchMap::iterator i = searches.begin(); i != searches.end(); i++)
	{
		delete i->second;
	}
	searches.clear();

	/*TDarkmodHidingSpotSearchNode* p_cursor = p_firstSearch;
	TDarkmodHidingSpotSearchNode* p_temp;
	while (p_cursor != NULL)
	{
		p_temp = p_cursor->p_next;
		delete p_cursor;
		p_cursor = p_temp;
	}*/

	p_firstSearch = NULL;

	// No active searches any more
	numSearchesInUse = 0;
}

void CHidingSpotSearchCollection::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(searches.size());
	for (HidingSpotSearchMap::const_iterator i = searches.begin(); i != searches.end(); i++)
	{
		TDarkmodHidingSpotSearchNode* node = i->second;

		savefile->WriteInt(node->searchId);
		savefile->WriteInt(node->refCount);
		node->search.Save(savefile);
	}

	savefile->WriteInt(highestSearchId);
	savefile->WriteUnsignedInt(numSearchesInUse);

	/*int searchesSaved = 0;
	TDarkmodHidingSpotSearchNode* p_cursor = p_firstSearch;
	while (p_cursor != NULL)
	{
		savefile->WriteInt(p_cursor->searchId);
		savefile->WriteInt(p_cursor->refCount);
		
		p_cursor->search.Save(savefile);

		searchesSaved++;
		p_cursor = p_cursor->p_next;
	}

	if (searchesSaved != numSearchesInUse)
	{
		DM_LOG(LC_AI, LT_ERROR).LogString("Error while saving collection: searchesSaved != numSearchesInUse\r");
	}*/
}

void CHidingSpotSearchCollection::Restore(idRestoreGame *savefile)
{
	clear();

	int num;
	savefile->ReadInt(num);

	for (int i = 0; i < num; i++)
	{
		TDarkmodHidingSpotSearchNode* node = new TDarkmodHidingSpotSearchNode;

		savefile->ReadInt(node->searchId);
		savefile->ReadInt(node->refCount);
		node->search.Restore(savefile);

		// Insert the allocated search into the map and take the searchid as index
		searches.insert(
			HidingSpotSearchMap::value_type(node->searchId, node)
		);
	}

	savefile->ReadInt(highestSearchId);
	savefile->ReadUnsignedInt(numSearchesInUse);

	p_firstSearch = NULL;

	/*TDarkmodHidingSpotSearchNode* lastSearch = NULL;
	for (unsigned int i = 0; i < numSearchesInUse; i++)
	{
		TDarkmodHidingSpotSearchNode* curSearch = new TDarkmodHidingSpotSearchNode;

		if (p_firstSearch == NULL)
		{
			p_firstSearch = curSearch;
		}

		curSearch->p_prev = lastSearch;
		curSearch->p_next = NULL;

		if (curSearch->p_prev != NULL)
		{
			curSearch->p_prev->p_next = curSearch;
		}

		savefile->ReadInt(curSearch->searchId);
		savefile->ReadInt(curSearch->refCount);

		curSearch->search.Restore(savefile);

		lastSearch = curSearch;
	}*/
}

//--------------------------------------------------------------------

THidingSpotSearchHandle CHidingSpotSearchCollection::getUnusedSearch()
{
	if (numSearchesInUse >= MAX_NUM_HIDING_SPOT_SEARCHES)
	{
		return NULL_HIDING_SPOT_SEARCH_HANDLE;
	}
	else
	{
		TDarkmodHidingSpotSearchNode* p_node = new TDarkmodHidingSpotSearchNode;
		if (p_node == NULL)
		{
			return NULL_HIDING_SPOT_SEARCH_HANDLE;
		}

		// We are returning to somebody, so they have a reference
		p_node->refCount = 1;

		// greebo: Assign a unique ID to this searchnode
		p_node->searchId = highestSearchId;

		searches.insert(
			HidingSpotSearchMap::value_type(p_node->searchId, p_node)
		);

		// Increase the unique ID
		highestSearchId++;

		/*p_node->p_prev = NULL;
		p_node->p_next = p_firstSearch;
		if (p_firstSearch != NULL)
		{
			p_firstSearch->p_prev = p_node;
		}
		p_firstSearch = p_node;*/

		// One more search in use
		numSearchesInUse++;

		return p_node->searchId; // ID is handle
	}
}


//**********************************************************************
// Public
//**********************************************************************

int CHidingSpotSearchCollection::getSearchId(THidingSpotSearchHandle searchHandle)
{
	HidingSpotSearchMap::const_iterator found = searches.find(searchHandle);

	// Return NULL handle if not found
	return (found != searches.end()) ? searchHandle : NULL_HIDING_SPOT_SEARCH_HANDLE;

	/*TDarkmodHidingSpotSearchNode* p_node = p_firstSearch;
	while (p_node != NULL)
	{
		if (static_cast<THidingSpotSearchHandle>(p_node) == searchHandle)
		{
			return p_node->searchId;
		}
		p_node = p_node->p_next;
	}

	// None found
	return -1;*/
}

THidingSpotSearchHandle CHidingSpotSearchCollection::getSearchHandle(int searchId)
{
	return searchId; // ID == handle

	/*TDarkmodHidingSpotSearchNode* p_node = p_firstSearch;
	while (p_node != NULL)
	{
		if (p_node->searchId == searchId)
		{
			return searchId; // ID == handle, TODO: remove this deprecated function, when done
		}
		p_node = p_node->p_next;
	}

	// None found
	return NULL_HIDING_SPOT_SEARCH_HANDLE;*/
}

CDarkmodAASHidingSpotFinder* CHidingSpotSearchCollection::getSearchByHandle
(
	THidingSpotSearchHandle searchHandle
)
{
	HidingSpotSearchMap::const_iterator found = searches.find(searchHandle);

	// Return NULL if not found
	return (found != searches.end()) ? &found->second->search : NULL;
}

//----------------------------------------------------------------------------------

CDarkmodAASHidingSpotFinder* CHidingSpotSearchCollection::getSearchAndReferenceCountByHandle
(
	THidingSpotSearchHandle searchHandle,
	unsigned int& out_refCount
)
{
	HidingSpotSearchMap::const_iterator found = searches.find(searchHandle);

	if (found != searches.end())
	{
		out_refCount = found->second->refCount;
		return &found->second->search;
	}
	else
	{
		// not found
		out_refCount = 0;
		return NULL;
	}

	/*TDarkmodHidingSpotSearchNode* p_node = (TDarkmodHidingSpotSearchNode*) searchHandle;
	if (p_node != NULL)
	{
		if (p_node->refCount <= 0)
		{
			out_refCount = 0;
			return NULL;
		}

		out_refCount = p_node->refCount;
		return &(p_node->search);
	}
	else
	{
		return NULL;
	}*/
}


//------------------------------------------------------------------------------

void CHidingSpotSearchCollection::dereference(THidingSpotSearchHandle searchHandle)
{
	HidingSpotSearchMap::iterator found = searches.find(searchHandle);

	if (found != searches.end())
	{
		found->second->refCount--;

		if (found->second->refCount <= 0)
		{
			// Delete and remove from map 
			delete found->second;
			searches.erase(found);
		
			// One less search
			numSearchesInUse--;
		}
	}

	/*TDarkmodHidingSpotSearchNode* p_node = 
		reinterpret_cast<TDarkmodHidingSpotSearchNode*>(searchHandle);

	if (p_node != NULL)
	{
		// Decrement Refcount
		p_node->refCount--;

		if (p_node->refCount <= 0)
		{
			if (p_node->p_prev != NULL)
			{
				p_node->p_prev->p_next = p_node->p_next;
			}
			else
			{
				p_firstSearch = p_node->p_next;
			}

			if (p_node->p_next != NULL)
			{
				p_node->p_next->p_prev = p_node->p_prev;
			}

			delete p_node;
		
			// One less search
			numSearchesInUse --;
		}
	}*/
	
}

//------------------------------------------------------------------------------

THidingSpotSearchHandle CHidingSpotSearchCollection::findSearchByBounds 
(
	idBounds bounds,
	idBounds exclusionBounds
)
{
	for (HidingSpotSearchMap::iterator i = searches.begin(); i != searches.end(); i++)
	{
		TDarkmodHidingSpotSearchNode* node = i->second;

		idBounds existingBounds = node->search.getSearchLimits();
		idBounds existingExclusionBounds = node->search.getSearchExclusionLimits();

		if (existingBounds.Compare(bounds, 50.0))
		{
			if (existingExclusionBounds.Compare(exclusionBounds, 50.0))
			{
				// Reuse this one and return the ID
				node->refCount++;
				return i->first;
			}
		}
	}

	// None found
	return NULL_HIDING_SPOT_SEARCH_HANDLE;

	/*TDarkmodHidingSpotSearchNode* p_node = p_firstSearch;
	while (p_node != NULL)
	{
		idBounds existingBounds = p_node->search.getSearchLimits();
		idBounds existingExclusionBounds = p_node->search.getSearchExclusionLimits();
		if (existingBounds.Compare (bounds, 50.0))
		{
			if (existingExclusionBounds.Compare (exclusionBounds, 50.0))
			{
				// Reuse this one
				p_node->refCount ++;
				return (THidingSpotSearchHandle) p_node;
			}
		}

		p_node = p_node->p_next;
	}

	// None found
	return NULL_HIDING_SPOT_SEARCH_HANDLE;*/

}

//------------------------------------------------------------------------------

THidingSpotSearchHandle CHidingSpotSearchCollection::getOrCreateSearch
(
	const idVec3 &hideFromPos, 
	idAAS* in_p_aas, 
	float in_hidingHeight,
	idBounds in_searchLimits, 
	idBounds in_searchExclusionLimits, 
	int in_hidingSpotTypesAllowed, 
	idEntity* in_p_ignoreEntity,
	int frameIndex,
	bool& out_b_searchCompleted
)
{
	// Search with same bounds already?
	THidingSpotSearchHandle hSearch = findSearchByBounds(in_searchLimits, in_searchExclusionLimits);

	// greebo: TODO simplify this algorithm, we're inside the collection class, so we might as well
	// access the members directly instead of using so many lookups.

	if (hSearch != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		CDarkmodAASHidingSpotFinder* p_search = getSearchByHandle(hSearch);
		out_b_searchCompleted = p_search->isSearchCompleted();
		return hSearch;
	}

	// Make new search
	hSearch = getUnusedSearch();
	if (hSearch == NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		return hSearch;
	}

	// Initialize the search
	CDarkmodAASHidingSpotFinder* p_search = getSearchByHandle(hSearch);
	p_search->initialize
	(
		hideFromPos, 
		//in_p_aas, 
		in_hidingHeight,
		in_searchLimits, 
		in_searchExclusionLimits,
		in_hidingSpotTypesAllowed, 
		in_p_ignoreEntity
	);

	// Start search
	DM_LOG(LC_AI, LT_DEBUG).LogString ("Starting search for hiding spots\r");
	bool b_moreProcessingToDo = p_search->startHidingSpotSearch
	(
		p_search->hidingSpotList,
		g_Global.m_maxNumHidingSpotPointTestsPerAIFrame,
		frameIndex
	);
	DM_LOG(LC_AI, LT_DEBUG).LogString ("First pass of hiding spot search found %d spots\r", p_search->hidingSpotList.getNumSpots());

	// Is search completed?
	out_b_searchCompleted = !b_moreProcessingToDo;

	// Search created, return search to caller
	return hSearch;
}
