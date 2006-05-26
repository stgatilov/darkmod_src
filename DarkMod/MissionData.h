/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2006/05/26 10:24:39  ishtvan
 * Initial release
 *
 *
 *
 ***************************************************************************/

// Copyright (C) 2006 Chris Sarantos <csarantos@gmail.com>
//

// TODO: Add support for irreversible objectives

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
	SPEC_GROUP, // for inventory items
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
	COMP_LOCATION,

// Special components, called separately
	COMP_DISTANCE, // distance of X from Y, this check will be triggered by a clock (same clock that triggers custom_clocked)
	COMP_CUSTOM_CLOCKED,
	COMP_CUSTOM_ASYNC
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
	friend class CMissionData;

public:

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

private:

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

	idList<int>	m_IntArgs;
	idStrList	m_StrArgs;

	// Only used by custom clocked objectives
	idStr		m_CustomClockedScript;
	int			m_CustomClockInterval; // milliseconds
}; // CObjectiveComponent

typedef struct SObjective_s
{
	// constructor for default values
	SObjective_s( void )
	{
		state = STATE_INCOMPLETE;
		bNeedsUpdate = false;
		bMandatory = false;
		bVisible = true;
		bOngoing = false;
		MinDifficulty = 0;

		Components.Clear();
	}
	~SObjective_s( void )
	{
		Components.Clear();
	}
	
	EObjCompletionState	state;

	// Set to true if one of the components changed this frame.  Test resets it to false.
	bool bNeedsUpdate;

	// Do we need optional objectives? 
	bool bMandatory;

	// sets whether the objective is shown in the objectives screen and at the beginning of the mission
	bool bVisible;
	
	/**
	* True if an objective is ongoing throughout the mission.
	* Will not be checked off as complete until the mission is over
	**/
	bool bOngoing;

	/**
	* Sets the difficulty level of this objective. Objective only appears at and above this difficutly level
	* TODO: Change to bitfield so objectives can also disappear at higher difficulties
	**/
	int MinDifficulty;
	
	// internal stuff from this point on
	
	// list of objective components (steal this, kill that, etc)
	idList<CObjectiveComponent> Components;
	
// TODO: Add these
	// boolean relationship among objective components required for definite success
	
	// boolean relationship among objective components required for definite failure

	// script to call when objective is completed
	
} SObjective;

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
	friend class CObjectiveComponent;

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
	* Sets a given component state.  
	* Used mostly by script callbacks for custom objectives
	**/
	void Event_SetComponentState( int ObjIndex, int CompIndex, bool bState );

	/**
	* Set an objective state to one of the completion states (used by external scripting)
	**/
	void Event_SetObjComplete( int ObjIndex );
	void Event_SetObjInComplete( int ObjIndex );
	void Event_SetObjFailed( int ObjIndex );
	void Event_SetObjInvalid( int ObjIndex );
/**
* Set whether an objective shows up in the player's objectives screen
**/
	void Event_SetObjVisible( int ObjIndex, bool bVal );
// self explanatory
	void Event_SetObjMandatory( int ObjIndex, bool bVal );
	void Event_SetObjOngoing( int ObjIndex, bool bVal );
//	void Event_SetObjDifficulty( int ObjIndex, int value );

//	int AddObjective( void );

//	int AddComponent( int ObjIndex );

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
		SObjEntParms *EntDat2 = NULL,
		bool bBoolArg = false
		);

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


// Other

	/**
	* // TODO: Repeat the check or don't... items that have to stay in a position
	* // One way to do this would be the entity has to keep getting the stim
	* // We must figure out how to handle items that must stay at position
	*
	* Called when an entity reaches an objective position.
	* 
	* In practice, this will usually be called by a scriptfunction, either
	* by a trigger or by stim/response.
	*
	* This general function handles several potential objectives:
	* Player getting to a place,
	* AI getting to a place (used for escort missions ala T2M1?),
	* Player dropping object in a place,
	* Player dropping KO'd AI in a place
	**/
	void EntityReachedPosition( idStr EntName, idStr PositionName, bool bPresent );

// Events
	/**
	* The following set the various objective states, called internally and in external scripts
	**/
	void Event_ObjectiveComplete( int ObjIndex );
	void Event_ObjectiveFailed( int ObjIndex );
	void Event_ObjectiveInvalid( int ObjIndex );
	void Event_ObjectiveIncomplete( int ObjIndex );

	void Event_MissionComplete( void );
	void Event_MissionFailed( void );


private:

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

	/**
	* Set the completion state of an objective.  This will be called by separate event functions
	* for each state, for userfriendliness (so the scripter doesn't need to remember state numbers)
	**/
	void SetCompletionState( int ObjIndex, EObjCompletionState State );

	// Temporary test method: sets up an objective
	void RunTest( void );

private:
	bool m_bObjsNeedUpdate;

	idList<SObjective> m_Objectives;

	SMissionStats m_Stats;
}; // CMissionData


#endif // MISSIONDATA_H