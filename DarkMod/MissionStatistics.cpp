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
	FoundLoot = 0;
	TotalLootInMission = 0;
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
	savefile->WriteInt(FoundLoot);
	savefile->WriteInt(TotalLootInMission);
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
	savefile->ReadInt(FoundLoot);
	savefile->ReadInt(TotalLootInMission);
	savefile->ReadUnsignedInt(TotalGamePlayTime);
}

void CampaignStats::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(_stats.Num());

	for (int i = 0; i < _stats.Num(); ++i)
	{
		_stats[i].Save(savefile);
	}
}

void CampaignStats::Restore(idRestoreGame* savefile)
{
	int num;
	savefile->ReadInt(num);
	_stats.SetNum(num);

	for (int i = 0; i < num; ++i)
	{
		_stats[i].Restore(savefile);
	}
}

void CampaignStats::EnsureSize(int size)
{
	if (_stats.Num() < size)
	{
		_stats.SetNum(size);
	}
}
