/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once
/**
* The purpose of this class is to contain all the hiding spot searches in use by
* the AIs in the game.  Hiding spot searches can be shared, so this holds all
* the searches and controls whether or not new searches are required.  
*
* An AI that would like a search done makes a call to the  singleton instance
* of this class and asks for a search for a given stimulus. If it is simlar
* enough to an existing search that search is re-used.
*
*/

// Required includes
#include "DarkmodAASHidingSpotFinder.h"
#include <map>

//---------------------------------------------------------------------------

struct TDarkmodHidingSpotSearchNode
{
	// greebo: The id of this search, to resolve pointers after map restore
	int searchId;

	int refCount;
	CDarkmodAASHidingSpotFinder search;
};

//---------------------------------------------------------------------------

/**
* This is the handle type by which an AI references searches in which it is 
* interested.
*/
typedef int THidingSpotSearchHandle;

#define NULL_HIDING_SPOT_SEARCH_HANDLE -1
#define MAX_NUM_HIDING_SPOT_SEARCHES 4

//---------------------------------------------------------------------------

class CHidingSpotSearchCollection
{
private:
	int highestSearchId;

	/**
	* Constructor (private, this is a singleton class)
	*/
	CHidingSpotSearchCollection();

public:
	/**
	* Destructor
	*/
	~CHidingSpotSearchCollection();

	/**
	 * greebo: Contains the singleton instance of this class.
	 */
	static CHidingSpotSearchCollection& Instance();

	/**
	* This gets a search by its handle
	*/
	CDarkmodAASHidingSpotFinder* getSearchByHandle
	(
		THidingSpotSearchHandle searchHandle
	);

	/**
	* This gets a search by its handle and indicates how many people
	* (including the caller) have a reference handle to the search.
	*/
	CDarkmodAASHidingSpotFinder* getSearchAndReferenceCountByHandle
	(
		THidingSpotSearchHandle searchHandle,
		unsigned int& out_refCount
	);

	/**
	* This should be called to dereference a hiding spot search. It ensures
	* the search is destroyed when the last user deallocates it.
	*
	* Once this is called, the handle should be considered invalid and never
	* be used again.
	*/
	void dereference(THidingSpotSearchHandle hSearch);

	/**
	* This attempts to get or create a new search. If the search
	* already exists, the existing one is returned.
	*
	* The search is referenced if the handle returned is not the null
	* value, so the caller must eventually dereference it.
	*/
	THidingSpotSearchHandle getOrCreateSearch
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
	);
		
	// Standard Save/Restore routines 
	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);
	
protected:

	// greebo: The array holding all active hiding spot search pointers
	typedef std::map<int, TDarkmodHidingSpotSearchNode*> HidingSpotSearchMap;
	HidingSpotSearchMap searches;

	/**
	* This destroys all searches. Don't call it unless you are shutting down the game.
	*/
	void clear();

	/**
	* This gets an empty hiding spot search from the list and references
	* it before returning it to the caller.
	*
	*/
	THidingSpotSearchHandle getUnusedSearch();

	/**
	* This searches the list for a search with similar bounds.
	*/
	THidingSpotSearchHandle findSearchByBounds(idBounds bounds, idBounds exclusionBounds);
};
