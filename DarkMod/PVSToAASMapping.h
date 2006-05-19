/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.2  2006/05/19 19:56:50  sparhawk
 * CVSHeader added
 *
 *
 ***************************************************************************/
#pragma once

// DarkMod globals (needed for ../game/Light.h)
#include "../game/Light.h"

//#include "../darkmod/darkmodglobals.h"

//------------------------------------------------------

typedef struct tagPVSToAASMappingNode
{
	tagPVSToAASMappingNode* p_next;

	int AASAreaIndex;

} PVSToAASMappingNode;

//------------------------------------------------------

class PVSToAASMapping
{
protected:

	// The map of PVS areas to AAS areas (one to many)
	// This is an array of linked lists, the offset into the array is
	// the PVS area number.  The list contains the indices of all the AAS
	// areas in that PVS area.
	int numPVSAreas;
	PVSToAASMappingNode** m_p_AASAreaIndicesPerPVSArea;

	// Which aas file are we currently mapping
	int aasFileIndex;

	/*!
	* This method clears one mapping list
	*/
	void clearMappingList (PVSToAASMappingNode* p_header);

	/*!
	* Inserts an aas area index into the list for a pvs area
	* @return true on success
	* @return false on failure
	*/
	bool insertAASAreaIntoPVSAreaMapping (int aasAreaIndex, int pvsAreaIndex);

public:
	PVSToAASMapping(void);
	virtual ~PVSToAASMapping(void);

	/*!
	* This method clears the currently held mapping
	*/
	void clear();

	/*!
	* This method builds a mapping which allows the AAS areas to be identified
	* for a given PVS area. Building this mapping may take some time, so it should
	* be kept around for re-use.
	*
	* @param aasNumber: The index of the aas file of the map. A typical map has at
	* least 1, with index 0.
	*
	* @return true on success
	* @return false on failure
	*/
	bool buildMappings(int aasNumber);

	/*!
	* This method gets the aas area index list for a particular pvs area
	*
	* @param pvsAreaIndex The index of the PVS area being querried
	* 
	* @return Pointer to the first node in the list of aas area indices for this pvs area
	* @return NULL If the requested mapping is empty or the pvs area requested is out of bounds

	*/
	PVSToAASMappingNode* getAASAreasForPVSArea (int pvsAreaIndex);

   /*!
   * Given a PVS area index, this retrieves a list of AAS area indices of AAS areas that it
   * contains. (More Doom3 familiar style container, but less efficient)
   */
   void getAASAreasForPVSArea(int pvsAreaIndex, idList<int>& out_aasAreaIndices);


};
