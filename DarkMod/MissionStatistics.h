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
	int FoundLoot;

	// greebo: This is the available amount of loot in the mission
	int TotalLootInMission;

	// This gets read out right at "mission complete" time, is 0 before
	unsigned int	TotalGamePlayTime;

	SMissionStats() 
	{
		Clear();
	}

	void Clear();

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};

/**
 * Multiple mission stats structs combined => campaign stats.
 * First mission is carrying index 0.
 */
class CampaignStats
{
private:
	// The internal array of statistics
	idList<SMissionStats> _stats;

public:
	// greebo: Use this operator to get access to the stats of the mission with the given index
	// The internal list will automatically be resized to fit.
	SMissionStats& operator[] (int index)
	{
		EnsureSize(index + 1);
		return _stats[index];
	}

	const SMissionStats& operator[] (int index) const
	{
		return _stats[index];
	}

	int Num() const
	{
		return _stats.Num();
	}

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	void EnsureSize(int size);
};

#endif /* MISSIONSTATISTICS_H */
