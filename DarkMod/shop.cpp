// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "shop.h"
#include "../game/game_local.h"
#include "MissionData.h"
#include "Missions/MissionManager.h"
#include "./Inventory/Inventory.h"

CShopItem::CShopItem() :
	id(""),
	name(""),
	description(""),
	cost(0),
	image(""),
	count(0),
	persistent(false),
	canDrop(false),
	dropped(0),			// tels (#2567) remember how many we dropped	
	stackable(false)	// grayman (#2376)
{}

CShopItem::CShopItem(const idStr& _id, const idStr& _name, const idStr& _description,
					 int _cost, const idStr& _image, int _count, bool _persistent, bool _canDrop, bool _stackable) : // grayman (#2376)
	id(_id),
	name(_name),
	description(_description),
	cost(_cost),
	image(_image),
	count(_count),
	persistent(_persistent),
	canDrop(_canDrop),
	dropped(0),
	stackable(_stackable) // grayman (#2376)
{}

CShopItem::CShopItem(const CShopItem& item, int _count, int _cost, bool _persistent) :
	id(item.id),
	name(item.name),
	description(item.description),
	cost(_cost == 0 ? item.cost : _cost),
	image(item.image),
	count(_count),
	persistent(_persistent == false ? item.persistent : _persistent),
	canDrop(item.canDrop),
	dropped(0),
	classNames(item.classNames),
	stackable(item.stackable) // grayman (#2376)
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

const idStringList& CShopItem::GetClassnames() const
{
	return classNames;
}

void CShopItem::AddClassname(const idStr& className)
{
	classNames.AddUnique(className);
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

int CShopItem::GetDroppedCount() {
	return this->dropped;
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

// tels (#2567)
void CShopItem::Drop(void) {
	if (this->canDrop && this->count > 0)
	{
		this->dropped = this->count;
		this->count = 0;
	}
}

// tels (#2567)
void CShopItem::Undrop(void) {
	if (this->canDrop && this->dropped > 0)
	{
		this->count = this->dropped;
		this->dropped = 0;
	}
}

// grayman (#2376) - add stackable methods
bool CShopItem::GetStackable() {
	return this->stackable;
}

void CShopItem::SetStackable(bool stackable) {
	this->stackable = stackable;
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
	savefile->WriteInt(dropped);
	savefile->WriteBool(persistent);
	savefile->WriteBool(canDrop);

	savefile->WriteInt(classNames.Num());
	for (int i = 0; i < classNames.Num(); ++i)
	{
		savefile->WriteString(classNames[i]);
	}
	savefile->WriteBool(stackable); // grayman (#2376)
}

void CShopItem::Restore(idRestoreGame *savefile)
{
	savefile->ReadString(id);
	savefile->ReadString(name);
	savefile->ReadString(description);

	savefile->ReadInt(cost);
	savefile->ReadString(image);
	savefile->ReadInt(count);
	savefile->ReadInt(dropped);
	savefile->ReadBool(persistent);
	savefile->ReadBool(canDrop);

	int temp;
	savefile->ReadInt(temp);
	classNames.SetNum(temp);
	for (int i = 0; i < temp; ++i)
	{
		savefile->ReadString(classNames[i]);
	}
	savefile->ReadBool(stackable); // grayman (#2376)
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
	pickSetShop = false;     // grayman (#2376) -
	pickSetStarting = false; // Lockpick handling
	gold = 0; // grayman - needs to be initialized
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
ShopItemList CShop::GetPlayerStartingEquipment()
{
	return startingItems;
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

		// refresh the display so items are greyed out
		gui->HandleNamedEvent("UpdateItemColours");
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
	else if (idStr::Icmp(menuCommand, "shopDropUndrop") == 0)
	{
		// Drop one of the starting items
		int dropItem = gui->GetStateInt("dropItem", "0");
		// Decide depending on the item if we should drop or undrop
		DropUndropItem(dropItem);
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
		// The player is done shopping, now set up the starting equipment
		CopyPurchasedIntoStartingEquipment();
	}
}

void CShop::CopyPurchasedIntoStartingEquipment()
{
	for (int i = 0; i < itemsPurchased.Num(); i++)
	{
		CShopItemPtr item = FindStartingItemByID(itemsPurchased[i]->GetID());

		if (item == NULL)
		{
			startingItems.Append(itemsPurchased[i]);
		} 
		else
		{
			// Starting item exists, just change the count
			item->ChangeCount(itemsPurchased[i]->GetCount());
		}
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

	for (int i = 0; i < numDecls; i++)
	{
		const idDecl * decl = declManager->DeclByIndex( DECL_ENTITYDEF, i, false );
		idStr name = idStr(decl->GetName());

		if (name.Icmpn("ShopItem", 8) == 0)
		{
			const idDecl* shopDecl = declManager->DeclByIndex( DECL_ENTITYDEF, i, true );
			const idDeclEntityDef* entityDef = static_cast<const idDeclEntityDef *>( shopDecl );
			const idDict& dict = entityDef->dict;

			const char* displayName = dict.GetString("displayName", "");
			const char* displayDesc = dict.GetString("displayDesc", "");
			const char* itemClassname = dict.GetString("itemClassname", "");
			const char* image = dict.GetString("image", "");
			int cost = dict.GetInt("price", "0");
			bool stackable = dict.GetBool("stackable","0"); // grayman (#2376)

			idStr id = name;
			id.StripLeadingOnce("shopitem_");
			id = "atdm:" + id;

			CShopItemPtr theItem(new CShopItem(id, displayName, displayDesc, cost, image, 0));
			theItem->SetStackable(stackable); // grayman (#2376)

			// Add all "itemClassname*" spawnargs to the list
			for (const idKeyValue* kv = dict.MatchPrefix("itemClassname"); kv != NULL; 
				 kv = dict.MatchPrefix("itemClassname", kv))
			{
				DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Adding class %s to shopitem %s\r", kv->GetValue().c_str(), displayName);
				theItem->AddClassname(kv->GetValue());
			}
			
			itemDefs.Append(theItem);
		}
	}
}

// grayman - provide max_ammo if defined

int CShop::GetMaxAmmo(const idStr& weaponName)
{
	// For now, hard-code a max_ammo limit of 50 for these weapons
	// because it takes too long to query FindEntityDefDict().

	// TODO - move the FindEntityDefDict() calls up in parallel with
	// the briefing if that's possible and drop the hard-coded limit.
	// Then you can query those results from here during shop processing.

	// The blackjack and sword also come through here, and their limit
	// is 1, but that's handled elsewhere, even if we return 50 here.

#if 1
	return 50;
#else
	int max_ammo = 1;
	const idDict* weaponDict = gameLocal.FindEntityDefDict(weaponName);
	if (weaponDict != NULL)
	{
		max_ammo = weaponDict->GetInt("max_ammo", "1");
	}
	return (max_ammo);
#endif
}

int CShop::AddItems(const idDict& mapDict, const idStr& itemKey, ShopItemList& list)
{
	int diffLevel = gameLocal.m_DifficultyManager.GetDifficultyLevel();
	
	int itemsAdded = 0;

	// grayman (#2376)
	// Convert itemKey to lowercase. mapDict methods ignore case, but
	// StripLeadingOnce() doesn't. This change allows recognition of shop items defined as
	// "startingItem_*", "startingitem_*", "shopItem_*", and "shopitem_*.

	idStr itemKeyLower = itemKey;
	itemKeyLower.ToLower();

	bool isShopList = (itemKeyLower.Find("shop") >= 0); // for lockpick checking

	for (const idKeyValue* kv = mapDict.MatchPrefix(itemKeyLower); kv != NULL; kv = mapDict.MatchPrefix(itemKeyLower, kv))
	{
		// Inspect the matching prefix, check whether the difficulty level applies
		idStr postfix = kv->GetKey();
		postfix.ToLower(); // grayman (#2376) convert postfix to lowercase so StripLeadingOnce()
						   // matches lowercase against lowercase

		// Cut off the prefix including the following underscore _
		postfix.StripLeadingOnce(itemKeyLower + "_");
		
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

		// put the item in the shop
		if (quantity > 0)
		{
			// grayman (#2376) - Special handling for weapon quantities.

			int index = itemName.Find("weapon_");
			if (index >= 0)
			{
				// A shop entity should use atdm:weapon_*, but there's at least one
				// that uses weapon_*, so convert the latter to the former.

				idStr weaponName;
				if (index == 0)
				{
					weaponName = "atdm:" + itemName;
				}
				else
				{
					weaponName = itemName;
				}

				// Weapon quantities have limits. (Arrows in particular.)

				int max_ammo = GetMaxAmmo(weaponName);
				quantity = (quantity > max_ammo) ? max_ammo : quantity;
			}

			/* grayman (#2376) - Since a lockpick_set comprises individual picks, putting one
								 on either the shopItems or startingItems list means we don't
								 have to put individual picks on the same list.
								 For now, just register whether a lockpick_set is being added
								 to either list. We'll post-process after the lists are built.
			 */

			if (isShopList)
			{
				if (!pickSetShop && (itemName.Find("lockpick_set") >= 0))
				{
					pickSetShop = true;
				}
			}
			else
			{
				if (!pickSetStarting && (itemName.Find("lockpick_set") >= 0))
				{
					pickSetStarting = true;
				}
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

			CShopItemPtr found = FindByID(itemDefs, itemName);

			if (found == NULL)
			{
				// Try to prepend "atdm:" as prefix, maybe this works
				found = FindByID(itemDefs, "atdm:" + itemName);
			}

			if (found != NULL) 
			{
				// grayman - TODO: If there are multiple shops, you can get multiple
				// entries for the same item. You need to group them into a single item.

				CShopItemPtr anItem(new CShopItem(*found, quantity, price, persistent));
				anItem->SetCanDrop(canDrop);
				list.Append(anItem);

				itemsAdded++;
			}
			else
			{
				gameLocal.Warning("Could not add item to shop: %s", itemName.c_str());
			}
		}
	}

	return itemsAdded;
}

// grayman (#2376) Add map entities where inv_map_start = 1 to the shop's starting list

void CShop::AddMapItems(idMapFile* mapFile)
{
	// get the difficulty level

	idStr diffString = "diff_" + idStr(gameLocal.m_DifficultyManager.GetDifficultyLevel()) + "_nospawn";

	// Cycle through map entities. Since the number of entities can change in the loop,
	// always refresh the entity count used to terminate the loop.

	// Skip entity 0, which is the world.

	for (int i = 1; i < mapFile->GetNumEntities(); i++)
	{
		idMapEntity* mapEnt = mapFile->GetEntity(i);

		// does this entity have an inv_map_start spawnflag set to 1?

		if (mapEnt->epairs.GetBool("inv_map_start", "0"))
		{
			// does this entity exist in the chosen difficulty level?

			if (idStr::Icmp(mapEnt->epairs.GetString(diffString, "0"), "0") == 0)
			{
				idStr itemName = mapEnt->epairs.GetString("classname");
				int quantity;
				bool isWeapon = false;	// is this an arrow?
				int max_ammo = 1;	// in case this is a weapon

				// Special handling for arrows. The shop definitions allow for
				// atdm:weapon_*, but not atdm:ammo_*. The latter form is used on
				// map entities. If this is an atdm:ammo_* entity, change its ID (itemName)
				// to the allowable atdm:weapon_* form.

				if (itemName.Find("atdm:ammo_") >= 0)
				{
					isWeapon = true;
					itemName.Replace( "atdm:ammo_", "atdm:weapon_" );

					// An arrow's quantity is defined by "inv_ammo_amount" instead
					// of "inv_count". Look for that.

					quantity = mapEnt->epairs.GetInt("inv_ammo_amount", "0");

					// Arrow quantities have limits. See if you can find the limit
					// for this weapon.

					if (quantity > 0)
					{
						max_ammo = GetMaxAmmo(itemName);
						quantity = (quantity > max_ammo) ? max_ammo : quantity;
					}
				}
				else
				{
					quantity = mapEnt->epairs.GetInt("inv_count", "1");
				}

				if (quantity > 0)
				{
					CShopItemPtr found = FindByID(itemDefs, itemName);

					if (found == NULL)
					{
						// Try again with "atdm:" prepended
						found = FindByID(itemDefs, "atdm:" + itemName);
					}

					if (found != NULL)
					{
						// If this item is stackable, and already exists in the startingItems list,
						// bump up the quantity there instead of appending the item to the list.
						// If the item is not stackable, and we already have it, ignore it.

						bool appendMapItem = true;
						for (int j = 0 ; j < startingItems.Num(); j++)
						{
							CShopItemPtr listItem = startingItems[j];
							if (idStr::Icmp(listItem->GetID(),itemName) == 0)
							{
								int oldQuantity = listItem->GetCount();
								int newQuantity = oldQuantity + quantity;
								bool isStackable = listItem->GetStackable();

								// Weapons have ammo limits. Even though you might have
								// adjusted that already in the incoming item,
								// you have to check again when it's added to
								// the existing amount already in the starting items.

								if (isWeapon)
								{
									if (newQuantity > max_ammo)
									{
										newQuantity = max_ammo;
									}
									quantity = newQuantity - oldQuantity; // amount to give
								}
								else if (!isStackable)
								{
									quantity = 0; // don't adjust item's quantity
								}

								if (quantity > 0)
								{
									listItem->ChangeCount(quantity); // add quantity to count
								}
								appendMapItem = false;
								break;
							}
						}

						// Append the item to the list if it didn't contribute quantity to
						// an existing list item.

						if (appendMapItem)
						{
							CShopItemPtr anItem(new CShopItem(*found, quantity, 0, false));
							bool canDrop = mapEnt->epairs.GetBool("inv_droppable", "1");
							anItem->SetCanDrop(canDrop);
							startingItems.Append(anItem);
						}
					}
					else
					{
						gameLocal.Warning("Map entity is not a valid shop item: %s", itemName.c_str());
					}
				}
			}
		}
	}
}

// grayman (#2376) - Post processing to remove individual lockpicks when a lockpick_set
// is in either the for sale or starting items lists.

void CShop::CheckPicks(ShopItemList& list)
{
	// Post processing for lockpicks. A lockpick_set is present,
	// so remove individual triangle and snake picks from the list,
	// since you get them from lockpick_set.

	for (int i = 0; i < list.Num(); i++) // regrab list size each iteration because it can change
	{
		idStr itemName = list[i]->GetName();
		if ((idStr::Icmp(itemName,"Snake Lockpick") == 0) ||
			(idStr::Icmp(itemName,"Triangle Lockpick") == 0))
		{
			list.RemoveIndex(i--); // decrement index to account for removed item
		}
	}
}

void CShop::DisplayShop(idUserInterface *gui)
{
	const idStr& curStartingMap = gameLocal.m_MissionManager->GetCurrentStartingMap();

	idStr filename = va("maps/%s", curStartingMap.c_str());

	// Let the GUI know which map to load
	gui->SetStateString("mapStartCmd", va("exec 'map %s'", curStartingMap.c_str()));

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

	// grayman (#2376) add "inv_map_start" items to the shop's list of starting items,
	// then check for lockpick duplications.
	AddMapItems(mapFile);

	if (pickSetShop)
	{
		CheckPicks(itemsForSale);
	}

	if (pickSetStarting)
	{
		CheckPicks(startingItems);
	}

	// greebo: Update the amount of gold to spend based on the loot the player found earlier
	AddGoldFromPreviousMission();

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
		forSaleItem = CShopItemPtr(new CShopItem(*boughtItem, 0, boughtItem->GetCost(), boughtItem->GetPersistent()));

		itemsForSale.Append(forSaleItem);
	}

	forSaleItem->ChangeCount(1);
};

CShopItemPtr CShop::FindPurchasedByID(const char *id)
{
	return FindByID(itemsPurchased, id);
}

CShopItemPtr CShop::FindStartingItemByID(const char *id)
{
	return FindByID(startingItems, id);
}

CShopItemPtr CShop::FindForSaleByID(const char *id)
{
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
			*forSaleItem, 0, forSaleItem->GetCost(), forSaleItem->GetPersistent())
		);

		itemsPurchased.Append(boughtItem);

		// scroll so new item is visible in purchased list
		if (itemsPurchased.Num() > purchasedTop + LIST_SIZE_PURCHASED)
		{
			purchasedTop = itemsPurchased.Num() - LIST_SIZE_PURCHASED;
		}
	}

	boughtItem->ChangeCount(1);
};

void CShop::DropUndropItem(int index)
{
	const CShopItemPtr& dropItem = startingItems[startingTop + index];

	if (dropItem->GetDroppedCount() > 0)
	{
		dropItem->Undrop();
	}
	else
	{
		// Tels: Drop() will check if the item can be dropped
		dropItem->Drop();
	}
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

void CShop::UpdateGUI(idUserInterface* gui)
{
	gui->SetStateInt("gold", gold);
	gui->SetStateInt("forSaleMoreVisible", itemsForSale.Num() > LIST_SIZE_FOR_SALE);
	gui->SetStateInt("purchasedMoreVisible", itemsPurchased.Num() > LIST_SIZE_PURCHASED);
	gui->SetStateInt("startingMoreVisible", startingItems.Num() > LIST_SIZE_STARTING);

	if (GetNothingForSale())
	{
		// nothing for sale, let the user know
		gui->SetStateInt("forSaleAvail0", 0);
		gui->SetStateString("forSale0_name", "<no items for sale>");
		// Tels: Fix #2661: do not show a description if nothing is for sale
		gui->SetStateString("gui::forSale0_desc", "");
		gui->SetStateString("gui::forSale0_image", "");
		gui->SetStateString("forSaleCost0_cost", "0");
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
		// Tels: if the item can be dropped or undropped, this is 1
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
			// Tels: Fix #2563 (startingItems can always be dropped, regardless of how much gold you have)
			// available = item->GetCost() <= gold ? item->GetCount() : 0;
			available = item->GetCount();
			dropVisible = item->GetCanDrop();
		}

		gui->SetStateBool(guiDrop, dropVisible);
		gui->SetStateInt(guiAvailable, available);
		gui->SetStateString(guiName, name);
		gui->SetStateString(guiDesc, desc);
		gui->SetStateString(guiImage, image);
	}

	// Tels: Always tell the GUI to refresh the display so the colors change
	gui->HandleNamedEvent("UpdateItemColours");

	// Tels: Reset these only after UpdateItemColors
	gui->SetStateInt("boughtItem", -1);
	gui->SetStateInt("soldItem", -1);
	gui->SetStateInt("dropItem", -1);
}

void CShop::AddGoldFromPreviousMission()
{
	// TODO
}
