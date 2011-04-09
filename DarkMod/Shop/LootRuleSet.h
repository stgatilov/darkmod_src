/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef TDM_LOOTRULESET_H
#define TDM_LOOTRULESET_H

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

	void Clear();

	// Equality operator, returns true if this class is the same as the other. Doesn't check LOOT_NONE conversion.
	bool operator==(const LootRuleSet& other) const;

	// Returns TRUE if this lootrule structure is at default values
	// This is a comparably slow call, so don't use it excessively
	bool IsEmpty() const
	{
		// Compare this instance against a default-constructed one
		return *this == LootRuleSet();
	}

	// Load the ruleset from the spawnargs matching the given prefix
	void LoadFromDict(const idDict& dict, const idStr& prefix);
};

#endif
