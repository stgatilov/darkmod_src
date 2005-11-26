
/******************************************************************************/
/*                                                                            */
/*         Dark Mod Sound Propagation (C) by Chris Sarantos in USA 2005		  */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
*
* DESCRIPTION: Sound propagation class for propagating suspicious sounds to AI
* during gameplay.  Friend class to CsndPropLoader.
*
*****************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 ******************************************************************************/

#ifndef SNDPROP_H
#define SNDPROP_H

#include "sndproploader.h"

/******************************************************************************
*
* DESCRIPTION: Sound propagation class for propagating suspicious sounds to AI
* during gameplay.  Friend class to CsndPropLoader.
*
*****************************************************************************/

template <class type> 
class CMatRUT;

/**
* Team bitmask definition, for comparing team alert flags
* with team status of an entity
**/

typedef struct STeamBits_s
{
	unsigned int friendly : 1;
	unsigned int neutral : 1;
	unsigned int enemy : 1;
	unsigned int same : 1;
} STeamBits;

typedef union UTeamMask_s
{
	unsigned int m_field;
	STeamBits m_bits;
} UTeamMask;

/**
* Array entry in populated areas array
**/
typedef struct SPopArea_s
{
	int				addedTime; // timestamp at which this entry was added from the AI list
	
	bool			bVisited; // area was visited at least once in wavefront expansion

	idList<idAI *>	AIContents; // list of AI that are present in area

	//TODO: Handle Listeners in another list here

	idList<int>		VisitedPorts; // portals that the sound flooded in on (reduces comp. time to store this)

} SPopArea;

/**
* Portal data stored in an event area
**/
typedef struct SPortEvent_s
{
	float	Loss; // dynamic array to store the current loss at the portal

	float	Dist; // distance at portal (used to add AI loss to existing loss)

	float	Att; // attenuation at portal (again used for final AI calculation)

	int		Floods; // How many floods did it take to get to that particular portal

	SsndPortal *ThisPort; // pointer to the snd portal object for this portal 

	SPortEvent_s *PrevPort; // the portal visited immediately before each portal

} SPortEvent;


/**
* Array entry in event areas array (storing visited areas information)
**/
typedef struct SEventArea_s
{
	bool		bVisited; // area was visited at least once in wavefront expansion

	SPortEvent	*PortalDat; // Array of event data for each portal in the area

} SEventArea;

/**
* Expansion queue entry for the wavefront expansion algorithm
**/
typedef struct SExpQue_s
{
	int			area; // area number

	int			portalH; // portal handle of the portal flooded in on

	float		curDist; // total distance travelled by wave so far

	float		curAtt; // total attenuation due to material losses so far

	float		curLoss; // total loss so far

	SPortEvent *PrevPort; // previous portal flooded through along path

} SExpQue;




class CsndProp : public CsndPropBase {

public:
	CsndProp( void );
	~CsndProp( void );

	void Clear( void );

	void Propagate 
		( float volMod, float durMod, idStr soundName,
		idVec3 origin, idEntity *maker, USprFlags *addFlags = NULL );

	/**
	* Get the appropriate vars from the sndPropLoader after
	* it has loaded data for the map.
	*
	* Also looks up door entity pointers for current map and 
	*	puts them into area/portal tree
	*
	* Also initializes various members
	**/
	void SetupFromLoader( const CsndPropLoader *in );

	/**
	* Check if a sound is defined in the soundprop def file
	**/
	bool CheckSound( const char *sndNameGlobal, bool isEnv );

	/**
	* Static var for AI checking the default threshold
	**/
	static float s_SPROP_DEFAULT_TRESHOLD;

protected:
	
	/**
	* Fill the door gentity ID hash index based on the
	* gentity numbers of doors.  Must be run AFTER entities spawn.
	**/
	void FillDoorEnts ( void );

	/**
	* Wavefront expansion algorithm, starts with volume volInit at point origin
	**/
	bool ExpandWave
		( float volInit, idVec3 origin, 
		  SSprParms *propParms );
	
	/**
	* Process the populated areas after a sound propagation event.
	**/
	void ProcessPopulated( float volInit, idVec3 origin, SSprParms *propParms );

	/**
	* Process individual AI.  Messages the individual AI, and will later calculate
	*	the effects of environmental sounds in the signal/noise response of the AI.
	*
	* Called by ProcessPopulated
	**/
	void ProcessAI( idAI* AI, idVec3 origin, SSprParms *propParms );

	/**
	* Copy parms from loader object, and also initialize several member vars
	**/
	void SetupParms( const idDict *parms, SSprParms *propParms,
					 USprFlags *addFlags, UTeamMask *tmask );

	/**
	* Obtain the loss in [dB] for a given door entity
	**/
	float GetDoorLoss( idEntity *doorEnt );

	/**
	* Detailed path minimization.  Finds the optimum path taking points along the portal surfaces
	* Writes the final loss info and apparent location of the sound to propParms.
	**/
	void DetailedMin( idAI* AI, SSprParms *propParms, 
					  SPortEvent *pPortEv, int AIArea, float volInit );

	/**
	* Takes point 1, point 2, a winding, and the center point of the winding
	* Returns the point on the winding surface that is closest
	*	to the line p1-p2.
	* 
	* If the line intersects the portal surface, the optimum point will
	*	be the intersection point.  Otherwise, the point will be somewhere
	*	along the outer boundary of the surface.
	*
	* Assumes a rectangular portal with 4 winding points.
	**/
	idVec3 OptSurfPoint( idVec3 p1, idVec3 p2, const idWinding *wind, idVec3 WCenter );

	/**
	* Draws debug lines between a list of points.  Used for soundprop debugging
	**/
	void DrawLines( idList<idVec3> *pointlist );


protected:

	int				m_TimeStamp; // time stamp for the current propagation event [ms]

	/**
	* Populated areas : List of indices of AI populated areas for this expansion
	**/
	idList<int>		m_PopAreasInd;

	/**
	* Populated areas array: Lists the AI present in each area
	*	and which portals the sound flowed in on, for later AI processing.
	* 
	* Stays in memory between events.  Each entry has a timestamp,
	*	and only entries whose indices are in the m_PopAreasInd list are
	*	checked when processing AI.
	**/
	SPopArea		*m_PopAreas;

	/**
	* (sparse) array of areas.  Areas that have been visited will have the 
	* current loss at each portal.  Size is the total number of areas, most
	* entries are NULL
	*
	* For now, this is cleared and re-written for every new sound event
	* later on, we might see if we can re-use it for multiple events that
	* come from close to the same spot, for optimization.
	**/
	SEventArea		*m_EventAreas;

};

#endif






	
