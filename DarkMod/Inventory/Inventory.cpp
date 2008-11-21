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

#pragma warning(disable : 4533 4800)

static bool init_version = FileVersionList("$Id$", init_version);

#include "Inventory.h"
#include "WeaponItem.h"

#include "../game/game_local.h"

#include "../MissionData.h"

static idStr sLootTypeName[LT_COUNT] = 
{
	"loot_none",
	"loot_jewels",
	"loot_gold",
	"loot_goods"
};

CLASS_DECLARATION(idClass, CInventory)
END_CLASS

CInventory::CInventory() :
	m_HighestCursorId(0),
	m_LootItemCount(0),
	m_Gold(0),
	m_Jewelry(0),
	m_Goods(0)
{
	m_Owner = NULL;

	CreateCategory(TDM_INVENTORY_DEFAULT_GROUP);	// We always have a defaultgroup if nothing else
}

CInventory::~CInventory()
{
	Clear();
}

void CInventory::Clear()
{
	m_Owner = NULL;
	m_Category.Clear();
	m_Cursor.Clear();
}

int	CInventory::GetNumCategories() const
{
	return m_Category.Num();
}

int CInventory::GetLoot(int &Gold, int &Jewelry, int &Goods)
{
	int i;

	Gold = 0;
	Jewelry = 0;
	Goods = 0;

	for(i = 0; i < m_Category.Num(); i++)
	{
		m_Category[i]->GetLoot(Gold, Jewelry, Goods);
	}

	Gold += m_Gold;
	Jewelry += m_Jewelry;
	Goods += m_Goods;

	return Gold + Jewelry + Goods;
}

void CInventory::SetLoot(int Gold, int Jewelry, int Goods)
{
	m_Gold = Gold;
	m_Jewelry = Jewelry;
	m_Goods = Goods;
}

void CInventory::NotifyOwnerAboutPickup(const idStr& pickedUpStr, const CInventoryItemPtr&)
{
	if (!cv_tdm_inv_hud_pickupmessages.GetBool()) return; // setting is turned off

	if (!m_Owner.GetEntity()->IsType(idPlayer::Type)) return; // owner is not a player
	
	idPlayer* player = static_cast<idPlayer*>(m_Owner.GetEntity());

	// Prepend the "acquired" text
	idStr pickedUpMsg = idStr(common->GetLanguageDict()->GetString("#str_07215")) + pickedUpStr;

	// Now actually send the message
	player->SendInventoryPickedUpMessage(pickedUpMsg);
}

CInventoryItemPtr CInventory::ValidateLoot(idEntity *ent)
{
	CInventoryItemPtr rc;
	int LGroupVal = 0;
	int dummy1(0), dummy2(0), dummy3(0); // for calling GetLoot

	CInventoryItem::LootType lootType = CInventoryItem::GetLootTypeFromSpawnargs(ent->spawnArgs);
	int value = ent->spawnArgs.GetInt("inv_loot_value", "-1");

	if (lootType != CInventoryItem::LT_NONE && value > 0)
	{
		idStr pickedUpMsg = idStr(value);

		// If we have an anonymous loot item, we don't need to 
		// store it in the inventory.
		switch(lootType)
		{
			case CInventoryItem::LT_GOLD:
				m_Gold += value;
				LGroupVal = m_Gold;
				pickedUpMsg += " in Gold";
			break;

			case CInventoryItem::LT_GOODS:
				m_Goods += value;
				LGroupVal = m_Goods;
				pickedUpMsg += " in Goods";
			break;

			case CInventoryItem::LT_JEWELS:
				m_Jewelry += value;
				LGroupVal = m_Jewelry;
				pickedUpMsg += " in Jewels";
			break;
			
			default: break;
		}

		m_LootItemCount++;

		rc = GetItem(TDM_LOOT_INFO_ITEM);

		assert(rc != NULL); // the loot item must exist

		// greebo: Update the total loot value in the objectives system BEFORE
		// the InventoryCallback. Some comparisons rely on a valid total loot value.
		gameLocal.m_MissionData->ChangeFoundLoot( value );

		// Objective Callback for loot on a specific entity:
		// Pass the loot type name and the net loot value of that group
		gameLocal.m_MissionData->InventoryCallback( 
			ent, 
			sLootTypeName[lootType], 
			LGroupVal, 
			GetLoot( dummy1, dummy2, dummy3 ), 
			true 
		);

		// Take the loot icon of the picked up item and use it for the loot stats item

		idStr lootIcon = ent->spawnArgs.GetString("inv_icon");
		if (rc != NULL && !lootIcon.IsEmpty())
		{
			rc->SetIcon(lootIcon);
		}
		
		NotifyOwnerAboutPickup(pickedUpMsg, rc);
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Item %s doesn't have an inventory name and is not anonymous.\r", ent->name.c_str());
	}

	return rc;
}

void CInventory::SetOwner(idEntity *owner)
{
	m_Owner = owner; 

	for (int i = 0; i < m_Category.Num(); i++)
	{
		m_Category[i]->SetOwner(owner);
	}
}

CInventoryCategoryPtr CInventory::CreateCategory(const idStr& categoryName, int* index)
{
	if (categoryName.IsEmpty()) return CInventoryCategoryPtr(); // empty category name

	// Try to lookup the category, maybe it exists already
	CInventoryCategoryPtr rc = GetCategory(categoryName, index);

	if (rc != NULL) return rc; // Category already exists

	// Try to allocate a new category with a link back to <this> Inventory
	rc = CInventoryCategoryPtr(new CInventoryCategory(this, categoryName));
	
	// Add the new Category to our list
	int i = m_Category.AddUnique(rc);

	// Should we return an index?
	if (index != NULL)
	{
		*index = i;
	}

	return rc;
}

CInventoryCategoryPtr CInventory::GetCategory(const idStr& categoryName, int* index)
{
	// If the groupname is empty we look for the default group
	if (categoryName.IsEmpty())
	{
		return GetCategory(TDM_INVENTORY_DEFAULT_GROUP);
	}

	// Traverse the categories and find the one matching <CategoryName>
	for (int i = 0; i < m_Category.Num(); i++)
	{
		if (m_Category[i]->GetName() == categoryName)
		{
			if (index != NULL)
			{
				*index = i;
			}

			return m_Category[i];
		}
	}

	return CInventoryCategoryPtr(); // not found
}

CInventoryCategoryPtr CInventory::GetCategory(int index)
{
	// return NULL for invalid indices
	return (index >= 0 && index < m_Category.Num()) ? m_Category[index] : CInventoryCategoryPtr();
}

int CInventory::GetCategoryIndex(const idStr& categoryName)
{
	int i = -1;

	GetCategory(categoryName, &i);

	return i;
}

int CInventory::GetCategoryIndex(const CInventoryCategoryPtr& category)
{
	return m_Category.FindIndex(category);
}

int CInventory::GetCategoryItemIndex(const idStr& itemName, int* itemIndex)
{
	// Set the returned index to -1 if applicable
	if (itemIndex != NULL) *itemIndex = -1;

	if (itemName.IsEmpty()) return -1;

	for (int i = 0; i < m_Category.Num(); i++)
	{
		// Try to find the item within the category
		int n = m_Category[i]->GetItemIndex(itemName);

		if (n != -1)
		{
			// Found, set the item index if desired
			if (itemIndex != NULL) *itemIndex = n;

			// Return the category index
			return i;
		}
	}

	return -1; // not found
}

int CInventory::GetCategoryItemIndex(const CInventoryItemPtr& item, int* itemIndex)
{
	if (itemIndex != NULL) *itemIndex = -1;

	for (int i = 0; i < m_Category.Num(); i++)
	{
		int n = m_Category[i]->GetItemIndex(item);

		if (n != -1)
		{
			// Found, return the index
			if (itemIndex != NULL) *itemIndex = n;

			// Return the category index
			return i;
		}
	}

	return -1; // not found
}

CInventoryItemPtr CInventory::PutItem(idEntity *ent, idEntity *owner)
{
	// Sanity checks
	if (ent == NULL || owner == NULL) return CInventoryItemPtr();

	// Check for loot items
	CInventoryItemPtr returnValue = ValidateLoot(ent);

	if (returnValue != NULL)
	{
		// The item is a valid loot item, remove the entity and return
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added loot item to inventory: %s\r", ent->name.c_str());

		// Remove the entity, it is a loot item (which vanish when added to the inventory)
		RemoveEntityFromMap(ent, true);

		return returnValue;
	}

	// Let's see if this is an ammonition item
	returnValue = ValidateAmmo(ent);

	if (returnValue != NULL)
	{
		// Remove the entity from the game, the ammonition is added
		RemoveEntityFromMap(ent, true);

		return returnValue;
	}

	// Not a loot or ammo item, determine name and category to check for existing item of same name/category
	idStr name = ent->spawnArgs.GetString("inv_name", "");
	idStr category = ent->spawnArgs.GetString("inv_category", "");

	if (name.IsEmpty() || category.IsEmpty())
	{
		// Invalid inv_name or inv_category
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put %s in inventory: inv_name or inv_category not specified.\r", ent->name.c_str());
		return returnValue;
	}

	// Check for existing items (create the category if necessary (hence the TRUE))
	CInventoryItemPtr existing = GetItem(name, category, true);

	if (existing != NULL)
	{
		// Item must be stackable, if items of the same name/category already exist
		if (!ent->spawnArgs.GetBool("inv_stackable", "0"))
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put %s in inventory: not stackable.\r", ent->name.c_str());
			return returnValue;
		}

		// Item is stackable, determine how many items should be added to the stack
		int count = ent->spawnArgs.GetInt("inv_count", "1");
		
		// Increase the stack count
		existing->SetCount(existing->GetCount() + count);

		// We added a stackable item that was already in the inventory
		gameLocal.m_MissionData->InventoryCallback(
			existing->GetItemEntity(), existing->GetName(), 
			existing->GetCount(), 
			1, 
			true
		);

		// Notify the player, if appropriate
		if (!ent->spawnArgs.GetBool("inv_map_start", "0"))
		{
			idStr msg = name;

			if (count > 0) 
			{
				name += " x" + idStr(count);
			}

			NotifyOwnerAboutPickup(msg, existing);
		}
		
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added stackable item to inventory: %s\r", ent->name.c_str());
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("New inventory item stack count is: %d\r", existing->GetCount());

		// Remove the entity, it has been stacked
		RemoveEntityFromMap(ent, true);

		// Return the existing value instead of a newly created one
		returnValue = existing;
	}
	else
	{
		// Item doesn't exist, create a new InventoryItem
		CInventoryItemPtr item(new CInventoryItem(ent, owner));

		if (item != NULL)
		{
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Adding new inventory item %s to category %s...\r", name.c_str(), category.c_str());
			// Put the item into its category
			PutItem(item, category);

			// We added a new inventory item
			gameLocal.m_MissionData->InventoryCallback(
				item->GetItemEntity(), item->GetName(), 
				1, 
				1, 
				true
			);

			if (!ent->spawnArgs.GetBool("inv_map_start", "0"))
			{
				NotifyOwnerAboutPickup(name, item);
			}

			// Hide the entity from the map (don't delete the entity)
			RemoveEntityFromMap(ent, false);
		}
		else
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put item into category: %s.\r", ent->name.c_str());
		}

		returnValue = item;
	}
	
	return returnValue;
}

void CInventory::RemoveEntityFromMap(idEntity *ent, bool bDelete)
{
	if (ent == NULL) return;

	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Hiding entity from game: %s...\r", ent->name.c_str());

	// Make the item invisible
	ent->Unbind();
	ent->GetPhysics()->PutToRest();
	ent->GetPhysics()->UnlinkClip();
	ent->Hide();

	if (bDelete == true)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Deleting entity from game: %s...\r", ent->name.c_str());
		ent->PostEventMS(&EV_Remove, 0);
	}
}

void CInventory::PutItem(const CInventoryItemPtr& item, const idStr& categoryName)
{
	if (item == NULL) return;
	
	CInventoryCategoryPtr category;

	// Check if it is the default group or not.
	if (categoryName.IsEmpty())
	{
		// category is empty, assign the item to the default group
		category = m_Category[0];
	}
	else
	{
		// Try to find the category with the given name
		category = GetCategory(categoryName);

		// If not found, create it
		if (category == NULL)
		{
			category = CreateCategory(categoryName);
		}
	}

	// Pack the item into the category
	category->PutItem(item);

	// Objective callback for non-loot items:
	// non-loot item passes in inv_name and individual item count, SuperGroupVal of 1
	gameLocal.m_MissionData->InventoryCallback( 
		item->GetItemEntity(), 
		item->GetName(), 
		item->GetCount(), 
		1, 
		true
	);
}

bool CInventory::ReplaceItem(idEntity* oldItemEnt, idEntity* newItemEnt)
{
	if (oldItemEnt == NULL) return false;

	idStr oldInvName = oldItemEnt->spawnArgs.GetString("inv_name");

	CInventoryItemPtr oldItem = GetItem(oldInvName);

	if (oldItem == NULL)
	{
		gameLocal.Warning("Could not find old inventory item for %s\n", oldItemEnt->name.c_str());
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Could not find old inventory item for %s\n", oldItemEnt->name.c_str());
		return false;
	}

	// greebo: Let's call PutItem on the new entity first to see what kind of item this is
	// PutItem will also take care of the mission data callbacks for the objectives
	CInventoryItemPtr newItem = PutItem(newItemEnt, m_Owner.GetEntity());

	if (newItem != NULL && newItem->Category() == oldItem->Category())
	{
		// New item has been added, swap the old and the new one to fulfil the inventory position guarantee
		oldItem->Category()->SwapItemPosition(oldItem, newItem);
	}
	
	// If SwapItemPosition has been called, newItem now takes the place of oldItem before the operation.
	// Remove the old item in any case, but only if the items are actually different.
	// In case anybody wonder, newItem might be the same as oldItem in the case of stackable items or loot.
	if (oldItem != newItem)
	{
		RemoveItem(oldItem);
	}

	return true;
}

void CInventory::RemoveItem(const CInventoryItemPtr& item)
{
	if (item == NULL) return;

	// Update the cursors first
	for (int i = 0; i < m_Cursor.Num(); i++)
	{
		if (m_Cursor[i]->GetCurrentItem() == item)
		{
			// Advance the cursor, this should be enough
			m_Cursor[i]->GetNextItem();
		}
	}

	// Now remove the item, the cursors are updated.
	item->Category()->RemoveItem(item);
}

CInventoryItemPtr CInventory::GetItem(const idStr& name, const idStr& categoryName, bool createCategory)
{
	// Do we have a specific category to search in?
	if (!categoryName.IsEmpty())
	{
		// We have a category name, look it up
		CInventoryCategoryPtr category = GetCategory(categoryName);

		if (category == NULL && createCategory)
		{
			// Special case, the caller requested to create this category if not found
			category = CreateCategory(categoryName);
		}

		// Let the category search for the item, may return NULL
		return (category != NULL) ? category->GetItem(name) : CInventoryItemPtr();
	}

	// No specific category specified, look in all categories
	for (int i = 0; i < m_Category.Num(); i++)
	{
		CInventoryItemPtr foundItem = m_Category[i]->GetItem(name);

		if (foundItem != NULL)
		{
			// Found the item
			return foundItem;
		}
	}

	return CInventoryItemPtr(); // nothing found
}

CInventoryItemPtr CInventory::GetItemById(const idStr& id, const idStr& categoryName, bool createCategory)
{
	// Do we have a specific category to search in?
	if (!categoryName.IsEmpty())
	{
		// We have a category name, look it up
		CInventoryCategoryPtr category = GetCategory(categoryName);

		if (category == NULL && createCategory)
		{
			// Special case, the caller requested to create this category if not found
			category = CreateCategory(categoryName);
		}

		// Let the category search for the item, may return NULL
		return (category != NULL) ? category->GetItemById(id) : CInventoryItemPtr();
	}

	// No specific category specified, look in all categories
	for (int i = 0; i < m_Category.Num(); i++)
	{
		CInventoryItemPtr foundItem = m_Category[i]->GetItemById(id);

		if (foundItem != NULL)
		{
			// Found the item
			return foundItem;
		}
	}

	return CInventoryItemPtr(); // nothing found
}

CInventoryCursorPtr CInventory::CreateCursor()
{
	// Get a new ID for this cursor
	int id = GetNewCursorId();

	CInventoryCursorPtr cursor(new CInventoryCursor(this, id));

	if (cursor != NULL)
	{
		m_Cursor.AddUnique(cursor);
	}

	return cursor;
}

CInventoryCursorPtr CInventory::GetCursor(int id)
{
	for (int i = 0; i < m_Cursor.Num(); i++)
	{
		if (m_Cursor[i]->GetId() == id)
		{
			return m_Cursor[i];
		}
	}

	DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Requested Cursor Id %d not found!\r", id);
	return CInventoryCursorPtr();
}

int CInventory::GetHighestCursorId()
{
	return m_HighestCursorId;
}

int CInventory::GetNewCursorId()
{
	return ++m_HighestCursorId;
}

void CInventory::Save(idSaveGame *savefile) const
{
	m_Owner.Save(savefile);
	
	savefile->WriteInt(m_Category.Num());
	for (int i = 0; i < m_Category.Num(); i++) {
		m_Category[i]->Save(savefile);
	}

	savefile->WriteInt(m_LootItemCount);
	savefile->WriteInt(m_Gold);
	savefile->WriteInt(m_Jewelry);
	savefile->WriteInt(m_Goods);
	savefile->WriteInt(m_HighestCursorId);

	savefile->WriteInt(m_Cursor.Num());
	for (int i = 0; i < m_Cursor.Num(); i++) {
		m_Cursor[i]->Save(savefile);
	}
}

void CInventory::Restore(idRestoreGame *savefile)
{
	// Clear all member variables beforehand 
	Clear();

	m_Owner.Restore(savefile);

	int num;
	savefile->ReadInt(num);
	for(int i = 0; i < num; i++) {
		CInventoryCategoryPtr category(new CInventoryCategory(this, ""));

		category->Restore(savefile);
		m_Category.Append(category);
	}

	savefile->ReadInt(m_LootItemCount);
	savefile->ReadInt(m_Gold);
	savefile->ReadInt(m_Jewelry);
	savefile->ReadInt(m_Goods);
	savefile->ReadInt(m_HighestCursorId);

	savefile->ReadInt(num);
	for(int i = 0; i < num; i++) {
		CInventoryCursorPtr cursor(new CInventoryCursor(this, 0));

		cursor->Restore(savefile);
		m_Cursor.Append(cursor);
	}
}

void CInventory::RemoveCategory(const CInventoryCategoryPtr& category)
{
	m_Category.Remove(category);
}

CInventoryItemPtr CInventory::ValidateAmmo(idEntity* ent)
{
	// Sanity check
	if (ent == NULL) return CInventoryItemPtr();
	
	// Check for ammonition
	int amount = ent->spawnArgs.GetInt("inv_ammo_amount", "0");

	if (amount <= 0) 
	{
		return CInventoryItemPtr(); // not ammo
	}

	CInventoryItemPtr returnValue;

	idStr weaponName = ent->spawnArgs.GetString("inv_weapon_name", "");

	if (weaponName.IsEmpty())
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Could not find 'inv_weapon_name' on item %s.\r", ent->name.c_str());
		gameLocal.Warning("Could not find 'inv_weapon_name' on item %s.\r", ent->name.c_str());
		return returnValue;
	}

	// Find the weapon category
	CInventoryCategoryPtr weaponCategory = GetCategory(TDM_PLAYER_WEAPON_CATEGORY);

	if (weaponCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Could not find weapon category in inventory.\r");
		return returnValue;
	}
	
	// Look for the weapon with the given name
	for (int i = 0; i < weaponCategory->GetNumItems(); i++)
	{
		CInventoryWeaponItemPtr weaponItem = 
			boost::dynamic_pointer_cast<CInventoryWeaponItem>(weaponCategory->GetItem(i));

		// Is this the right weapon?
		if (weaponItem != NULL && weaponItem->GetWeaponName() == weaponName)
		{
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Adding %d ammo to weapon %s.\r", amount, weaponName.c_str());

			// Add the ammo to this weapon
			weaponItem->SetAmmo(weaponItem->GetAmmo() + amount);

			idStr msg = ent->spawnArgs.GetString("inv_name");

			if (amount > 1)
			{
				msg += " x" + idStr(amount);
			}

			NotifyOwnerAboutPickup(msg, weaponItem);
			
			// We're done
			return weaponItem;
		}
	}

	return returnValue;
}
