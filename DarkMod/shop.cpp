#include "../idlib/precompiled.h"
#pragma hdrstop

#include "shop.h"
#include "../game/Game_local.h"

// type-in field for map name (temporary)
idCVar tdm_mapName( "tdm_mapName", "", CVAR_GUI, "" );

CShopItem::CShopItem(const char *id, const char *name, const char *description, int cost, const char *image, int count) {
	this->id = id;
	this->name = name;
	this->description = description;
	this->cost = cost;
	this->image = image;
	this->count = count;
};

CShopItem::CShopItem(CShopItem* item, int count, int cost) {
	this->id = item->id;
	this->name = item->name;
	this->description = item->description;
	this->cost = cost == 0 ? item->cost : cost;
	this->image = item->image;
	this->count = count;
};

const char *CShopItem::GetID(void) const {
	return this->id;
};

const char *CShopItem::GetName(void) const {
	return this->name;
};

const char *CShopItem::GetDescription(void) const {
	return this->description;
};

const char *CShopItem::GetImage(void) const {
	return this->image;
};

int CShopItem::GetCost(void) {
	return this->cost;
};

int CShopItem::GetCount(void) {
	return this->count;
};

void CShopItem::ChangeCount(int amount) {
	this->count += amount;
};

CShop::CShop() {
};

void CShop::Init() {
	itemsForSale.Clear();
	itemsPurchased.Clear();
	startingItems.Clear();
	itemDefs.Clear();
	forSaleTop = 0;
	purchasedTop = 0;
	startingTop = 0;
}

void CShop::AddItemForSale(CShopItem* CShopItem) {
	itemsForSale.Append(CShopItem);
};

void CShop::AddStartingItem(CShopItem* CShopItem) {
	startingItems.Append(CShopItem);
};

idList<CShopItem *>* CShop::GetItemsForSale() {
	return &itemsForSale;
}

idList<CShopItem *>* CShop::GetStartingItems() {
	return &startingItems;
}

idList<CShopItem *>* CShop::GetPurchasedItems() {
	return &itemsPurchased;
}

/**
 * Combine the purchased list and the starting list
 */
idList<CShopItem *>* CShop::GetPlayerItems() {
	idList<CShopItem *>* playerItems = new idList<CShopItem *>(itemsPurchased);
	for (int i = 0; i < startingItems.Num(); i++)
	{
		CShopItem* item = FindPurchasedByID(startingItems[i]->GetID());
		if (item == NULL) {
			playerItems->Append(startingItems[i]);
		} else {
			item->ChangeCount(startingItems[i]->GetCount());
		}
	}
	return playerItems;
}

/**
 * Handle Main Menu commands
 */
void CShop::HandleCommands(const char *menuCommand, idUserInterface *gui) {
	if (stricmp(menuCommand, "shopLoad") == 0)
	{
		// Initialize the shop
		Init();
		DisplayShop(gui);
	}
	else if (stricmp(menuCommand, "shopBuy") == 0)
	{
		// Buy an item
		int boughtItem = gui->GetStateInt("boughtItem", "0");
		BuyItem(boughtItem);
	}
	else if (stricmp(menuCommand, "shopSold") == 0)
	{
		// Return an item to the shelf
		int soldItem = gui->GetStateInt("soldItem", "0");
		SellItem(soldItem);
	}
	else if (stricmp(menuCommand, "shopDrop") == 0)
	{
		// Drop one of the starting items
		int dropItem = gui->GetStateInt("dropItem", "0");
		DropItem(dropItem);
	}
	else if (stricmp(menuCommand, "shopMore") == 0)
	{
		const char * listName = gui->GetStateString("moreList", "");
		if (stricmp(listName, "forSale") == 0) {	
			ScrollList(&forSaleTop, LIST_SIZE_FOR_SALE, &itemsForSale);
		} else if (stricmp(listName, "starting") == 0) {	
			ScrollList(&startingTop, LIST_SIZE_STARTING, &startingItems);
		} else if (stricmp(listName, "purchased") == 0) {	
			ScrollList(&purchasedTop, LIST_SIZE_PURCHASED, &itemsPurchased);
		}
	}
	else if (stricmp(menuCommand, "shopDone") == 0)
	{
		// nothing to do here
	}
	UpdateGUI(gui);
}

void CShop::ScrollList(int* topItem, int maxItems, idList<CShopItem *>* list) {
	if (*topItem + maxItems < list->Num()) {
		*topItem += maxItems;
	} else {
		*topItem = 0;
	}
}

void CShop::LoadShopItemDefinitions() {
	// Load the definitions for the shop items. Include classname (for spawing),
	// display name and description, modal name (for image display), and cost
	int numDecls = declManager->GetNumDecls( DECL_ENTITYDEF );
	for (int i = 0; i < numDecls; i++) {
		const idDecl * decl = declManager->DeclByIndex( DECL_ENTITYDEF, i, false );
		idStr name = idStr(decl->GetName());
		if (name.Icmpn("ShopItem", 8) == 0) {
			const idDecl * shopDecl = declManager->DeclByIndex( DECL_ENTITYDEF, i, true );
			const idDeclEntityDef *entityDef = static_cast<const idDeclEntityDef *>( shopDecl );
			const char* displayName = entityDef->dict.GetString("displayName", "");
			const char* displayDesc = entityDef->dict.GetString("displayDesc", "");
			const char* itemClassname = entityDef->dict.GetString("itemClassname", "");
			const char* image = entityDef->dict.GetString("image", "");
			int cost = entityDef->dict.GetInt("price", "0");
			CShopItem* theItem = new CShopItem(itemClassname, displayName, displayDesc, cost, image, 0);
			itemDefs.Append(theItem);
		}
	}
}

void CShop::AddItems(idDict* mapDict, char* itemKey, idList<CShopItem *>* list) {
	int itemNum = 1;
	while (true) {
		const char* itemName = mapDict->GetString(va("%s_%d_item",itemKey,itemNum));
		if (strlen(itemName) == 0) {
			break;
		}
		// look for skill-specific quantity first
		int quantity = mapDict->GetInt(va("%s_%d_%d_qty",itemKey,itemNum,g_skill.GetInteger()));
		if (quantity == 0) {
			quantity = mapDict->GetInt(va("%s_%d_qty",itemKey,itemNum));
		}
		// look for skill-specific price first
		int price = mapDict->GetInt(va("%s_%d_%d_price",itemKey,itemNum,g_skill.GetInteger()));
		if (price == 0) {
			price = mapDict->GetInt(va("%s_%d_price",itemKey,itemNum));
		}
		// put the items in the shop
		if (quantity > 0) {
			for (int i = 0; i < itemDefs.Num(); i++) {
				if (strcmp(itemDefs[i]->GetID(), itemName) == 0) {
					CShopItem* anItem = new CShopItem(itemDefs[i], quantity, price);
					list->Append(anItem);
				}
			}
		}
		itemNum++;
	}
}

void CShop::DisplayShop(idUserInterface *gui) {
	const char * mapName = tdm_mapName.GetString();
	const char * filename = va("maps/%s", mapName);

	idMapFile* mapFile = new idMapFile;
	if ( !mapFile->Parse( idStr( filename ) + ".map") ) {
		delete mapFile;
		mapFile = NULL;
		gameLocal.Warning( "Couldn't load %s", filename );
		return;
	}
	const char* mapFileName = mapFile->GetName();
	idMapEntity* mapEnt = mapFile->GetEntity( 0 );
	idDict mapDict = mapEnt->epairs;

	gui->SetStateInt("isShopMenuVisible", 1);
	gui->SetStateInt("isNewGameRootMenuVisible", 0);
	gui->SetStateString("mapStartCmd", va("exec 'map %s'", mapName));

	// get list of all items that can be sold
	LoadShopItemDefinitions();

	// get the starting information from spawnargs on the worldspawn in the map

	// the starting gold
	int gold = mapDict.GetInt("shop_gold_start", "0");
	SetGold(gold);

	// items for sale
	AddItems(&mapDict, "shopItem", GetItemsForSale());

	// starting items (items that player already has
	AddItems(&mapDict, "startingItem", GetStartingItems());

	UpdateGUI(gui);
}

void CShop::SellItem(int index) {
	CShopItem* boughtItem = itemsPurchased[purchasedTop + index];
	CShopItem* forSaleItem = FindForSaleByID(boughtItem->GetID());
	boughtItem->ChangeCount(-1);

	// If last in the purchased items list, remove it from the list
	if (boughtItem->GetCount() == 0)
	{
		itemsPurchased.RemoveIndex(purchasedTop + index);
		// scroll so appropriate items visible
		if ((purchasedTop >= itemsPurchased.Num()) || (purchasedTop % LIST_SIZE_PURCHASED != 0)) {
			purchasedTop = max(itemsPurchased.Num() - LIST_SIZE_PURCHASED, 0);
		}
	}
	ChangeGold(boughtItem->GetCost());

	// If the weapon class wasn't in the for sale list (it should be), add it
	if (forSaleItem == NULL)
	{
		forSaleItem = new CShopItem(boughtItem->GetID(), boughtItem->GetName(), boughtItem->GetDescription(), boughtItem->GetCost(), boughtItem->GetImage(), 0);
		itemsForSale.Append(forSaleItem);
	}
	forSaleItem->ChangeCount(1);
};

CShopItem* CShop::FindPurchasedByID(const char *id) {
	return FindByID(&itemsPurchased, id);
}

CShopItem* CShop::FindForSaleByID(const char *id) {
	return FindByID(&itemsForSale, id);
}

CShopItem* CShop::FindByID(idList<CShopItem *>* items, const char *id) {
	for (int i = 0; i < items->Num(); i++)
	{
		CShopItem* item = (*items)[i];
		if (item->GetID() == id)
		{
			return item;
		}
	}
	return NULL;
}

void CShop::BuyItem(int index) {
	CShopItem* forSaleItem = itemsForSale[forSaleTop + index];
	CShopItem* boughtItem = FindPurchasedByID(forSaleItem->GetID());
	forSaleItem->ChangeCount(-1);
	ChangeGold(-(forSaleItem->GetCost()));

	// if the weapon class wasn't in the purchased item list, add it
	if (boughtItem == NULL)
	{
		boughtItem = new CShopItem(forSaleItem->GetID(), forSaleItem->GetName(), forSaleItem->GetDescription(), forSaleItem->GetCost(), forSaleItem->GetImage(), 0);
		itemsPurchased.Append(boughtItem);
		// scroll so new item is visible in purchased list
		if (itemsPurchased.Num() > purchasedTop + LIST_SIZE_PURCHASED) {
			purchasedTop = itemsPurchased.Num() - LIST_SIZE_PURCHASED;
		}
	}
	boughtItem->ChangeCount(1);
};

void CShop::DropItem(int index) {
	CShopItem* dropItem = startingItems[startingTop + index];
	dropItem->ChangeCount(-1);
};

void CShop::ChangeGold(int amount) {
	this->gold += amount;
}

void CShop::SetGold(int gold) {
	this->gold = gold;
}

int CShop::GetGold(void) {
	return this->gold;
};

/**
 * Update the GUI variables. This will change when we get the real GUI.
 */
void CShop::UpdateGUI(idUserInterface* gui) {
	gui->SetStateInt("boughtItem", -1);
	gui->SetStateInt("soldItem", -1);
	gui->SetStateInt("dropItem", -1);
	gui->SetStateInt("gold", gold);
	gui->SetStateInt("forSaleMoreVisible", itemsForSale.Num() > LIST_SIZE_FOR_SALE);
	gui->SetStateInt("purchasedMoreVisible", itemsPurchased.Num() > LIST_SIZE_PURCHASED);
	gui->SetStateInt("startingMoreVisible", startingItems.Num() > LIST_SIZE_STARTING);
	for (int i = 0; i < LIST_SIZE_FOR_SALE; i++) {
		idStr guiCost = idStr("forSaleCost") + i + "_cost";
		idStr guiName = idStr("forSale") + i + "_name";
		idStr guiDesc = idStr("forSale") + i + "_desc";
		idStr guiAvailable = idStr("forSaleAvail") + i;
		idStr name = idStr("");
		idStr desc = idStr("");
		idStr cost = idStr("");
		int available = 0;
		if (forSaleTop + i < itemsForSale.Num()) {
			CShopItem* item = itemsForSale[forSaleTop + i];
			name = idStr(item->GetName()) + " (" + item->GetCount() + ")";
			desc = idStr(item->GetName()) + ": " + item->GetDescription();
			available = item->GetCost() <= gold ? item->GetCount() : 0;
			cost = idStr(item->GetCost()) + " GP";
		}
		gui->SetStateString(guiCost, cost);
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
	}

	for (int i = 0; i < LIST_SIZE_PURCHASED; i++) {
		idStr guiCost = idStr("boughtCost") + i + "_cost";
		idStr guiName = idStr("bought") + i + "_name";
		idStr guiDesc = idStr("bought") + i + "_desc";
		idStr guiAvailable = idStr("boughtAvail") + i;
		idStr name = idStr("");
		idStr desc = idStr("");
		idStr cost = idStr("");
		int available = 0;
		if (purchasedTop + i < itemsPurchased.Num()) {
			CShopItem* item = itemsPurchased[purchasedTop + i];
			name = idStr(item->GetName()) + " (" + item->GetCount() + ")";
			desc = idStr(item->GetName()) + ": " + item->GetDescription();
			available = item->GetCost() <= gold ? item->GetCount() : 0;
			cost = idStr(item->GetCost()) + " GP";
		}
		gui->SetStateString(guiCost, cost);
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
	}

	for (int i = 0; i < LIST_SIZE_STARTING; i++) {
		idStr guiName = idStr("starting") + i + "_name";
		idStr guiDesc = idStr("starting") + i + "_desc";
		idStr guiAvailable = idStr("startingAvail") + i;
		idStr guiDrop = idStr("dropVisible") + i;
		idStr name = idStr("");
		idStr desc = idStr("");
		int available = 0;
		int dropVisible = 0;
		if (startingTop + i < startingItems.Num()) {
			CShopItem* item = startingItems[startingTop + i];
			name = idStr(item->GetName()) + " (" + item->GetCount() + ")";
			desc = idStr(item->GetName()) + ": " + item->GetDescription();
			available = item->GetCost() <= gold ? item->GetCount() : 0;
			dropVisible = 1;
		}
		gui->SetStateInt(guiDrop, dropVisible);
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
	}
}

