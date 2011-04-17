/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef MISSIONSTATISTICS_H
#define MISSIONSTATISTICS_H

#include "../idlib/precompiled.h"

#include "Objective.h" // for objective state enum
#include "../Inventory/Item.h" // for loot type enum

// Maximum array sizes:
#define MAX_TEAMS 64
#define MAX_TYPES 16
#define MAX_AICOMP 16
#define MAX_ALERTLEVELS 16

struct SStat
{
	int Overall;
	int ByTeam[ MAX_TEAMS ];
	int ByType[ MAX_TYPES ];
	int ByInnocence[2];
	int WhileAirborne;

	SStat() 
	{
		Clear();
	}

	void Clear();
};

/**
* Mission stats: Keep track of everything except for loot groups, which are tracked by the inventory
**/
class MissionStatistics
{
public:
	// AI Stats:
	SStat AIStats[ MAX_AICOMP ];
	
	SStat AIAlerts[ MAX_ALERTLEVELS ];

	int DamageDealt;
	int DamageReceived;
	int HealthReceived;
	int PocketsPicked;

	// Item stats are handled by the inventory, not here, 
	// Might need this for copying over to career stats though
	int FoundLoot[LOOT_COUNT];

	// greebo: This is the available amount of loot in the mission
	int LootInMission[LOOT_COUNT];

	// This gets read out right at "mission complete" time, is 0 before
	unsigned int TotalGamePlayTime;

	// Use an array to store the objective states after mission complete
	// We need the historic state data to handle conditional objectives.
	// This list will be empty throughout the mission, and is filled on mission complete
	idList<EObjCompletionState> ObjectiveStates;

	MissionStatistics() 
	{
		Clear();
	}

	void Clear();

	// Returns the state of the objective specified by the (0-based) index
	// Will return INVALID if the objective index is out of bounds or no data is available
	EObjCompletionState GetObjectiveState(int objNum) const;

	// Store the objective state into the ObjectiveStates array
	void SetObjectiveState(int objNum, EObjCompletionState state);

	// Returns the sum of all found loot types (gold+jewels+goods)
	int GetFoundLootValue() const;

	// Returns the total of all loot types in the mission (gold+jewels+goods)
	int GetTotalLootInMission() const;

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};

#endif /* MISSIONSTATISTICS_H */
