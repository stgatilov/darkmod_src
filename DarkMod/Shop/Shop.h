// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __SHOP_H__
#define	__SHOP_H__

#define LIST_SIZE_FOR_SALE 11
#define LIST_SIZE_PURCHASED 9
#define LIST_SIZE_STARTING 7

#include "DifficultyManager.h"
#include "Inventory/LootType.h"
#include <boost/shared_ptr.hpp>

// Represents an item for sale
class CShopItem
{
private:
	idStr		id;
	idStr		name;
	idStr		description;
	int			cost;
	idStr		image;
	int			count;
	bool		persistent;
	bool		canDrop;
	int			dropped;	// tels if dropped, store how many we had so player can undo drop

	// The list of entityDef names to add to the player's inventory 
	// when this shop item is purchased
	idStringList classNames;
	
	bool		stackable; // grayman (#2376)

public:
	CShopItem();

	CShopItem(const idStr& id, 
			  const idStr& name, 
			  const idStr& description, 
			  int cost,
			  const idStr& image, 
			  int count, 
			  bool persistent = false, 
			  bool canDrop = true,
			  bool stackable = false); // grayman (#2376)

	CShopItem(const CShopItem& item, 
			  int count, 
			  int cost = 0, 
			  bool persistent = false);

	// unique identifier for this item
	const idStr& GetID() const;

	// name of the item (for display)
	const idStr& GetName() const;

	// description of the item (for display)
	const idStr& GetDescription() const;

	// Get the list of classnames of entities to spawn for this shop item
	const idStringList& GetClassnames() const;

	// Adds a new classname for this shop item to be added to the player's inventory
	void AddClassname(const idStr& className);

	// cost of the item
	int GetCost();	

	// modal name (for displaying)
	const idStr& GetImage() const;

	// number of the items in this collection (number for sale,
	// or number user has bought, or number user started with)
	int GetCount();				

	// if starting item and it was dropped, the count before the drop (so we can undrop it)
	int GetDroppedCount();				

	// whether the item can be carried to the next mission
	bool GetPersistent();

	// whether the item can dropped by the player from the starting items list
	bool GetCanDrop();
	void SetCanDrop(bool canDrop);

	// grayman (#2376) - whether the item can be stacked
	bool GetStackable();
	void SetStackable(bool stackable);

	// modifies number of items
	void ChangeCount( int amount );

	// sets dropped => count and count => 0
	void Drop( void );

	// sets count => dropped and dropped => 0
	void Undrop( void );

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);
};
typedef boost::shared_ptr<CShopItem> CShopItemPtr;

// A list of shop items
typedef idList<CShopItemPtr> ShopItemList;

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
struct LootRules
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

	LootRules()
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
	bool operator==(const LootRules& other) const
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
		return *this == LootRules();
	}
};

// Represents the Shop
class CShop
{
private:
	ShopItemList	_itemDefs;
	ShopItemList	_itemsForSale;
	ShopItemList	_itemsPurchased;
	ShopItemList	_startingItems;

	int				_gold;
	int				_forSaleTop;
	int				_purchasedTop;
	int				_startingTop;

	// True if the purchase menu should be skipped
	bool			_skipShop;
	
	// grayman (#2376) - Lockpick handling
	bool			_pickSetShop;
	bool			_pickSetStarting;

	// The non-difficulty-specific loot rules, apply if not overridden by diff-specific ones
	LootRules		generalLootRules;

	// The difficulty-specific loot rules, will override generalLootRules
	LootRules		diffLootRules[DIFFICULTY_COUNT];

public:
	void Init();

	// Clears out all lists
	void Clear();

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);

	// move the current inventory to the Starting Items list
	void LoadFromInventory(idPlayer *player);

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui, idPlayer *player);

	// how much gold the player has left to spend
	int GetGold();
	void SetGold(int gold);
	void ChangeGold(int change);

	// put item in the For Sale list
	void AddItemForSale(const CShopItemPtr& shopItem);

	// put item in the Starting Items list
	void AddStartingItem(const CShopItemPtr& shopItem);

	// initializes the 'list' based on the map
	int AddItems(const idDict& mapDict, const idStr& itemKey, ShopItemList& list);

	// returns the various lists
	const ShopItemList& GetItemsForSale();
	const ShopItemList& GetStartingItems();
	const ShopItemList& GetPurchasedItems();

	// returns the combination of For Sale and Starting items
	ShopItemList GetPlayerStartingEquipment();

	// adjust the lists
	void SellItem(int index);
	void BuyItem(int index);
	void DropUndropItem(int index);

	// update the GUI variables to match the shop
	void UpdateGUI(idUserInterface* gui);

	// find items based on the id
	CShopItemPtr FindPurchasedByID(const char *id);
	CShopItemPtr FindForSaleByID(const char *id);
	CShopItemPtr FindStartingItemByID(const char *id);

	CShopItemPtr FindByID(ShopItemList& items, const char *id);

	// initialize the shop
	void DisplayShop(idUserInterface *gui);

	// true if there are no items for sale
	bool GetNothingForSale();

private:
	// read from defs and map to initialze the shop
	void LoadShopItemDefinitions();

	// grayman (#2376) - put inv_map_start entities in the Starting Items list
	void AddMapItems(idMapFile* mapFile);

	// grayman (#2376) - check for individual lockpicks
	void CheckPicks(ShopItemList& list);

	// scroll a list to the next "page" of values, returns the new top index
	int ScrollList(int topItem, int maxItems, ShopItemList& list);

	// Load all data from shop entities and worldspawn of the given map
	void LoadFromMap(idMapFile* mapFile);

	// Load all shop and starting items from the given spawnargs
	void LoadFromDict(const idDict& dict);

	// Copies purchasedItems into startingItems
	void CopyPurchasedIntoStartingEquipment();

	// grayman - Provides max_ammo value
	int GetMaxAmmo(const idStr& weaponName);

	// Adds the gold from the previous mission
	void AddGoldFromPreviousMission();

	// Load loot rules from the given map dict
	void LoadLootRules(const idDict& dict);
};

#endif	/* !__SHOP_H__ */

