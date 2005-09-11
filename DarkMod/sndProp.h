
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
	int				areaNum;
	
	bool			bVisited; // area was visited at least once in wavefront expansion

	idList<idAI *>	AIContents; // list of AI that are present in area

	//TODO: Handle Listeners in another list here

	idList<int>		VisitedPorts; // portals that the sound flooded in on (reduces comp. time to store this)
} SPopArea;

/**
* Array entry in event areas array (storing visited areas information)
**/
typedef struct SEventArea_s
{
	bool	bVisited; // area was visited at least once in wavefront expansion

// TODO: Consolidate ALL these separate arrays into an array of one structure!!

	float	*LossAtPortal; // dynamic array to store the current loss at the portal

	float	*DistAtPortal; // distance at portal (used to add AI loss to existing loss)

	float	*AttAtPortal; // attenuation at portal (again used for final AI calculation)

	int		*FloodsAtPortal; // How many floods did it take to get to that particular portal

	SsndPortal **PrevPortAtPort; // the portal visited immediately before each portal

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

	SsndPortal *PrevPort; // previous portal flooded through along path

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

	bool ExpandWave
		( float volInit, idVec3 origin, 
		  SSprParms *propParms );

	void ProcessPopulated( float volInit, idVec3 origin, SSprParms *propParms );

	void ProcessAI( idAI* AI, idVec3 origin, SSprParms *propParms );


	void SetupParms( const idDict *parms, SSprParms *propParms,
					 USprFlags *addFlags, UTeamMask *tmask );

	float GetDoorLoss( idEntity *doorEnt );

	/**
	* Linear search to find the index of the given area in the populated
	* areas array.  Returns -1 if the area is not present
	**/
	int FindPopIndex( int areaNum );

protected:

	/**
	* Populated areas : Areas that contain AI within the cutoff distance
	* these should eventually be propagated to
	**/
	idList<SPopArea>		m_PopAreas;

	/**
	* (sparse) array of areas.  Areas that have been visited will have the 
	* current loss at each portal.  Size is the total number of areas, most
	* entries are NULL
	*
	* For now, this is cleared and re-written for every new sound event
	* later on, we might see if we can re-use it for multiple events that
	* come from close to the same spot, for optimization.
	**/
	SEventArea				*m_EventAreas;

};

#endif






	
