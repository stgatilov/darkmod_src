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

	// The list of entityDef names to add to the player's inventory 
	// when this shop item is purchased
	idStringList classNames;
	
public:
	CShopItem();

	CShopItem(const idStr& id, 
			  const idStr& name, 
			  const idStr& description, 
			  int cost,
			  const idStr& image, 
			  int count, 
			  bool persistent = false, 
			  bool canDrop = true);

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

	// whether the item can be carried to the next mission
	bool GetPersistent();

	// whether the item can dropped by the player from the starting items list
	bool GetCanDrop();
	void SetCanDrop(bool canDrop);

	// modifies number of items
	void ChangeCount( int amount );

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);
};
typedef boost::shared_ptr<CShopItem> CShopItemPtr;

// A list of shop items
typedef idList<CShopItemPtr> ShopItemList;

// Represents the Shop
class CShop
{
private:
	ShopItemList	itemDefs;
	ShopItemList	itemsForSale;
	ShopItemList	itemsPurchased;
	ShopItemList	startingItems;

	int				gold;
	int				forSaleTop;
	int				purchasedTop;
	int				startingTop;

	// True if the purchase menu should be skipped
	bool			skipShop;

public:
	void Init();

	// Clears out all lists
	void Clear();

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);

	// read from defs and map to initialze the shop
	void LoadShopItemDefinitions();

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
	void DropItem(int index);

	// update the GUI variables to match the shop
	void UpdateGUI(idUserInterface* gui);

	// find items based on the id
	CShopItemPtr FindPurchasedByID(const char *id);
	CShopItemPtr FindForSaleByID(const char *id);
	CShopItemPtr FindStartingItemByID(const char *id);

	CShopItemPtr FindByID(ShopItemList& items, const char *id);

	// initialize the shop
	void DisplayShop(idUserInterface *gui);

	// scroll a list to the next "page" of values
	void ScrollList(int* topItem, int maxItems, ShopItemList& list);

	// true if there are no items for sale
	bool GetNothingForSale();

private:
	// Load all data from shop entities and worldspawn of the given map
	void LoadFromMap(idMapFile* mapFile);

	// Load all shop and starting items from the given spawnargs
	void LoadFromDict(const idDict& dict);

	// Copies purchasedItems into startingItems
	void CopyPurchasedIntoStartingEquipment();
};


#endif	/* !__SHOP_H__ */

