/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.11  2006/07/30 23:39:42  ishtvan
 * new objective script event setObjectiveEnabling
 *
 * Revision 1.10  2006/07/28 01:38:36  ishtvan
 * info_location objective
 *
 * Revision 1.9  2006/07/22 21:09:14  ishtvan
 * comp_info_location preliminary checkin
 *
 * Revision 1.8  2006/07/19 21:51:03  ishtvan
 * added irreversible behavior, modified some internal functions
 *
 * Revision 1.7  2006/07/19 09:10:09  ishtvan
 * bugfixes
 *
 * Revision 1.6  2006/07/19 05:19:49  ishtvan
 * added enabling objectives and scripts to call when objective completes
 *
 * Revision 1.5  2006/07/17 02:42:25  ishtvan
 * fixes to comp_custom_clocked and comp_distance
 *
 * Revision 1.4  2006/07/17 01:45:59  ishtvan
 * updates: custom objectives, distance objectives, custom clocked objectives
 *
 * Revision 1.3  2006/06/07 09:03:28  ishtvan
 * location component updates
 *
 * Revision 1.2  2006/05/30 06:22:04  ishtvan
 * added parsing of objectives from entity
 *
 * Revision 1.1  2006/05/26 10:24:39  ishtvan
 * Initial release
 *
 *
 *
 ***************************************************************************/

// Copyright (C) 2006 Chris Sarantos <csarantos@gmail.com>
//

// TODO: Add support for irreversible objectives/components

// TODO: Item in inventory?  Could put the inventory name in the second entdata parms
// that would allow automatic handling of objectives like put so many things in a chest,
// or give some AI so many things.  Maybe this isn't needed tho, maybe it could just be
// done with scripting in those particular cases.
// Basically we'd just have to make COMP_ITEM take a second ObjEntParms arg and make it
// match the name of the inventory to that as well as the rest.

// TODO: Right now, AI_FindBody stats does not track the stat of team of body found
// AND team of AI finding body.  It only tracks team of AI finding body.
// Whether an AI reacts to a body should probably be determined by the AI, for example,
// average guards wouldn't react to a dead rat, but an elite might.
// If that's the case, we don't have to track both variables in the stats, since
// AI will only call FindBody if they react to that type of body.

// TODO: Make difficulty a bitfield instead of a minimum integer value

#ifndef MISSIONDATA_H
#define MISSIONDATA_H

#include "../idlib/precompiled.h"
#include "DarkModGlobals.h"

// Maximum array sizes:
#define MAX_TEAMS 64
#define MAX_TYPES 16
#define MAX_AICOMP 16
#define MAX_ALERTNUMS 16

/**
* Objective completion states
**/
typedef enum
{
	STATE_INCOMPLETE,
	STATE_COMPLETE,
	STATE_INVALID, 
	STATE_FAILED
} EObjCompletionState;
// NOTE: STATE_INVALID may also be used for initially deactivating objectives, 
// then activating later by setting STATE_INCOMPLETE

/**
* Objective component specification types
**/
typedef enum
{
// The following apply to both AIs and items
	SPEC_NONE,
	SPEC_NAME,
	SPEC_OVERALL,
	SPEC_GROUP, // for inventory items, info_location groups, etc
	SPEC_CLASSNAME, // soft/scripting classname
	SPEC_SPAWNCLASS, // hard / SDK classname

// Specifically for AI:
	SPEC_AI_TYPE,
	SPEC_AI_TEAM,
	SPEC_AI_INNOCENCE
} ESpecificationMethod;

/**
* Objective component action types
* TODO: Move to game_local.h so that it can be used in external calls
**/
typedef enum
{
// AI components - MUST BE KEPT TOGETHER IN THE ENUM because later these enums are used as an array index
// COMP_KILL must be kept as the first one
	COMP_KILL, // also includes non-living things being destroyed
	COMP_KO,
	COMP_AI_FIND_ITEM,
	COMP_AI_FIND_BODY,
// END AI components that must be kept together
	COMP_ALERT,
	COMP_ITEM, // Add inventory item or imaginary loot
	COMP_LOCATION, // Item X is at location Y
	COMP_CUSTOM_ASYNC, // asynchronously updated custom objective (updated by mapper from script)

// The following are special clocked components, updated in CMissionData::UpdateObjectives
	COMP_CUSTOM_CLOCKED,
	COMP_INFO_LOCATION, // like location, but uses existing info_location areas instead of a brush
	COMP_DISTANCE // distance from origin of ent X to that of ent Y

} EComponentType;

// TODO: Two overloads of the CheckObjectie function, one passing in the ent pointer
// and one passing in a filled in data object (For fake items and scripts that don't
// have an actual entity)

// move to game_local.h?
typedef struct SObjEntParms_s
{
	idStr	name;
	idStr	group; // inventory group for items, e.g., loot group "gems"
	idStr	classname;
	idStr	spawnclass;

	idVec3	origin;

// AI data:
	int		team;
	int		type;
	int		innocence;

/**
* Numerical value, filled by callbacks in some cases for things that are kept
* track of externally (for example, number of inventory items, overall loot, etc)
**/
	int value; // should default to 1
	int valueSuperGroup; // Just used to pass overall loot for now

	bool bIsAI;
	bool bWhileAirborne; // a must-have :)
} SObjEntParms;

class CObjectiveComponent
{
public:
	friend class CMissionData;
	friend class CObjective;

	CObjectiveComponent( void );
	virtual ~CObjectiveComponent( void );

	/**
	* Input type and argument data for the objective component.
	* The specification method list, integer arguments list and string arguments
	* list should all be filled before calling this to copy them in.
	**/
//	Setup( EComponentType type, idList<ESpecificationMethod> &SpecMethods, 
//			idList<int> &IntArgs, idStrList &StrArgs );  

	/**
	* Update the state of the objective component.  
	* Returns true if the state has changed as of this call
	**/
	bool SetState( bool bState );

public:
	/**
	* Index of this component in the form of [objective num, component num]
	* NOTE: This index is that from the external scripting.
	* So it starts at 1, not at zero.
	**/
	int m_Index[2]; 

protected:

/**
* Set to true if the FM author has NOTted this component
**/
	bool m_bNotted;
	
	EComponentType m_Type;

// This could be made more general into a list, but I can't think of any component
// types that would require more than 2 items to match.  More complicated logic
// can be constructed out of multiple components.
	ESpecificationMethod m_SpecMethod[2];

/**
* Values of the specifier to match, e.g., if specmethod is group, specvalue is "beast"
* Could be either an int or a string depending on spec type, so make room for both.
**/
	idStr m_SpecStrVal[2];
	int   m_SpecIntVal[2];

	/**
	* Current component state (true/false)
	**/
	bool		m_bState;

	/**
	* Whether the irreversible component has latched into a state
	**/
	bool		m_bLatched;

	idList<int>	m_IntArgs;
	idStrList	m_StrArgs;

// Only used by clocked objectives:
	int			m_ClockInterval; // milliseconds
	
	int			m_TimeStamp;

	/**
	* Whether the objective component latches after it changes once
	* Default is reversible.
	* NOT YET IMPLEMENTED
	**/
	bool m_bReversible;

}; // CObjectiveComponent

/**
* Abstract class for storing objective data
* This class contains all the objective components
**/

class CObjective
{
public:
	friend class CObjectiveComponent;
	friend class CMissionData;

	CObjective( void );

	virtual ~CObjective( void );

	void Clear( void );

public:
	/** 
	* Text description of the objective in the objectives GUI
	**/
	idStr m_text;

	/** 
	* Set to false if an objective is optional
	**/
	bool m_bMandatory;

	/**
	* Sets whether the objective is shown in the objectives screen
	**/
	bool m_bVisible;
	
	/**
	* True if an objective is ongoing throughout the mission.
	* Will not be checked off as complete until the mission is over
	**/
	bool m_bOngoing;

	/**
	* Sets the difficulty level of this objective. Objective only appears at and above this difficutly level
	* TODO: Change to bitfield so objectives can also disappear at higher difficulties
	**/
	int m_MinDifficulty;

protected:
	/**
	* Completion state.  Either COMP_INCOMPLETE, COMP_COMPLETE, COMP_FAILED or COMP_INVALID
	**/
	EObjCompletionState	m_state;

	/**
	* Set to true if one of the components changed this frame.  Test resets it to false.
	*/
	bool m_bNeedsUpdate;

	/**
	* Whether the objective may change state again once it initially changes to FAILED or SUCCESSFUL
	* Default is reversible.
	**/
	bool m_bReversible;

	/**
	* Set to true if the objective is irreversible and has latched into a state
	**/
	bool m_bLatched;

	/**
	* List of objective components (steal this, kill that, etc)
	**/
	idList<CObjectiveComponent> m_Components;

	/**
	* Other objectives that must be completed prior to the completion of this objective
	**/
	idList<int> m_EnablingObjs;

	/**
	* String storing the script to call when this objective is completed
	* (optional)
	**/
	idStr m_CompletionScript;

	/**
	* String storing the script to call when this objective is failed
	* (optional)
	**/
	idStr m_FailureScript;
	
// TODO: Add these
	// boolean relationship among objective components required for definite success
	
	// boolean relationship among objective components required for definite failure
	
};

typedef struct SStat_s
{
	int Overall;
	int ByTeam[ MAX_TEAMS ];
	int ByType[ MAX_TYPES ];
	int ByInnocence[2];
	int WhileAirborne;
} SStat;

/**
* Mission stats: Keep track of everything except for loot groups, which are tracked by the inventory
**/
typedef struct SMissionStats_s
{
// AI Stats:
	SStat AIStats[ MAX_AICOMP ];
	
	SStat AIAlerts[ MAX_ALERTNUMS ];

	int DamageDealt;
	int DamageReceived;

	// Item stats are handled by the inventory, not here, 
	// Might need this for copying over to career stats though
	int LootOverall;

} SMissionStats;

/**
* CMissionData handles the tasks of maintaining stats and objective completion status
* in response to ingame events.
*
* Also handles the task of parsing objectives written by FM author
**/

class CMissionData 
{	
public:
	CMissionData( void );
	virtual ~CMissionData( void );

	void Clear( void );

	/**
	* Update objectives if they need it
	* Called each frame by idPlayer::Think, does nothing if no objectives need updating
	**/
	void UpdateObjectives( void );

	/**
	* Set the completion state of an objective.  Called both externally and internally.
	* NOTE: Uses the "internal" index number, so subtract the index by 1 if calling it with "user" index
	**/
	void SetCompletionState( int ObjIndex, int State );

	/**
	* Get completion state.  Uses "internal" index (starts at 0)
	**/
	int GetCompletionState( int ObjIndex );

	/**
	* Get component state.  Uses "internal" index (starts at 0)
	**/
	bool GetComponentState( int ObjIndex, int CompIndex );

	/**
	* Sets a given component state.  
	* Externally called version: Checks and reports index validity
	* Uses "user" index (starts at 1 instead of 0)
	* Calls internal SetComponentState.
	**/
	void SetComponentState_Ext( int ObjIndex, int CompIndex, bool bState );

	/**
	* Unlatch an irreversible objective (used by scripting)
	* Uses internal index (starts at 0)
	**/
	void UnlatchObjective( int ObjIndex );

	/**
	* Unlatch an irreversible component (used by scripting)
	* Uses internal indeces(starts at 0)
	**/
	void UnlatchObjectiveComp( int ObjIndex, int CompIndex );


/**
* Set whether an objective shows up in the player's objectives screen
**/
	void Event_SetObjVisible( int ObjIndex, bool bVal );
// self explanatory
	void Event_SetObjMandatory( int ObjIndex, bool bVal );
	void Event_SetObjOngoing( int ObjIndex, bool bVal );
/**
* Replace an objective's list of enabling components with a new one
* Takes a string list of space-delimited ints and parses it in.
**/
	void Event_SetObjEnabling( int ObjIndex, idStr StrIn );
//	void Event_SetObjDifficulty( int ObjIndex, int value );

/**
* Getters for the mission stats.  Takes an objective component event type,
* index for the category (for example, the index would be the team int if 
* you are calling GetStatByTeam)
* 
* The AlertNum must be specified if you are getting alert stats, but otherwise
* is optional.
**/
	int GetStatByTeam( EComponentType CompType, int index, int AlertNum = 0 );
	int GetStatByType( EComponentType CompType, int index, int AlertNum = 0 );
	int GetStatByInnocence( EComponentType CompType, int index, int AlertNum = 0 );
/**
* The following stat functions don't need an index var, since there is only one of them tracked
**/
	int GetStatOverall( EComponentType CompType, int AlertNum = 0 );
	int GetStatAirborne( EComponentType CompType, int AlertNum = 0);
	int GetDamageDealt( void );
	int GetDamageReceived( void );

// Callback functions:

	/**
	* Called by external callbacks when events occur that could effect objectives.
	* This is the most general call and requires passing in filled-in SObjEntParms objects
	* bBoolArg is a multifunctional bool argument
	* 
	* For AI, bBoolArg represents whether the player is responsible for the action
	* 
	* For items and locations, bBoolArg is true when the ent is entering the location/inventory
	* and false when leaving the location or being dropped from the inventory.
	*
	* This is the most generic version, will be called by the inventory after it puts "value" into the parms
	**/
	void CMissionData::MissionEvent
		( 
		EComponentType CompType, 					 
		SObjEntParms *EntDat1, 				 
		SObjEntParms *EntDat2,
		bool bBoolArg = false
		);

	void CMissionData::MissionEvent
		( 
		EComponentType CompType, 					 
		SObjEntParms *EntDat1, 				 
		bool bBoolArg = false
		)
	{ MissionEvent( CompType, EntDat1, NULL, bBoolArg ); };

	/**
	* Overloaded MissionEvent with entities instead of parms 
	* (for real ents as opposed to fake inventory items)
	* Used by AI events, locations, etc
	**/
	void CMissionData::MissionEvent
		( 
		EComponentType CompType, 					 
		idEntity *Ent1, 				 
		idEntity *Ent2,
		bool bBoolArg,
		bool bWhileAirborne = false
		);

	void CMissionData::MissionEvent
		( 
		EComponentType CompType, 					 
		idEntity *Ent1, 				 
		bool bBoolArg,
		bool bWhileAirborne = false
		)
	{ MissionEvent( CompType, Ent1, NULL, bBoolArg, bWhileAirborne ); }

	/**
	* Fill the SObjEntParms data from an entity.  Does not fill in value and superGroupValue
	**/
	void FillParmsData( idEntity *ent, SObjEntParms *parms );
	
	/**
	* Called when the player takes damage.  Used for damage stats
	**/
	void PlayerDamaged( int DamageAmount );

	/**
	* Called when the player damages AI.  Used for damage stats.
	**/
	void AIDamagedByPlayer( int DamageAmount );

	/**
	* Parse the objective data on an entity and add it to the objectives system
	* Called by CTarget_AddObjectives
	* This may be done during gameplay to add new objectives
	* Returns the index of the LAST objective added, for later addressing
	**/
	int AddObjsFromEnt( idEntity *ent );

// Events
	/**
	* The following are called internally when an objective completes or fails
	**/
	void Event_ObjectiveComplete( int ObjIndex );
	void Event_ObjectiveFailed( int ObjIndex );

	void Event_MissionComplete( void );
	void Event_MissionFailed( void );

protected:
	
	/**
	* Sets a given component state.  
	* Internally used version: Doesn't check/report index validity
	* Uses "internal" index (starts at 0 instead of 1)
	**/
	void SetComponentState( int ObjIndex, int CompIndex, bool bState );

	/**
	* Set component state when indexed by a pointer to a component
	**/
	void SetComponentState( CObjectiveComponent *pComp, bool bState );

	/**
	* Do the numerical comparison
	* Get the number to compare, either provided or from stats, depending on component type
	* e.g., for AI events, get the number from stats.
	* For item events, the number will be passed in by the callback from the inventory.
	**/
	bool	EvaluateObjective
		(
			CObjectiveComponent *pComp,
			SObjEntParms *EntDat1,
			SObjEntParms *EntDat2,
			bool bBoolArg
		);

	/**
	* Reads the specification type from the objective component,
	* then tests if that specification on EntDat matches that in the component.
	* The index determines which specification of the component to check,
	* since some components have two objectives. ind=0 => first, ind=1 => second
	* A component has a max of two specificaton checks, so ind should never be > 1.
	**/
	bool	MatchSpec( CObjectiveComponent *pComp, SObjEntParms *EntDat, int ind );

protected:
	/**
	* Set to true if any of the objective states have changed and objectives need updating
	**/
	bool m_bObjsNeedUpdate;

	/**
	* List of current objectives
	**/
	idList<CObjective> m_Objectives;

	/**
	* Pointers to objective components that are centrally clocked
	* Components that fall under this domain are:
	* CUSTOM_CLOCKED, DISTANCE, and TIMER
	**/
	idList<CObjectiveComponent *> m_ClockedComponents;

	/**
	* Object holding all mission stats relating to AI, damage to player and AI
	* Loot stats are maintained by the inventory
	* TODO: Also put in a persistent stats object 
	**/
	SMissionStats m_Stats;

	/**
	* Hash indices to store string->enum conversions for objective component type and
	* specification method type.
	* Used for parsing objectives from spawnargs.
	**/
	idHashIndex m_CompTypeHash;
	idHashIndex m_SpecTypeHash;

}; // CMissionData

// Helper entity for objective locations
class CObjectiveLocation : public idEntity
{
public:
	CLASS_PROTOTYPE( CObjectiveLocation );
	
	CObjectiveLocation( void );

	void Think( void );
	void Spawn( void );

//	void Save( idSaveGame *savefile ) const;
//	void Restore( idRestoreGame *savefile );

protected:
	/**
	* Clock interval [seconds]
	**/
	int m_Interval;

	int m_TimeStamp;

	/**
	* List of entity names that intersected bounds in previous clock tick
	**/
	idStrList m_EntsInBounds;

}; // CObjectiveLocation
	


#endif // MISSIONDATA_H