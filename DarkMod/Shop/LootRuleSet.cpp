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

#include "LootRuleSet.h"

void LootRuleSet::Clear()
{
	conversionRate[LOOT_NONE] = 0;
	conversionRate[LOOT_GOLD] = 1;	// 1:1 exchange rates by default
	conversionRate[LOOT_JEWELS] = 1;
	conversionRate[LOOT_GOODS] = 1;

	goldLoss = 0;
	goldLossPercent = 0;
	goldCap = -1;
	keepUnspentGold = true;
}

bool LootRuleSet::operator==(const LootRuleSet& other) const
{
	return goldLoss == other.goldLoss && goldLossPercent == other.goldLossPercent &&
		   goldCap == other.goldCap && keepUnspentGold == other.keepUnspentGold &&
		   conversionRate[LOOT_GOLD] == other.conversionRate[LOOT_GOLD] &&
		   conversionRate[LOOT_JEWELS] == other.conversionRate[LOOT_JEWELS] &&
		   conversionRate[LOOT_GOODS] == other.conversionRate[LOOT_GOODS];
}

void LootRuleSet::LoadFromDict(const idDict& dict, const idStr& prefix)
{
	
}
