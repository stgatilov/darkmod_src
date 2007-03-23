/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once

//class darkModLAS;

// DarkMod globals (needed for ../game/Light.h)
#include "../darkmod/darkmodglobals.h"

// idLight 
#include "../game/Light.h"

// The PVS to AAS mapping table
#include "PVSToAASMapping.h"


/*!
* This structure tracks a light in relation to the area system
*/
typedef struct darkModLightRecord_s
{
	/*!
	* The light object
	*/
	idLight* p_idLight;

    /*!
	* index of the Area the light is currently located within
	*/
    int areaIndex;
    
    /*!
	* Last world position of the light 
	* This is tracked so that we don't bother to recalcluate movement between PVS
	* areas if the position has not changed this frame.
	*/
    idVec3 lastWorldPos;

    /*!
	* A flag used to track if this light has been updated yet this frame
	*/
    unsigned int lastFrameUpdated;
        
} darkModLightRecord_t;

//---------------------------------------------------------------------------

class darkModLAS
{
protected:

    // Each area (PVS) in the map has its own light list
    int m_numAreas;
	
	// The area index is the index into the array
	// Dereferencing it gives a list of darkModLightRecord_t pointers
	(idLinkList<darkModLightRecord_t>)** m_pp_areaLightLists;

   /*!
   * Called to update the lights in a given bounding area.  
   */
   void updateLights (idBounds updateBounds, int updateFrameIndex);

   /*!
   * This is used to identify which lights have not been updated this frame
   * This value should be updated once per game frame, (or some fraction thereof if lighting gets
   * updated less often)
   */ 
   unsigned int m_updateFrameIndex;

   /*!
   * This method is used to move a light from one area to another
   * @return false if failed
   * @return true if succeeded
   */ 
   __inline bool moveLightBetweenAreas (darkModLightRecord_t* p_light, int oldAreaNum, int newAreaNum );


   /*!
   * This method is used to add up all the light intensities contributed from
   * a specific region apon the line between the two test points.
   *
   * @param areaIndex index of the area within the Area System who's lights should
   *     be accumulated
   * @param testPoint1 first point along the line who's lighting is being tested
   * @param testPoint2 second point along the line who's lighting is being tested
   * @param p_ignoreEntity An entity who's occlusion of a light should not be considered
   * @param b_useShadows if true, then shadow volumes are considered
   * @returns the total intensity on the point from lights in the test area
   */
   void accumulateEffectOfLightsInArea 
	( 
		float& inout_totalIllumination,
		int areaIndex, 
		idVec3 testPoint1,
		idVec3 testPoint2,
		idEntity* p_ignoreEntity,
		bool b_useShadows
	);




public:

   /*!
   * This mapping is used by certain queries
   * It is initialized in darkModLAS::initialize and cleared in darkModLAS::shutdown
   */
   PVSToAASMapping pvsToAASMappingTable;

	/*!
	* Constructor
	*/
	darkModLAS(void);

	/*!
	* Destructor
	*/
	virtual ~darkModLAS(void);

	/*!
	* Call this when the map is loaded to initialize the PVS tracking
	*
	* @param aasFileIndex: The aas file to use for mapping between PVS and AAS
	*  areas (useful for AI visibility and search routines)
	*/
	void initialize();

	/*!
	* Call this when the map is being shut down
	*/
	void shutDown();


   /*!
   * add a light on light spawn
   */
   void addLight (idLight* p_idLight);

   /*!
   * Remove a light on its destruction
   */
   void removeLight (idLight* p_idLight);

   /*!
   * updateLASState:
   * This does one frame of light awareness tracking updates. It updates every
   * light in the system and moves it from area to area as needed.
   */
   void updateLASState();

   /*!
   * This would  determine a light intensity rating by summing the intensities of the lights
   * able to shine on the line.
   *
   * @param areaIndex index of the area within the Area System who's lights should
   *     be accumulated
   * @param testPoint1 first point along the line who's lighting is being tested
   * @param testPoint2 second point along the line who's lighting is being tested
   * @param p_ignoreEntity An entity who's occlusion of a light should not be considered
   * @param b_useShadows if true, then shadow volumes are considered
   * @returns the total intensity on the point from lights in the test area
   */
   float queryLightingAlongLine
   (
		idVec3 testPoint1,
		idVec3 testPoint2,
		idEntity* p_ignoreEntity,
		bool b_useShadows
   );

   /*!
   * This method can be used to determine a list of lights affecting the 
   * given location. AI routines can use this to decide to turn lights on or off
   * to aid or hinder in searches.
   *
   * @param testPoint [in] The point that is to be tested.
   *
   * @param b_currentlyLighting [in] The optional flag determines if the light has to 
   * be currently raising the light level at the point, or just potentially influencing 
   * it if turned up in intensity.
   *
   */
   idList<qhandle_t> getListOfLightsAffectingPoint (const idVec3& testPoint, bool b_currentlyLighting = true );
  




};

// The Dark Mod Light Awareness System
extern darkModLAS LAS;
