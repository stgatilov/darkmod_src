/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
/******************************************************************************/
/*                                                                            */
/*         Dark Mod AI Relationships (C) by Chris Sarantos in USA 2005		  */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
*
* DESCRIPTION: CRelations is a "relationship manager" that keeps track of and
* allows changes to the relationship between each team, including every
* AI team and the player team (team 0).
* Relationship values are integers.  negative is enemy, positive friend, zero
* neutral.  
*
* The larger the value, the more strongly an AI "cares" about the relationship.
* Large - value => will not rest until they've hunted down every last member
* of this team.  
* Large + value => these teams had best not forget their anniversary.
*
* Accessor methods should be made available to scripting, for relationship dependent
* scripts, and for changing the relationships on a map in realtime.
*
* TODO: Allow for "persistent relationships" that copy over between maps.
* This would be useful for T3-esque factions.  It might require a "faction ID"
* that is different from the "team ID" in the mapfile though.
*
*****************************************************************************/

#ifndef RELATIONS_H
#define RELATIONS_H

#include "../idlib/precompiled.h"
#include "DarkModGlobals.h"

template <class type> 
class CMatrixSq;

extern CRelations g_globalRelations;

class CRelations : public idClass {

CLASS_PROTOTYPE( CRelations );

public:

	typedef enum ERel_Type
	{
		E_ENEMY = -1,
		E_NEUTRAL = 0,
		E_FRIEND = 1
	} ERel_Type;

	typedef struct SEntryData_s
	{
		int row;
		int col;
		int val;
	} SEntryData;

public:

	CRelations(void);
	~CRelations(void);

	CRelations &operator=( const CRelations &in );

	void Clear(void);

	bool IsCleared(void);

/**
* Returns the dimension of the square relationship matrix.
* For example, if the matrix is 3x3, it returns 3.
**/
	int Size(void);

/**
* Return the integer number for the relationship between team i and team j
**/
	int GetRelNum(int i, int j);

/**
* Return the type of relationship (E_ENEMY, E_NEUTRAL or E_FRIEND)
* for the relationship between team i and team j
**/
	int GetRelType(int i, int j);

/**
* Set the integer value of the relationship between team i and team j to rel
**/
	void SetRel(int i, int j, int rel);

/**
* Add the integer 'offset' to the relationship between team i and team j
* (You can add a negative offset to subtract)
**/
	void ChangeRel( int i, int j, int offset);

/**
* Returns true if team i and team j are friends
**/
	bool IsFriend( int i, int j);

/**
* Returns true if team i and team j are enemies
**/
	bool IsEnemy( int i, int j);

/**
* Returns true if team i and team j are neutral
**/
	bool IsNeutral( int i, int j);

/**
* Fill the relationship matrix from the def file for the map
* returns FALSE if there was a problem loading
**/
	bool SetFromArgs( idDict *args );

/**
* Save the current relationship matrix to a savefile
* To be consistent w/ D3, this should be called in
* in idGameLocal::SaveGame , where everything else is saved.
* This is most likely necessary to save in the correct place.
**/
	void Save( idSaveGame *save ) const;

/**
* Load the current relationship matrix from a savefile
**/
	void Restore( idRestoreGame *save );

/**
* Copies itself to the static var for storing the global
* relations manager.
**/
	void CopyThisToGlobal( void );

/**
* Output the matrix to the console and logfile for debug purposes
**/
	void DebugPrintMat( void );

protected:

/**
* The relationship matrix uses class CMatrixSq to store a square matrix
**/
	CMatrixSq<int> *	m_RelMat;

/**
* Boolean to store whether the relations matrix failed to load
* Accessors will check this and return the default relation if true
**/
	bool				m_bMatFailed;

};


#endif
