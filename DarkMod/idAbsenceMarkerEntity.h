/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef ABSENCEMARKERENTITY_H
#define ABSENCEMARKERENTITY_H

#pragma hdrstop

// Includes
#include "..\game\entity.h"


/**
* The purpose of this entity subclass is to act as a marker for other entities that have
* been moved or destroyed. An instance of this class can be spawned when the other entity
* is removed, destroyed, moved or re-oriented in a noticeable fashion.  If an instance
* of this class has a stim, it can signal entities to notice it, and then provide them
* with a copy of the missing entities spawn args so that a script will konw how to
* react to the absence.
*
* @author SophisticatedZombie
* @project The Dark Mode
* @copyright 2006 The Dark Mod team
*
*/
class idAbsenceMarkerEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idAbsenceMarkerEntity );

	int ownerTeam;

protected:

	// Defines the spawnargs etc.. for this entity's script type
	int referenced_entityDefNumber;

	// The name of the entity being referenced
	idStr referenced_entityName;

	// The spawn args of the entity being referenced
	idDict referenced_spawnArgs;


public:

	idAbsenceMarkerEntity(void);
	virtual ~idAbsenceMarkerEntity(void);

	/**
	* Call this method to set the information about the entity
	* which this references as "missing" from its normal location.
	*
	* @param absetEntity idEntityPtr indicating the entity who's
	*	absence we are marking.
	*/
	bool initAbsenceReference
	(
		const idEntityPtr<idEntity>& absentEntity,
		idVec3 absentPosition,
		idMat3 absentOrientation
	);


	/**
	* These script events handle getting the spawnargs of the referenced object
	*/
	void Event_GetNextReferencedKey( const char *prefix, const char *lastMatch );
	void Event_SetReferencedKey( const char *key, const char *value );
	void Event_GetReferencedKey( const char *key );
	void Event_GetReferencedIntKey( const char *key );
	void Event_GetReferencedFloatKey( const char *key );
	void Event_GetReferencedVectorKey( const char *key );
	void Event_GetReferencedEntityKey( const char *key );



	

};


// End of header wrapper
#endif
