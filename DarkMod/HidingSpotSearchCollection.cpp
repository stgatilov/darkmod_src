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
	highestSearchId(0)
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
	// Clear the map now that the pointers are destroyed
	searches.clear();
}

void CHidingSpotSearchCollection::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(searches.size());
	for (HidingSpotSearchMap::const_iterator i = searches.begin(); i != searches.end(); i++)
	{
		HidingSpotSearchNode* node = i->second;

		savefile->WriteInt(node->searchId);
		savefile->WriteInt(node->refCount);
		node->search.Save(savefile);
	}

	savefile->WriteInt(highestSearchId);
}

void CHidingSpotSearchCollection::Restore(idRestoreGame *savefile)
{
	clear();

	int num;
	savefile->ReadInt(num);

	for (int i = 0; i < num; i++)
	{
		HidingSpotSearchNode* node = new HidingSpotSearchNode;

		savefile->ReadInt(node->searchId);
		savefile->ReadInt(node->refCount);
		node->search.Restore(savefile);

		// Insert the allocated search into the map and take the searchid as index
		searches.insert(
			HidingSpotSearchMap::value_type(node->searchId, node)
		);
	}

	savefile->ReadInt(highestSearchId);
}

//--------------------------------------------------------------------

int CHidingSpotSearchCollection::getNewSearch()
{
	if (searches.size() >= MAX_NUM_HIDING_SPOT_SEARCHES)
	{
		return NULL_HIDING_SPOT_SEARCH_HANDLE;
	}
	else
	{
		HidingSpotSearchNode* p_node = new HidingSpotSearchNode;
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

		return p_node->searchId; // ID is handle
	}
}


//**********************************************************************
// Public
//**********************************************************************

CDarkmodAASHidingSpotFinder* CHidingSpotSearchCollection::getSearchByHandle
(
	int searchHandle
)
{
	HidingSpotSearchMap::const_iterator found = searches.find(searchHandle);

	// Return NULL if not found
	return (found != searches.end()) ? &found->second->search : NULL;
}

//----------------------------------------------------------------------------------

CDarkmodAASHidingSpotFinder* CHidingSpotSearchCollection::getSearchAndReferenceCountByHandle
(
	int searchHandle,
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
}


//------------------------------------------------------------------------------

void CHidingSpotSearchCollection::dereference(int searchHandle)
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
		}
	}
}

//------------------------------------------------------------------------------

int CHidingSpotSearchCollection::findSearchByBounds 
(
	idBounds bounds,
	idBounds exclusionBounds
)
{
	for (HidingSpotSearchMap::iterator i = searches.begin(); i != searches.end(); i++)
	{
		HidingSpotSearchNode* node = i->second;

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
}

//------------------------------------------------------------------------------

int CHidingSpotSearchCollection::getOrCreateSearch
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
	int hSearch = findSearchByBounds(in_searchLimits, in_searchExclusionLimits);

	if (hSearch != NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		CDarkmodAASHidingSpotFinder* p_search = getSearchByHandle(hSearch);
		out_b_searchCompleted = p_search->isSearchCompleted();
		return hSearch;
	}

	// Make new search
	hSearch = getNewSearch();
	if (hSearch == NULL_HIDING_SPOT_SEARCH_HANDLE)
	{
		return hSearch;
	}

	// Initialize the search
	CDarkmodAASHidingSpotFinder* p_search = getSearchByHandle(hSearch); // TODO don't look the search up, we know that it exists
	p_search->initialize
	(
		hideFromPos, 
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
