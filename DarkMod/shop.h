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
	const char	*id;
	const char	*name;
	const char	*description;
	int			cost;
	const char	*image;
	int			count;
	idEntity	*entity;
	bool		persistent;
	bool		canDrop;
	
public:
	CShopItem(const char *id, 
			  const char *name, 
			  const char *description, 
			  int cost,
			  const char *image, 
			  int count, 
			  bool persistent = false, 
			  idEntity *entity = NULL, 
			  bool canDrop = true);

	CShopItem(const CShopItem& item, 
			  int count, 
			  int cost = 0, 
			  bool persistent = false);

	// unique identifier for this item
	const char *GetID() const;

	// name of the item (for display)
	const char *GetName() const;

	// description of the item (for display)
	const char *GetDescription() const;

	// cost of the item
	int GetCost( void );	

	// modal name (for displaying) (TODO)
	const char *GetImage() const;

	// number of the items in this collection (number for sale,
	// or number user has bought, or number user started with)
	int GetCount( void );				

	// whether the item can be carried to the next mission
	bool GetPersistent(void);

	// whether the item can dropped by the player from the starting items list
	bool GetCanDrop(void);
	void SetCanDrop(bool canDrop);

	// existing entity for this item
	idEntity *GetEntity( void );				

	// modifies number of items
	void ChangeCount( int amount );				
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
	bool			nothingForSale;

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
	int AddItems(const idDict& mapDict, const char* itemKey, ShopItemList& list);

	// returns the various lists
	ShopItemList& GetItemsForSale();
	ShopItemList& GetStartingItems();
	ShopItemList& GetPurchasedItems();

	// returns the combination of For Sale and Starting items
	ShopItemList GetPlayerItems();

	// adjust the lists
	void SellItem(int index);
	void BuyItem(int index);
	void DropItem(int index);

	// update the GUI variables to match the shop
	void UpdateGUI(idUserInterface* gui);

	// find items based on the id
	CShopItemPtr FindPurchasedByID(const char *id);
	CShopItemPtr FindForSaleByID(const char *id);

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
};


#endif	/* !__SHOP_H__ */

