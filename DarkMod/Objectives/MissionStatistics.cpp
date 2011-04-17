/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "MissionStatistics.h"

void SStat::Clear() 
{
	Overall = 0;

	for (int i = 0; i < MAX_TEAMS; i++)
	{
		ByTeam[i] = 0;
	}

	for (int i = 0; i < MAX_TYPES; i++)
	{
		ByType[i] = 0;
	}

	ByInnocence[0] = 0;
	ByInnocence[1] = 0;
	WhileAirborne = 0;
}

void SMissionStats::Clear()
{
	for (int i = 0; i < MAX_AICOMP; i++)
	{
		AIStats[i].Clear();
	}

	for (int i = 0; i < MAX_ALERTLEVELS; i++)
	{
		AIAlerts[i].Clear();
	}

	DamageDealt = 0;
	DamageReceived = 0;
	HealthReceived = 0;
	PocketsPicked = 0;

	for (int i = 0; i < LOOT_COUNT; ++i)
	{
		FoundLoot[i] = 0;
		LootInMission[i] = 0;
	}
	
	TotalGamePlayTime = 0;
}

void SMissionStats::Save(idSaveGame* savefile) const
{
	// Save mission stats
	for(int j = 0; j < MAX_AICOMP; j++)
	{
		savefile->WriteInt(AIStats[j].Overall);
		savefile->WriteInt(AIStats[j].WhileAirborne);

		for (int k1 = 0; k1 < MAX_TEAMS; k1++)
		{
			savefile->WriteInt(AIStats[j].ByTeam[k1]);
		}

		for (int k2 = 0; k2 < MAX_TYPES; k2++)
		{
			savefile->WriteInt(AIStats[j].ByType[k2]);
		}

		savefile->WriteInt(AIStats[j].ByInnocence[0]);
		savefile->WriteInt(AIStats[j].ByInnocence[1]);
	}

	for (int l = 0; l < MAX_ALERTLEVELS; l++)
	{
		savefile->WriteInt(AIAlerts[l].Overall);
		savefile->WriteInt(AIAlerts[l].WhileAirborne);

		for (int m1 = 0; m1 < MAX_TEAMS; m1++)
		{
			savefile->WriteInt(AIAlerts[l].ByTeam[m1]);
		}

		for (int m2 = 0; m2 < MAX_TYPES; m2++)
		{
			savefile->WriteInt(AIAlerts[l].ByType[m2]);
		}

		savefile->WriteInt(AIAlerts[l].ByInnocence[0]);
		savefile->WriteInt(AIAlerts[l].ByInnocence[1]);
	}

	savefile->WriteInt(DamageDealt);
	savefile->WriteInt(DamageReceived);
	savefile->WriteInt(PocketsPicked);

	for (int i = 0; i < LOOT_COUNT; ++i)
	{
		savefile->WriteInt(FoundLoot[i]);
		savefile->WriteInt(LootInMission[i]);
	}

	savefile->WriteUnsignedInt(TotalGamePlayTime);
}

void SMissionStats::Restore(idRestoreGame* savefile)
{
	// Restore mission stats
	for (int j = 0; j < MAX_AICOMP; j++)
	{
		savefile->ReadInt(AIStats[j].Overall);
		savefile->ReadInt(AIStats[j].WhileAirborne);

		for (int k1 = 0; k1 < MAX_TEAMS; k1++)
		{
			savefile->ReadInt(AIStats[j].ByTeam[k1]);
		}

		for (int k2 = 0; k2 < MAX_TYPES; k2++)
		{
			savefile->ReadInt(AIStats[j].ByType[k2]);
		}

		savefile->ReadInt(AIStats[j].ByInnocence[0]);
		savefile->ReadInt(AIStats[j].ByInnocence[1]);
	}

	for (int l = 0; l < MAX_ALERTLEVELS; l++)
	{
		savefile->ReadInt(AIAlerts[l].Overall);
		savefile->ReadInt(AIAlerts[l].WhileAirborne);

		for (int m1 = 0; m1 < MAX_TEAMS; m1++)
		{
			savefile->ReadInt(AIAlerts[l].ByTeam[m1]);
		}

		for (int m2 = 0; m2 < MAX_TYPES; m2++)
		{
			savefile->ReadInt(AIAlerts[l].ByType[m2]);
		}

		savefile->ReadInt(AIAlerts[l].ByInnocence[0]);
		savefile->ReadInt(AIAlerts[l].ByInnocence[1]);
	}	

	savefile->ReadInt(DamageDealt);
	savefile->ReadInt(DamageReceived);
	savefile->ReadInt(PocketsPicked);

	for (int i = 0; i < LOOT_COUNT; ++i)
	{
		savefile->ReadInt(FoundLoot[i]);
		savefile->ReadInt(LootInMission[i]);
	}

	savefile->ReadUnsignedInt(TotalGamePlayTime);
}

int SMissionStats::GetFoundLootValue() const
{
	int sum = 0;

	for (int i = LOOT_NONE+1; i < LOOT_COUNT; ++i)
	{
		sum += FoundLoot[i];
	}

	return sum;
}

int SMissionStats::GetTotalLootInMission() const
{
	int sum = 0;

	for (int i = LOOT_NONE+1; i < LOOT_COUNT; ++i)
	{
		sum += LootInMission[i];
	}

	return sum;
}

#if 0

void ObjectiveHistory::SetMissionObjectiveState(int missionNum, int objNum, EObjCompletionState state)
{
	// Ensure the history has enough space
	EnsureMissionSize(missionNum, objNum + 1);

	_objHistory[missionNum][objNum] = state;
}

EObjCompletionState ObjectiveHistory::GetMissionObjectiveState(int missionNum, int objNum) const
{
	if (missionNum < 0 || missionNum >= _objHistory.Num()) return STATE_INVALID;

	if (objNum < 0 || objNum >= _objHistory[missionNum].Num()) return STATE_INVALID;

	return _objHistory[missionNum][objNum];
}

void ObjectiveHistory::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(_objHistory.Num());

	for (int m = 0; m < _objHistory.Num(); ++m)
	{
		const ObjectiveStates& states = _objHistory[m];

		savefile->WriteInt(states.Num());

		for (int i = 0; i < states.Num(); ++i)
		{
			savefile->WriteInt(states[i]);
		}
	}
}

void ObjectiveHistory::Restore(idRestoreGame* savefile)
{
	int num;
	savefile->ReadInt(num);
	_objHistory.SetNum(num);
	
	for (int m = 0; m < _objHistory.Num(); ++m)
	{
		const ObjectiveStates& states = _objHistory[m];

		savefile->ReadInt(num);
		states.SetNum(num);
		
		for (int i = 0; i < states.Num(); ++i)
		{
			int state;
			savefile->WriteInt(state);

			assert(state >= STATE_INCOMPLETE && state <= STATE_FAILED);

			states[i] = static_cast<EObjCompletionState>(state);
		}
	}
}

void ObjectiveHistory::EnsureHistorySize(int size)
{
	if (_objHistory.Num() < size)
	{
		_objHistory.SetNum(size);
	}
}

void ObjectiveHistory::EnsureMissionSize(int missionNum, int size)
{
	EnsureHistorySize(missionNum + 1);

	if (_objHistory[missionNum].Num() < size)
	{
		_objHistory[missionNum].SetNum(size);
	}
}

#endif
