#include "../idlib/precompiled.h"
#pragma hdrstop

#include "shop.h"
#include "../game/game_local.h"
#include "MissionData.h"
#include "./Inventory/Inventory.h"

CShopItem::CShopItem() :
	id(""),
	name(""),
	description(""),
	cost(0),
	image(""),
	count(0),
	entity(NULL),
	persistent(false),
	canDrop(false)
{}

CShopItem::CShopItem(const idStr& _id, const idStr& _name, const idStr& _description,
					 int _cost, const idStr& _image, int _count, bool _persistent, idEntity* _entity, bool _canDrop) :
	id(_id),
	name(_name),
	description(_description),
	cost(_cost),
	image(_image),
	count(_count),
	entity(_entity),
	persistent(_persistent),
	canDrop(_canDrop)
{}

CShopItem::CShopItem(const CShopItem& item, int _count, int _cost, bool _persistent) :
	id(item.id),
	name(item.name),
	description(item.description),
	cost(_cost == 0 ? item.cost : _cost),
	image(item.image),
	count(_count),
	entity(item.entity),
	persistent(_persistent == false ? item.persistent : _persistent),
	canDrop(item.canDrop)
{}

const idStr& CShopItem::GetID() const {
	return this->id;
}

const idStr& CShopItem::GetName() const {
	return this->name;
}

const idStr& CShopItem::GetDescription() const {
	return this->description;
}

const idStr& CShopItem::GetImage() const {
	return this->image;
}

int CShopItem::GetCost() {
	return this->cost;
}

int CShopItem::GetCount() {
	return this->count;
}

bool CShopItem::GetPersistent() {
	return this->persistent;
}

bool CShopItem::GetCanDrop() {
	return this->canDrop;
}

void CShopItem::SetCanDrop(bool canDrop) {
	this->canDrop = canDrop;
}

idEntity *CShopItem::GetEntity() {
	return this->entity;
}

void CShopItem::ChangeCount(int amount) {
	this->count += amount;
}

void CShopItem::Save(idSaveGame *savefile) const
{
	savefile->WriteString(id);
	savefile->WriteString(name);
	savefile->WriteString(description);

	savefile->WriteInt(cost);
	savefile->WriteString(image);
	savefile->WriteInt(count);
	savefile->WriteObject(entity);
	savefile->WriteBool(persistent);
	savefile->WriteBool(canDrop);
}

void CShopItem::Restore(idRestoreGame *savefile)
{
	savefile->ReadString(id);
	savefile->ReadString(name);
	savefile->ReadString(description);

	savefile->ReadInt(cost);
	savefile->ReadString(image);
	savefile->ReadInt(count);
	savefile->ReadObject(reinterpret_cast<idClass*&>(entity));
	savefile->ReadBool(persistent);
	savefile->ReadBool(canDrop);
}

// ================= Shop ============================

void CShop::Init()
{
	Clear();
}

void CShop::Clear()
{
	itemsForSale.Clear();
	itemsPurchased.Clear();
	startingItems.Clear();
	itemDefs.Clear();
	forSaleTop = 0;
	purchasedTop = 0;
	startingTop = 0;
	skipShop = false;
}

void CShop::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(itemDefs.Num());
	for (int i = 0; i < itemDefs.Num(); ++i)
	{
		itemDefs[i]->Save(savefile);
	}

	savefile->WriteInt(itemsForSale.Num());
	for (int i = 0; i < itemsForSale.Num(); ++i)
	{
		itemsForSale[i]->Save(savefile);
	}

	savefile->WriteInt(itemsPurchased.Num());
	for (int i = 0; i < itemsPurchased.Num(); ++i)
	{
		itemsPurchased[i]->Save(savefile);
	}

	savefile->WriteInt(startingItems.Num());
	for (int i = 0; i < startingItems.Num(); ++i)
	{
		startingItems[i]->Save(savefile);
	}
	
	savefile->WriteInt(gold);
	savefile->WriteInt(forSaleTop);
	savefile->WriteInt(purchasedTop);
	savefile->WriteInt(startingTop);

	savefile->WriteBool(skipShop);
}

void CShop::Restore(idRestoreGame *savefile)
{
	int num;

	savefile->ReadInt(num);
	itemDefs.SetNum(num);
	for (int i = 0; i < num; ++i)
	{
		itemDefs[i] = CShopItemPtr(new CShopItem);
		itemDefs[i]->Restore(savefile);
	}

	savefile->ReadInt(num);
	itemsForSale.SetNum(num);
	for (int i = 0; i < num; ++i)
	{
		itemsForSale[i] = CShopItemPtr(new CShopItem);
		itemsForSale[i]->Restore(savefile);
	}

	savefile->ReadInt(num);
	itemsPurchased.SetNum(num);
	for (int i = 0; i < num; ++i)
	{
		itemsPurchased[i] = CShopItemPtr(new CShopItem);
		itemsPurchased[i]->Restore(savefile);
	}

	savefile->ReadInt(num);
	startingItems.SetNum(num);
	for (int i = 0; i < num; ++i)
	{
		startingItems[i] = CShopItemPtr(new CShopItem);
		startingItems[i]->Restore(savefile);
	}

	savefile->ReadInt(gold);
	savefile->ReadInt(forSaleTop);
	savefile->ReadInt(purchasedTop);
	savefile->ReadInt(startingTop);

	savefile->ReadBool(skipShop);
}

void CShop::AddItemForSale(const CShopItemPtr& shopItem) {
	itemsForSale.Append(shopItem);
};

void CShop::AddStartingItem(const CShopItemPtr& shopItem) {
	startingItems.Append(shopItem);
};

const ShopItemList& CShop::GetItemsForSale()
{
	return itemsForSale;
}

const ShopItemList& CShop::GetStartingItems()
{
	return startingItems;
}

const ShopItemList& CShop::GetPurchasedItems()
{
	return itemsPurchased;
}

bool CShop::GetNothingForSale()
{
	return itemsForSale.Num() == 0;
}

/**
 * Combine the purchased list and the starting list
 */
ShopItemList CShop::GetPlayerItems()
{
	// Copy-construct the list using the list of purchased items
	ShopItemList playerItems(itemsPurchased);

	for (int i = 0; i < startingItems.Num(); i++)
	{
		CShopItemPtr item = FindPurchasedByID(startingItems[i]->GetID());

		if (item == NULL)
		{
			playerItems.Append(startingItems[i]);
		} 
		else
		{
			item->ChangeCount(startingItems[i]->GetCount());
		}
	}

	return playerItems;
}

/**
 * Handle Main Menu commands
 */
void CShop::HandleCommands(const char *menuCommand, idUserInterface *gui, idPlayer *player)
{
	if (idStr::Icmp(menuCommand, "shopLoad") == 0)
	{
		// Clear out the shop
		Init();

		// get list of all items that can be sold
		LoadShopItemDefinitions();

		// load persistent items into Starting Items list
		LoadFromInventory(player);

		// init and update the shop GUI
		DisplayShop(gui);
	}
	else if (idStr::Icmp(menuCommand, "shopBuy") == 0)
	{
		// Buy an item
		int boughtItem = gui->GetStateInt("boughtItem", "0");
		BuyItem(boughtItem);
		UpdateGUI(gui);
	}
	else if (idStr::Icmp(menuCommand, "shopSold") == 0)
	{
		// Return an item to the shelf
		int soldItem = gui->GetStateInt("soldItem", "0");
		SellItem(soldItem);
		UpdateGUI(gui);
	}
	else if (idStr::Icmp(menuCommand, "shopDrop") == 0)
	{
		// Drop one of the starting items
		int dropItem = gui->GetStateInt("dropItem", "0");
		DropItem(dropItem);
		UpdateGUI(gui);
	}
	else if (idStr::Icmp(menuCommand, "shopMore") == 0)
	{
		const char * listName = gui->GetStateString("moreList", "");

		if (idStr::Icmp(listName, "forSale") == 0)
		{
			ScrollList(&forSaleTop, LIST_SIZE_FOR_SALE, itemsForSale);
		} 
		else if (idStr::Icmp(listName, "starting") == 0)
		{
			ScrollList(&startingTop, LIST_SIZE_STARTING, startingItems);
		} 
		else if (idStr::Icmp(listName, "purchased") == 0)
		{
			ScrollList(&purchasedTop, LIST_SIZE_PURCHASED, itemsPurchased);
		}

		UpdateGUI(gui);
	}
	else if (idStr::Icmp(menuCommand, "shopDone") == 0)
	{
		// nothing to do here
	}
}

void CShop::ScrollList(int* topItem, int maxItems, ShopItemList& list)
{
	if (*topItem + maxItems < list.Num())
	{
		*topItem += maxItems;
	}
	else {
		*topItem = 0;
	}
}

void CShop::LoadFromInventory(idPlayer *player) {
	if (player == NULL)
	{
		return;
	}

	int count = 0;

	for (int catNum = 0; catNum < player->Inventory()->GetNumCategories(); catNum++)
	{
		CInventoryCategoryPtr cat = player->Inventory()->GetCategory(catNum);

		for (int itemNum = 0; itemNum < cat->GetNumItems(); itemNum++)
		{
			CInventoryItemPtr it = cat->GetItem(itemNum);
			if ((count = it->GetPersistentCount()) > 0)
			{
				idEntity * itemEntity = it->GetItemEntity();
				const char * name = itemEntity->spawnArgs.GetString("classname");

				CShopItemPtr shopItem = FindByID(itemDefs, name);

				if (shopItem != NULL)
				{
					CShopItemPtr item(new CShopItem(*shopItem, count, 0, true));
					startingItems.Append(item);
				}
			}
		}
	}
}

void CShop::LoadFromDict(const idDict& dict)
{
	// greebo: Assemble the difficulty prefix (e.g. "diff_0_")
	idStr diffPrefix = "diff_" + idStr(gameLocal.m_DifficultyManager.GetDifficultyLevel()) + "_";

	if (dict.GetBool("shop_skip","0") || dict.GetBool(diffPrefix + "shop_skip","0"))
	{
		// if skip flag is set, skip the shop
		skipShop = true;

		// No need to parse any further, the shop will be skipped anyway
		return;
	}

	// Check for an "all-difficulty" gold value
	if (dict.FindKey("shop_gold_start") != NULL)
	{
		SetGold(dict.GetInt("shop_gold_start"));
	}

	// Try to retrieve the starting gold for the given difficulty level
	if (dict.FindKey(diffPrefix + "shop_gold_start") != NULL)
	{
		SetGold(dict.GetInt(diffPrefix + "shop_gold_start"));
	}

	// items for sale
	AddItems(dict, "shopItem", itemsForSale);

	// starting items (items that player already has
	AddItems(dict, "startingItem", startingItems);
}

void CShop::LoadFromMap(idMapFile* mapFile)
{
	// Get the worldspawn entity
	idMapEntity* worldspawn = mapFile->GetEntity(0);

	// Load shop data from worldspawn first
	LoadFromDict(worldspawn->epairs);

	// Check the rest of the map entities for shop entityDefs
	for (int i = 1; i < mapFile->GetNumEntities(); ++i)
	{
		idMapEntity* mapEnt = mapFile->GetEntity(i);

		if (idStr::Icmp(mapEnt->epairs.GetString("classname"), "atdm:shop") == 0)
		{
			// Found a shop entity, process its spawnargs
			LoadFromDict(mapEnt->epairs);
		}
	}
}

void CShop::LoadShopItemDefinitions()
{
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

			CShopItemPtr theItem(new CShopItem(itemClassname, displayName, displayDesc, cost, image, 0));
			itemDefs.Append(theItem);
		}
	}
}

int CShop::AddItems(const idDict& mapDict, const idStr& itemKey, ShopItemList& list)
{
	int diffLevel = gameLocal.m_DifficultyManager.GetDifficultyLevel();
	
	int itemsAdded = 0;

	for (const idKeyValue* kv = mapDict.MatchPrefix(itemKey); kv != NULL; kv = mapDict.MatchPrefix(itemKey, kv))
	{
		// Inspect the matching prefix, check whether the difficulty level applies
		idStr postfix = kv->GetKey();

		// Cut off the prefix including the following underscore _
		postfix.StripLeadingOnce(itemKey + "_");
		
		int pos = postfix.Find("_item");
		
		if (pos == -1 || pos != postfix.Length() - 5)
		{
			continue; // no suitable "_item" found
		}

		// This is the number portion, like "1_2" or merely "2"
		idStr indexStr = postfix.Mid(0, pos);

		// Check if we have still an underscore in the index string, this implies
		// that there is a difficulty number included
		int underScorePos = indexStr.Find('_');

		// Extract the item index
		int itemIndex = (underScorePos != -1) ? atoi(indexStr.Mid(0, underScorePos)) : atoi(indexStr);

		if (underScorePos != -1)
		{
			// Check out the second number, this is the difficulty level
			idStr diffStr = indexStr.Mid(underScorePos + 1, indexStr.Length() - underScorePos);

			// Check if the difficulty level matches
			if (atoi(diffStr) != diffLevel)
			{
				// Ignore this spawnarg
				continue;
			}
		}

		idStr itemName = kv->GetValue();

		if (itemName.IsEmpty())
		{
			continue; // Empty names are not considered
		}

		// greebo: Assemble the item prefix (e.g. "shopItem_1_") to look up the rest of the spawnargs
		idStr itemPrefix = itemKey + "_" + idStr(itemIndex);
		idStr diffLevelStr = "_" + idStr(diffLevel);

		// look for quantity, but let a difficulty-specific setting override the general one
		int quantity = mapDict.GetInt(itemPrefix + "_qty");

		if (mapDict.FindKey(itemPrefix + diffLevelStr + "_qty") != NULL)
		{
			quantity = mapDict.GetInt(itemPrefix + diffLevelStr + "_qty");
		}

		// look for price
		int price = mapDict.GetInt(itemPrefix + "_price");

		if (mapDict.FindKey(itemPrefix + diffLevelStr + "_price") != NULL)
		{
			price = mapDict.GetInt(itemPrefix + diffLevelStr + "_price");
		}

		// look for persistency
		bool persistent = mapDict.GetBool(itemPrefix + "_persistent");

		if (mapDict.FindKey(itemPrefix + diffLevelStr + "_persistent") != NULL)
		{
			persistent = mapDict.GetBool(itemPrefix + diffLevelStr + "_persistent");
		}

		// look for canDrop flag 
		bool canDrop = mapDict.GetBool(itemPrefix + "_canDrop", "1"); // items can be dropped by default

		if (mapDict.FindKey(itemPrefix + diffLevelStr + "_canDrop") != NULL)
		{
			canDrop = mapDict.GetBool(itemPrefix + diffLevelStr + "_canDrop", "1");
		}

		// put the item in the shop
		if (quantity > 0)
		{
			CShopItemPtr found = FindByID(itemDefs, itemName);

			if (found != NULL) 
			{
				CShopItemPtr anItem(new CShopItem(*found, quantity, price, persistent));
				anItem->SetCanDrop(canDrop);
				list.Append(anItem);

				itemsAdded++;
			}
			else
			{
				gameLocal.Printf("Could not add item to shop: %s\n", itemName.c_str());
			}
		}
	}

	return itemsAdded;
}

void CShop::DisplayShop(idUserInterface *gui)
{
	idStr filename = va("maps/%s", cv_tdm_mapName.GetString());

	// Let the GUI know which map to load
	gui->SetStateString("mapStartCmd", va("exec 'map %s'", cv_tdm_mapName.GetString()));

	// Load the map from the missiondata class (provides cached loading)
	idMapFile* mapFile = gameLocal.m_MissionData->LoadMap(filename);

	if (mapFile == NULL)
	{
		// Couldn't load map
		gui->HandleNamedEvent("SkipShop");

		gameLocal.Warning("Couldn't load map %s, skipping shop.", filename.c_str());
		return;
	}

	// Load the shop items from the map entity/entities
	LoadFromMap(mapFile);

	if (skipShop)
	{
		// Shop data says: skip da shoppe
		gui->HandleNamedEvent("SkipShop");
		return;
	}

	UpdateGUI(gui);
}

void CShop::SellItem(int index)
{
	CShopItemPtr boughtItem = itemsPurchased[purchasedTop + index];
	CShopItemPtr forSaleItem = FindForSaleByID(boughtItem->GetID());
	boughtItem->ChangeCount(-1);

	// If last in the purchased items list, remove it from the list
	if (boughtItem->GetCount() == 0)
	{
		itemsPurchased.RemoveIndex(purchasedTop + index);
		// scroll so appropriate items visible
		if ((purchasedTop >= itemsPurchased.Num()) || (purchasedTop % LIST_SIZE_PURCHASED != 0)) {
			purchasedTop = itemsPurchased.Num() - LIST_SIZE_PURCHASED;
			if (purchasedTop < 0) purchasedTop = 0;
		}
	}

	ChangeGold(boughtItem->GetCost());

	// If the weapon class wasn't in the for sale list (it should be), add it
	if (forSaleItem == NULL)
	{
		forSaleItem = CShopItemPtr(new CShopItem(
			boughtItem->GetID(), 
			boughtItem->GetName(),
			boughtItem->GetDescription(), 
			boughtItem->GetCost(), 
			boughtItem->GetImage(), 
			0
		));

		itemsForSale.Append(forSaleItem);
	}

	forSaleItem->ChangeCount(1);
};

CShopItemPtr CShop::FindPurchasedByID(const char *id)
{
	return FindByID(itemsPurchased, id);
}

CShopItemPtr CShop::FindForSaleByID(const char *id) {
	return FindByID(itemsForSale, id);
}

CShopItemPtr CShop::FindByID(ShopItemList& items, const char *id)
{
	for (int i = 0; i < items.Num(); i++)
	{
		const CShopItemPtr& item = items[i];

		if (item != NULL && idStr::Icmp(item->GetID(), id) == 0)
		{
			return item;
		}
	}

	return CShopItemPtr();
}

void CShop::BuyItem(int index)
{
	CShopItemPtr forSaleItem = itemsForSale[forSaleTop + index];
	CShopItemPtr boughtItem = FindPurchasedByID(forSaleItem->GetID());

	forSaleItem->ChangeCount(-1);
	ChangeGold(-(forSaleItem->GetCost()));

	// if the weapon class wasn't in the purchased item list, add it
	if (boughtItem == NULL)
	{
		boughtItem = CShopItemPtr(new CShopItem(
			forSaleItem->GetID(), 
			forSaleItem->GetName(), 
			forSaleItem->GetDescription(),
			forSaleItem->GetCost(), 
			forSaleItem->GetImage(), 
			0, 
			forSaleItem->GetPersistent()
		));

		itemsPurchased.Append(boughtItem);

		// scroll so new item is visible in purchased list
		if (itemsPurchased.Num() > purchasedTop + LIST_SIZE_PURCHASED)
		{
			purchasedTop = itemsPurchased.Num() - LIST_SIZE_PURCHASED;
		}
	}

	boughtItem->ChangeCount(1);
};

void CShop::DropItem(int index)
{
	const CShopItemPtr& dropItem = startingItems[startingTop + index];
	dropItem->ChangeCount(-1);
};

void CShop::ChangeGold(int amount)
{
	this->gold += amount;
}

void CShop::SetGold(int gold)
{
	this->gold = gold;
}

int CShop::GetGold()
{
	return this->gold;
};

/**
 * Update the GUI variables. This will change when we get the real GUI.
 */
void CShop::UpdateGUI(idUserInterface* gui)
{
	gui->SetStateInt("boughtItem", -1);
	gui->SetStateInt("soldItem", -1);
	gui->SetStateInt("dropItem", -1);
	gui->SetStateInt("gold", gold);
	gui->SetStateInt("forSaleMoreVisible", itemsForSale.Num() > LIST_SIZE_FOR_SALE);
	gui->SetStateInt("purchasedMoreVisible", itemsPurchased.Num() > LIST_SIZE_PURCHASED);
	gui->SetStateInt("startingMoreVisible", startingItems.Num() > LIST_SIZE_STARTING);

	if (GetNothingForSale())
	{
		// nothing for sale, let the user know
		gui->SetStateInt("forSaleAvail0", 0);
		gui->SetStateString("forSale0_name", "<no items for sale>");
	}
	else
	{
		for (int i = 0; i < LIST_SIZE_FOR_SALE; i++)
		{
			idStr guiCost = idStr("forSaleCost") + i + "_cost";
			idStr guiName = idStr("forSale") + i + "_name";
			idStr guiDesc = idStr("forSale") + i + "_desc";
			idStr guiImage = idStr("forSale") + i + "_image";
			idStr guiAvailable = idStr("forSaleAvail") + i;
			idStr name = idStr("");
			idStr desc = idStr("");
			idStr image = idStr("");
			idStr cost = idStr("");

			int available = 0;

			if (forSaleTop + i < itemsForSale.Num())
			{
				const CShopItemPtr& item = itemsForSale[forSaleTop + i];

				name = item->GetName() + " (" + item->GetCount() + ")";
				desc = item->GetName() + ": " + item->GetDescription();
				image = item->GetImage();
				available = item->GetCost() <= gold ? item->GetCount() : 0;
				cost = idStr(item->GetCost()) + " GP ";
			}

			gui->SetStateString(guiCost, cost);
			gui->SetStateInt(guiAvailable, available);
			gui->SetStateString(guiName, name);
			gui->SetStateString(guiDesc, desc);
			gui->SetStateString(guiImage, image);
		}
	}

	for (int i = 0; i < LIST_SIZE_PURCHASED; i++)
	{
		idStr guiCost = idStr("boughtCost") + i + "_cost";
		idStr guiName = idStr("bought") + i + "_name";
		idStr guiDesc = idStr("bought") + i + "_desc";
		idStr guiImage = idStr("bought") + i + "_image";
		idStr guiAvailable = idStr("boughtAvail") + i;
		idStr name = idStr("");
		idStr desc = idStr("");
		idStr image = idStr("");
		idStr cost = idStr("");

		int available = 0;

		if (purchasedTop + i < itemsPurchased.Num())
		{
			const CShopItemPtr& item = itemsPurchased[purchasedTop + i];
			name = item->GetName() + " (" + item->GetCount() + ")";
			desc = item->GetName() + ": " + item->GetDescription();
			image = item->GetImage();
			available = 1; // sell item is always available
			cost = idStr(item->GetCost()) + " GP ";
		}

		gui->SetStateString(guiCost, cost);
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
		gui->SetStateString(guiImage, image);
	}

	for (int i = 0; i < LIST_SIZE_STARTING; i++)
	{
		idStr guiName = idStr("starting") + i + "_name";
		idStr guiDesc = idStr("starting") + i + "_desc";
		idStr guiImage = idStr("starting") + i + "_image";
		idStr guiAvailable = idStr("startingAvail") + i;
		idStr guiDrop = idStr("dropVisible") + i;
		idStr name = idStr("");
		idStr desc = idStr("");
		idStr image = idStr("");

		int available = 0;
		bool dropVisible = false;

		if (startingTop + i < startingItems.Num())
		{
			const CShopItemPtr& item = startingItems[startingTop + i];
			name = item->GetName() + " (" + item->GetCount() + ")";
			desc = item->GetName() + ": " + item->GetDescription();
			image = item->GetImage();
			available = item->GetCost() <= gold ? item->GetCount() : 0;
			dropVisible = item->GetCanDrop();
		}

		gui->SetStateBool(guiDrop, dropVisible);
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
		gui->SetStateString(guiImage, image);
	}
}

