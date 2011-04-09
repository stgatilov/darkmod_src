/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "DifficultyManager.h"
#include "Inventory/LootType.h"

/**
 * greebo: A structure defining loot carry-over rules for the shop.
 * On the one hand the mapper can define how much of the collected loot 
 * makes it into the shop and on the other hand the mapper can 
 * allow or disallow keeping the unspent gold after the shop.
 *
 * A default-constructed LootRules set won't change the incoming loot values
 * everything is losslessly passed through with a 1:1 conversion, 
 * and the player is allowed to keep his money.
 */
struct LootRuleSet
{
	// The conversion rate for each loot type. Before entering the shop
	// every loot type is converted to gold.
	float	conversionRate[LOOT_COUNT];

	// Gold loss after conversion (defaults to 0 => no loss)
	int		goldLoss;

	// Gold loss by percent after conversion (defaults to 0 => no loss)
	float	goldLossPercent;

	// Maximum amount of gold available in the shop. Enforced after conversion. (default is -1 => no cap)
	int		goldCap;

	// Whether the player is allowed to keep his unspent gold after the shop (defaults to true)
	bool	keepUnspentGold;

	LootRuleSet()
	{
		Clear();
	}

	void Clear()
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

	// Equality operator, returns true if this class is the same as the other. Doesn't check LOOT_NONE conversion.
	bool operator==(const LootRuleSet& other) const
	{
		return goldLoss == other.goldLoss && goldLossPercent == other.goldLossPercent &&
			   goldCap == other.goldCap && keepUnspentGold == other.keepUnspentGold &&
			   conversionRate[LOOT_GOLD] == other.conversionRate[LOOT_GOLD] &&
			   conversionRate[LOOT_JEWELS] == other.conversionRate[LOOT_JEWELS] &&
			   conversionRate[LOOT_GOODS] == other.conversionRate[LOOT_GOODS];
	}

	// Returns TRUE if this lootrule structure is at default values
	// This is a comparably slow call, so don't use it excessively
	bool IsEmpty() const
	{
		// Compare this instance against a default-constructed one
		return *this == LootRuleSet();
	}
};
