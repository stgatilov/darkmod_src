#include "../idlib/precompiled.h"
#pragma hdrstop

#include "./pvstoaasmapping.h"
#include "../darkMod/darkmodGlobals.h"
#include "../game/PVS.h"
#include "../renderer/renderworld.h"
#include "../darkmod/intersection.h"

//----------------------------------------------------------------------------

PVSToAASMapping::PVSToAASMapping(void)
{
	// No allocated mapping
	aasFileIndex = -1;
	numPVSAreas = 0;
	m_p_AASAreaIndicesPerPVSArea = NULL;
}

//----------------------------------------------------------------------------

PVSToAASMapping::~PVSToAASMapping(void)
{
	clear();
}


//----------------------------------------------------------------------------

void PVSToAASMapping::clearMappingList (PVSToAASMappingNode* p_header)
{
	if (p_header == NULL)
	{
		return;
	}

	// Ride the list and delete each node
	PVSToAASMappingNode* p_cursor = p_header;
	PVSToAASMappingNode* p_temp = NULL;
	while (p_cursor != NULL)
	{
		p_temp = p_cursor->p_next;
		delete p_cursor;
		p_cursor = p_temp;
	}
	
}

//----------------------------------------------------------------------------

void PVSToAASMapping::clear()
{
	if (m_p_AASAreaIndicesPerPVSArea != NULL)
	{
		// Clear each mapping list
		for (int i = 0; i < numPVSAreas; i ++)
		{
			clearMappingList (m_p_AASAreaIndicesPerPVSArea[i]);
		}

		// Done with mapping list header pointers
		delete[] m_p_AASAreaIndicesPerPVSArea;

		// Mapping list header pointers unallocated
		m_p_AASAreaIndicesPerPVSArea = NULL;
	}

	numPVSAreas = 0;
	aasFileIndex = -1;
}

//----------------------------------------------------------------------------

bool PVSToAASMapping::buildMappings(int aasNumber)
{
	// If we already map this one, we are done
	if (aasNumber == aasFileIndex)
	{
		return true;
	}

	// Clear any previous mapping
	clear();

	// Get the aas 
	idAAS* p_aas = gameLocal.GetAAS (aasNumber);
	if (p_aas == NULL)
	{
		DM_LOG (LC_AI, LT_ERROR).LogString ("No aas files exist for this map, AI will not be able to locate darkness...\n");
		return false;
	}

	// Get number of PVS areas
	numPVSAreas = gameRenderWorld->NumAreas();
	if (numPVSAreas > 0)
	{
		// Allocate areas
		m_p_AASAreaIndicesPerPVSArea = new PVSToAASMappingNode*[numPVSAreas];
		if (m_p_AASAreaIndicesPerPVSArea == NULL)
		{
			numPVSAreas = 0;
			DM_LOG (LC_AI, LT_ERROR).LogString ("Failed to alloate mapping table header pointers\n");
			return false;
		}
	}

	// All start empty
	for (int i = 0; i < numPVSAreas; i ++)
	{
		m_p_AASAreaIndicesPerPVSArea[i] = NULL;
	}

	// Iterate AAS areas and add each one to the appropriate PVS area
	bool b_worked = true;
	int numAASAreas = p_aas->GetNumAreas();
	for (int aasAreaIndex = 0; aasAreaIndex  < numAASAreas; aasAreaIndex ++)
	{
		// Get AAS area center
		idVec3 aasAreaCenter = p_aas->AreaCenter (aasAreaIndex);

		// What PVS area does it go in?
		int pvsAreaIndex = gameLocal.pvs.GetPVSArea (aasAreaCenter);
		if (!insertAASAreaIntoPVSAreaMapping (aasAreaIndex, pvsAreaIndex))
		{
			clear();
			return false;
		}
	}

	// Remember file
	aasFileIndex = aasNumber;

	// Log success
	DM_LOG(LC_AI, LT_DEBUG).LogString 
	(
		"Successfully set up mapping of %d PVS areas to %d AAS areas\n", 
		numPVSAreas,
		numAASAreas
	);

	// Done
	return true;
}

//---------------------------------------------------------------------------

bool PVSToAASMapping::insertAASAreaIntoPVSAreaMapping (int aasAreaIndex, int pvsAreaIndex)
{

	if (pvsAreaIndex >= numPVSAreas)
	{
		// Log error
		DM_LOG(LC_AI, LT_ERROR).LogString 
		(
			"AAS area %d falls in PVS area %d which is beyond supposed PVS area count of %d\n", 
			aasAreaIndex, 
			pvsAreaIndex,
			numPVSAreas
		);
		return false;
	}
	else if (pvsAreaIndex < 0)
	{
		DM_LOG(LC_AI, LT_WARNING).LogString 
		(
			"AAS area %d falls in no PVS area, left out of mapping\n", 
			aasAreaIndex
		);
	}
	else 
	{
		// Allocate node
		PVSToAASMappingNode* p_node = new PVSToAASMappingNode;
		if (p_node == NULL)
		{
			DM_LOG(LC_AI, LT_ERROR).LogString 
			(
				"Failed to allocate mapping node\n"
			);
			return false;
		}
		else
		{
			p_node->AASAreaIndex = aasAreaIndex;
		}

		// Add to front
		if (m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex] == NULL)
		{
			p_node->p_next = NULL;
			m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex] = p_node;
		}
		else
		{
			p_node->p_next = m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex];
			m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex] = p_node;
		}

	}

	// Success
	return true;

}

//----------------------------------------------------------------------------

PVSToAASMappingNode* PVSToAASMapping::getAASAreasForPVSArea (int pvsAreaIndex)
{
	if ((pvsAreaIndex < 0) || (pvsAreaIndex >= numPVSAreas))
	{
		return NULL;
	}
	else
	{
		return m_p_AASAreaIndicesPerPVSArea[pvsAreaIndex];
	}
}

//----------------------------------------------------------------------------

void PVSToAASMapping::getAASAreasForPVSArea(int pvsAreaIndex, idList<int>& out_aasAreaIndices)
{
	out_aasAreaIndices.Clear();

	PVSToAASMappingNode* p_node = getAASAreasForPVSArea (pvsAreaIndex);

	while (p_node != NULL)
	{
		out_aasAreaIndices.AddUnique (p_node->AASAreaIndex);
		p_node = p_node->p_next;
	}

	// Done
}

