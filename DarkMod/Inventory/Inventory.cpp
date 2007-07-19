/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 987 $
 * $Date: 2007-05-12 15:36:09 +0200 (Sa, 12 Mai 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#pragma warning(disable : 4533 4800)

static bool init_version = FileVersionList("$Id: Inventory.cpp 987 2007-05-12 13:36:09Z greebo $", init_version);

#include "Inventory.h"
#include "WeaponItem.h"

#include "../game/game_local.h"

#include "../MissionData.h"

const idEventDef EV_PostRestore( "postRestore", NULL );

static idLinkList<idClass>	tdmInventoryObjList;

static idStr sLootTypeName[LT_COUNT] = 
{
	"loot_none",
	"loot_jewels",
	"loot_gold",
	"loot_goods"
};

CLASS_DECLARATION(idClass, CInventory)
END_CLASS

CInventory::CInventory()
: idClass()
{
	m_Owner = NULL;
	CreateCategory(TDM_INVENTORY_DEFAULT_GROUP);	// We always have a defaultgroup if nothing else
	m_LootItemCount = 0;
	m_Gold = 0;
	m_Jewelry = 0;
	m_Goods = 0;
	m_HighestCursorId = 0;
}

CInventory::~CInventory()
{
	int i, n;

	n = m_Category.Num();
	for(i = 0; i < n; i++)
		delete m_Category[i];
}

int CInventory::GetLoot(int &Gold, int &Jewelry, int &Goods)
{
	int i;

	Gold = 0;
	Jewelry = 0;
	Goods = 0;

	for(i = 0; i < m_Category.Num(); i++)
		m_Category[i]->GetLoot(Gold, Jewelry, Goods);

	Gold += m_Gold;
	Jewelry += m_Jewelry;
	Goods += m_Goods;

	return Gold + Jewelry + Goods;
}

CInventoryItem* CInventory::ValidateLoot(idEntity *ent)
{
	CInventoryItem *rc = NULL;
	int LGroupVal = 0;
	int dummy = 0; // for calling GetLoot

	CInventoryItem::LootType lootType = CInventoryItem::getLootTypeFromSpawnargs(ent->spawnArgs);
	int value = ent->spawnArgs.GetInt("inv_loot_value", "-1");

	if (lootType != CInventoryItem::LT_NONE && value > 0)
	{
		// If we have an anonymous loot item, we don't need to 
		// store it in the inventory.
		switch(lootType)
		{
			case CInventoryItem::LT_GOLD:
				m_Gold += value;
				LGroupVal = m_Gold;
			break;

			case CInventoryItem::LT_GOODS:
				m_Goods += value;
				LGroupVal = m_Goods;
			break;

			case CInventoryItem::LT_JEWELS:
				m_Jewelry += value;
				LGroupVal = m_Jewelry;
			break;
		}

		m_LootItemCount++;

		rc = GetItem(TDM_LOOT_INFO_ITEM);

		// Objective Callback for loot:
		// Pass the loot type name and the net loot value of that group
		gameLocal.m_MissionData->InventoryCallback( 
			ent, 
			sLootTypeName[lootType], 
			LGroupVal, 
			GetLoot( dummy, dummy, dummy ), 
			true 
		);
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Item %s doesn't have an inventory name and is not anonymous.\r", ent->name.c_str());
	}

	return rc;
}

void CInventory::SetOwner(idEntity *owner)
{
	int i, n;

	m_Owner = owner; 
	n = m_Category.Num();
	for(i = 0; i < n; i++)
		m_Category[i]->SetOwner(owner);
}

CInventoryCategory *CInventory::CreateCategory(const idStr& CategoryName, int *Index)
{
	CInventoryCategory	*rc = NULL;
	int i;

	if (CategoryName.IsEmpty())
		goto Quit; // empty category name

	if((rc = GetCategory(CategoryName, Index)) != NULL)
		goto Quit; // Category already exists

	// Try to allocate a new category with a link back to <this> Inventory
	if((rc = new CInventoryCategory(this)) == NULL)
		goto Quit; // Creation failed

	rc->m_Name = CategoryName;
	i = m_Category.AddUnique(rc);
	if(Index != NULL)
		*Index = i;

Quit:
	return rc;
}

CInventoryCategory *CInventory::GetCategory(const idStr& CategoryName, int *Index) {
	// If the groupname is empty we look for the default group
	if (CategoryName.IsEmpty()) {
		return GetCategory(TDM_INVENTORY_DEFAULT_GROUP);
	}

	// Traverse the categories and find the one matching <CategoryName>
	for (int i = 0; i < m_Category.Num(); i++) {
		if (m_Category[i]->m_Name == CategoryName) {
			if (Index != NULL) {
				*Index = i;
			}

			return m_Category[i];
		}
	}

	return NULL;
}

CInventoryCategory* CInventory::GetCategory(int index) {
	if (index >= 0 && index < m_Category.Num()) {
		return m_Category[index];
	}
	return NULL;
}

int CInventory::GetCategoryIndex(const idStr& CategoryName)
{
	int i = -1;

	GetCategory(CategoryName, &i);

	return i;
}

int CInventory::GetCategoryIndex(const CInventoryCategory* Category)
{
	// If the groupname is empty we look for the default group
	if (Category == NULL) {
		return -1;
	}

	// Traverse the categories and find the one matching <CategoryName>
	for (int i = 0; i < m_Category.Num(); i++) {
		if (m_Category[i] == Category) {
			return i;
		}
	}

	return -1;
}

int CInventory::GetCategoryItemIndex(const idStr& ItemName, int *ItemIndex)
{
	int rc = -1;
	int i;
	int n = -1;

	if(ItemIndex != NULL)
		*ItemIndex = -1;

	if (ItemName.IsEmpty())
		goto Quit;

	for(i = 0; i < m_Category.Num(); i++)
	{
		if((n = m_Category[i]->GetItemIndex(ItemName)) != -1)
		{
			if(ItemIndex != NULL)
				*ItemIndex = n;

			rc = i;
			break;
		}
	}

Quit:
	return rc;
}

int CInventory::GetCategoryItemIndex(CInventoryItem *Item, int *ItemIndex)
{
	int rc = -1;
	int i;
	int n = -1;

	if(ItemIndex != NULL)
		*ItemIndex = -1;

	for(i = 0; i < m_Category.Num(); i++)
	{
		if((n = m_Category[i]->GetItemIndex(Item)) != -1)
		{
			if(ItemIndex != NULL)
				*ItemIndex = n;

			rc = i;
			break;
		}
	}

	return rc;
}

CInventoryItem *CInventory::PutItem(idEntity *ent, idEntity *owner)
{
	CInventoryItem* returnValue = NULL;

	// Sanity checks
	if(ent == NULL || owner == NULL) {
		return returnValue;
	}

	// Check for loot items
	returnValue = ValidateLoot(ent);

	if (returnValue != NULL) {
		// The item is a valid loot item, remove the entity and return
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added loot item to inventory: %s\r", ent->name.c_str());

		// Remove the entity, it is a loot item (which vanish when added to the inventory)
		RemoveEntityFromMap(ent, true);

		return returnValue;
	}

	// Not a loot item, determine name and category to check for existing item of same name/category
	idStr name = ent->spawnArgs.GetString("inv_name", "");
	idStr category = ent->spawnArgs.GetString("inv_category", "");

	if (name.IsEmpty() || category.IsEmpty()) {
		// Invalid inv_name or inv_category
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put %s in inventory: inv_name or inv_category not specified.\r", ent->name.c_str());
		return returnValue;
	}

	returnValue = ValidateAmmo(ent);

	if (returnValue != NULL) {
		// Remove the entity from the game, the ammonition is added
		RemoveEntityFromMap(ent, true);

		return returnValue;
	}

	// Check for existing items (create the category if necessary (hence the TRUE))
	CInventoryItem* existing = GetItem(name, category, true);

	if (existing != NULL) {
		// Item must be stackable, if items of the same name/category already exist
		if (!ent->spawnArgs.GetBool("inv_stackable", "0")) {
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

		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added stackable item to inventory: %s\r", ent->name.c_str());
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("New inventory item stack count is: %d\r", existing->GetCount());

		// Remove the entity, it has been stacked
		RemoveEntityFromMap(ent, true);

		// Return the existing value instead of a newly created one
		returnValue = existing;
	}
	else {
		// Item doesn't exist, create a new InventoryItem
		CInventoryItem* item = new CInventoryItem(ent, owner);

		if (item != NULL) {
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Adding new inventory item %s to category %s...\r", name.c_str(), category.c_str());
			// Put the item into its category
			PutItem(item, category);
			item->SetCount(1);

			// Hide the entity from the map (don't delete the entity)
			RemoveEntityFromMap(ent, false);
		}
		else {
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put item into category: %s.\r", ent->name.c_str());
		}

		returnValue = item;
	}
	
	return returnValue;
}

void CInventory::PutEntityInMap(idEntity *ent, idEntity *owner, CInventoryItem *item)
{
	if(ent == NULL || owner == NULL || item == NULL)
		return;

	// Make the item visible
	ent->GetPhysics()->LinkClip();
	ent->Bind(item->m_BindMaster.GetEntity(), item->m_Orientated);
	ent->Show();
	ent->UpdateVisuals();
	ent->PostEventMS(&EV_Activate, 0, ent);

	// Objectives callback.  Cannot drop loot, so assume it is not loot
	gameLocal.m_MissionData->InventoryCallback( ent, item->GetName(), item->GetValue(), 1, false ); 
}

void CInventory::RemoveEntityFromMap(idEntity *ent, bool bDelete)
{
	if(ent == NULL)
		return;

	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Hiding entity from game: %s...\r", ent->name.c_str());
	// Make the item invisible
	ent->Unbind();
	ent->GetPhysics()->PutToRest();
	ent->GetPhysics()->UnlinkClip();
	ent->Hide();



	if(bDelete == true) {
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Deleting entity from game: %s...\r", ent->name.c_str());
		ent->PostEventMS(&EV_Remove, 0);
	}
}

void CInventory::PutItem(CInventoryItem *item, const idStr& category)
{
	int i;
	CInventoryCategory *gr;

	if(item == NULL)
		goto Quit;

	// Check if it is the default group or not.
	if (category.IsEmpty())
	{
		// category is empty, assign the item to the default group
		gr = m_Category[0];
	}
	else
	{
		gr = GetCategory(category, &i);
		if(gr == NULL)
			gr = CreateCategory(category);
	}

	// Pack the item into the category
	gr->PutItem(item);

	// Objective callback for non-loot items:
	// non-loot item passes in inv_name and individual item count, SuperGroupVal of 1
	gameLocal.m_MissionData->InventoryCallback
								( item->GetItemEntity(), item->GetName(), 
									item->GetCount(), 1, true );

Quit:
	return;
}

CInventoryItem *CInventory::GetItem(const idStr& Name, const idStr& Category, bool bCreateCategory)
{
	CInventoryItem *rc = NULL;
	int i, n, s;
	CInventoryCategory *gr;

	if (Category.IsEmpty())
	{
		n = m_Category.Num();
		s = 0;
	}
	else
	{
		gr = GetCategory(Category, &i);
		if(gr == NULL)
		{
			if(bCreateCategory == true)
				gr = CreateCategory(Category, &i);
			else
				goto Quit;
		}

		n = i+1;
		s = i;
	}

	for(i = s; i < n; i++)
	{
		gr = m_Category[i];
		if((rc = gr->GetItem(Name)) != NULL)
			goto Quit;
	}

Quit:
	return rc;
}

CInventoryItem *CInventory::GetItemById(const idStr& id, const idStr& Category, bool bCreateCategory)
{
	CInventoryItem *rc = NULL;
	int i, n, s;
	CInventoryCategory *gr;

	if (id.IsEmpty())
		goto Quit;

	if (Category.IsEmpty())
	{
		n = m_Category.Num();
		s = 0;
	}
	else
	{
		gr = GetCategory(Category, &i);
		if(gr == NULL)
		{
			if(bCreateCategory == true)
				gr = CreateCategory(Category, &i);
			else
				goto Quit;
		}

		n = i+1;
		s = i;
	}

	for(i = s; i < n; i++)
	{
		gr = m_Category[i];
		if((rc = gr->GetItemById(id)) != NULL)
			goto Quit;
	}

Quit:
	return rc;
}

CInventoryCursor* CInventory::CreateCursor(void)
{
	CInventoryCursor *rc = NULL;

	// Get a new ID for this cursor
	int id = GetHighestCursorId() + 1;

	if((rc = new CInventoryCursor(this, id)) != NULL)
	{
		m_Cursor.AddUnique(rc);
		m_HighestCursorId++;
	}

	return rc;
}

CInventoryCursor* CInventory::GetCursor(int id)
{
	for(int i = 0; i < m_Cursor.Num(); i++)
	{
		if (m_Cursor[i]->GetId() == id)
		{
			return m_Cursor[i];
		}
	}

	DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Requested Cursor Id %d not found!\r", id);
	return NULL;
}

int CInventory::GetHighestCursorId()
{
	return m_HighestCursorId;
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
	int num;

	m_Owner.Restore(savefile);

	savefile->ReadInt(num);
	for(int i = 0; i < num; i++) {
		CInventoryCategory* category = new CInventoryCategory(this, "");

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
		CInventoryCursor* cursor = new CInventoryCursor(this, 0);

		cursor->Restore(savefile);
		m_Cursor.Append(cursor);
	}
}

void CInventory::removeCategory(CInventoryCategory* category) {
	// Cycle through the categories and remove the specified category.
	for (int i = 0; i < m_Category.Num(); i++) {
		if (m_Category[i] == category) {
			m_Category.RemoveIndex(i);
			delete category;
			break;
		}
	}
}

CInventoryItem* CInventory::ValidateAmmo(idEntity* ent) {
	CInventoryItem* returnValue = NULL;

	// Sanity check
	if (ent == NULL) {
		return returnValue;
	}

	idStr name = ent->spawnArgs.GetString("inv_name", "");
	idStr category = ent->spawnArgs.GetString("inv_category", "");

	// Check for ammonition
	if (category == TDM_CATEGORY_AMMO) {
		idStr ammoAmountKey = TDM_INVENTORY_AMMO_PREFIX;
		const idKeyValue* key = ent->spawnArgs.MatchPrefix(TDM_INVENTORY_AMMO_PREFIX);
		if (key != NULL) {
			int amount = ent->spawnArgs.GetInt(key->GetKey().c_str(), "0");

			// Retrieve the weapon name, e.g. "broadhead", by stripping the prefix
			idStr weaponName = key->GetKey();
			weaponName.Strip(TDM_INVENTORY_AMMO_PREFIX);

			// Find the weapon category
			CInventoryCategory* weaponCategory = GetCategory(TDM_PLAYER_WEAPON_CATEGORY);

			if (weaponCategory == NULL) {
				DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Could not find weapon category in inventory.\r");
				return returnValue;
			}
			
			// Look for the weapon with the given name
			for (int i = 0; i < weaponCategory->size(); i++) {
				CInventoryWeaponItem* weaponItem = dynamic_cast<CInventoryWeaponItem*>(weaponCategory->GetItem(i));

				// Is this the right weapon?
				if (weaponItem != NULL && weaponItem->getWeaponName() == weaponName) {
					DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Adding %d ammo to weapon %s.\r", amount, weaponName.c_str());
					// Add the ammo to this weapon
					weaponItem->setAmmo(weaponItem->getAmmo() + amount);

					// We're done
					return weaponItem;
				}
			}

			// Loop ended without result, name not found
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Could not add ammo to weapon: name not found %s.\r", weaponName.c_str());
		}
		else {
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot add ammo entity %s to inventory: no key with inv_ammo_* prefix.\r", ent->name.c_str());
		}
	}

	return returnValue;
}