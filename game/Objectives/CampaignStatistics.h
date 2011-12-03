/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef TDM_CAMPAIGN_STATISTICS_H
#define TDM_CAMPAIGN_STATISTICS_H

#include "../idlib/precompiled.h"

#include "MissionStatistics.h"

/**
 * Multiple mission stats structs combined => campaign stats.
 * First mission is carrying index 0.
 */
class CampaignStats
{
private:
	// The internal array of statistics
	idList<MissionStatistics> _stats;

public:
	// greebo: Use this operator to get access to the stats of the mission with the given index
	// The internal list will automatically be resized to fit.
	MissionStatistics& operator[] (int index)
	{
		EnsureSize(index + 1);
		return _stats[index];
	}

	const MissionStatistics& operator[] (int index) const
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

#endif
