
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

	void FillDoorIDHash ( const CsndPropLoader * in );

	/**
	* GetDoorEnt returns a pointer to the door entity for a given door ID
	**/
	idEntity *GetDoorEnt( int doorID );

	float p2pLoss( idVec3 point1, idVec3 point2, int area );

	float CsndProp::PropToPoint
		( float volInit, idVec3 origin, 
		  idVec3 target, SSprParms *propParms,
		  bool *bSameArea );

	void SetupParms( const idDict *parms, SSprParms *propParms,
					 USprFlags *addFlags, UTeamMask *tmask );

	float GetDoorLoss( SPropPath *path );

protected:

	/**
	* DoorIDHash references door IDs to game entity numbers
	**/
	idHashIndex			m_DoorIDHash;

};

#endif






	
