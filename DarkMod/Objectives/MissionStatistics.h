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
struct SMissionStats
{
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

	SMissionStats() 
	{
		Clear();
	}

	void Clear();

	// Returns the sum of all found loot types (gold+jewels+goods)
	int GetFoundLootValue() const;

	// Returns the total of all loot types in the mission (gold+jewels+goods)
	int GetTotalLootInMission() const;

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};

#if 0
/**
 * Objective history. Each mission stores the final
 * state of its objectives here.
 */
class ObjectiveHistory
{
private:
	// Each mission has an array of objective states
	typedef idList<EObjCompletionState> ObjectiveStates;

	// The internal array of ObjectiveStates, one for each mission
	idList<ObjectiveStates> _objHistory;

public:
	// greebo: Store the state of the given objective for the given mission number
	void SetMissionObjectiveState(int missionNum, int objNum, EObjCompletionState state);

	// Returns the state of the requested objective of the requested mission. 
	// If no such objective state or mission was stored, the state INVALID is returned.
	EObjCompletionState GetMissionObjectiveState(int missionNum, int objNum) const;

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	void EnsureHistorySize(int size);
	void EnsureMissionSize(int missionNum, int size);
};
#endif

#endif /* MISSIONSTATISTICS_H */
