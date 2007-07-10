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

CInventoryItem *CInventory::ValidateLoot(idEntity *ent, CInventoryItem::LootType lt, int v)
{
	CInventoryItem *rc = NULL;
	int LGroupVal = 0;
	int dummy = 0; // for calling GetLoot

	if(lt != CInventoryItem::LT_NONE && v > 0)
	{
		// If we have an anonymous loot item, we don't need to 
		// store it in the inventory.
		switch(lt)
		{
			case CInventoryItem::LT_GOLD:
				m_Gold += v;
				LGroupVal = m_Gold;
			break;

			case CInventoryItem::LT_GOODS:
				m_Goods += v;
				LGroupVal = m_Goods;
			break;

			case CInventoryItem::LT_JEWELS:
				m_Jewelry += v;
				LGroupVal = m_Jewelry;
			break;
		}

		m_LootItemCount++;

		rc = GetItem(TDM_LOOT_INFO_ITEM);

		// Objective Callback for loot:
		// Pass the loot type name and the net loot value of that group
		gameLocal.m_MissionData->InventoryCallback
									( ent, sLootTypeName[lt], LGroupVal, 
										GetLoot( dummy, dummy, dummy ), 
										true );
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

CInventoryCategory *CInventory::CreateCategory(const char *Name, int *Index)
{
	CInventoryCategory	*rc = NULL;
	int i;

	if(Name == NULL || Name[0] == 0)
		goto Quit;

	if((rc = GetCategory(Name, Index)) != NULL)
		goto Quit;

	if((rc = new CInventoryCategory(m_Owner.GetEntity())) == NULL)
		goto Quit;

	rc->SetInventory(this);
	rc->m_Name = Name;
	i = m_Category.AddUnique(rc);
	if(Index != NULL)
		*Index = i;

Quit:
	return rc;
}

CInventoryCategory *CInventory::GetCategory(const char *pName, int *Index)
{
	CInventoryCategory *rc = NULL;
	int i, n;

	// If the groupname is null we look for the default group
	if(pName == NULL || pName[0] == 0)
		return GetCategory(TDM_INVENTORY_DEFAULT_GROUP);

	n = m_Category.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Category[i]->m_Name.Cmp(pName) == 0)
		{
			rc = m_Category[i];
			if(Index != NULL)
				*Index = i;

			goto Quit;
		}
	}

Quit:
	return rc;
}

int CInventory::GetCategoryIndex(const char *CategoryName)
{
	int i = -1;

	GetCategory(CategoryName, &i);

	return i;
}

int CInventory::GetCategoryItemIndex(const char *ItemName, int *ItemIndex)
{
	int rc = -1;
	int i;
	int n = -1;

	if(ItemIndex != NULL)
		*ItemIndex = -1;

	if(ItemName == NULL)
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
	CInventoryItem *rc = NULL;
	CInventoryItem *item = NULL;
	idStr s;
	idStr name;
	idStr icon;
	idStr category;
	idStr id;
	bool stackable = false;
//	bool bValidItem = false;
	int v, droppable = 0, del = -1, count = 0;
	CInventoryItem::LootType lt = CInventoryItem::LT_NONE;
	CInventoryItem::ItemType it = CInventoryItem::IT_ITEM;

	if(ent == NULL || owner == NULL)
		goto Quit;

	del = 0;
	if(ent->spawnArgs.GetInt("inv_loot_type", "", v) != false)
	{
		if(v >= CInventoryItem::LT_NONE && v < CInventoryItem::LT_COUNT)
			lt = (CInventoryItem::LootType)v;
		else
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid loot type: %d for InventoryItemType on %s\r", v, ent->name.c_str());
	}

	ent->spawnArgs.GetString("inv_icon", "", icon);
	ent->spawnArgs.GetInt("inv_loot_value", "-1", v);
	ent->spawnArgs.GetInt("inv_droppable", "0", droppable);
	ent->spawnArgs.GetInt("inv_delete", "0", del);

	// If the item can be dropped we can not allow the item
	// to be deleted.
	if(del == 1 && droppable == 1)
		del = 0;

	// If we have an item without an icon and it is loot, then we can 
	// simply add the value to the global values. If the item
	// is not loot and has no icon, it is an error, because it means
	// that it is an individual item but can not be displayed.
	if(icon.Length() == 0)
	{
		if((rc = ValidateLoot(ent, lt, v)) == NULL)
			del = -1;

		// We can skip the rest of this, because either
		// it was an anonymous loot item or it was an error.
		goto Quit;
	}

	ent->spawnArgs.GetInt("inv_count", "1", count);
	if(count < 0)
		count = 0;

	ent->spawnArgs.GetString("inv_category", "", category);
	ent->spawnArgs.GetString("inv_name", "", name);
	ent->spawnArgs.GetInt("inv_stackable", "0", v);
	ent->spawnArgs.GetString("inv_item_id", "", id);
	if(v != 0)
	{
		CInventoryItem *exists = GetItem(name.c_str(), category.c_str(), true);
		stackable = true;

		if(exists != NULL)
		{
			exists->SetCount(exists->GetCount()+count);
			rc = exists;

			// We added a stackable item that was already in the inventory
			gameLocal.m_MissionData->InventoryCallback
								( exists->GetItemEntity(), exists->GetName(), 
									exists->GetCount(), 1, true );
			goto Quit;
		}
	}
	else
		count = 0;

	// We're ready to actually create an inventory item
	item = new CInventoryItem(owner);

	// "Item" Entity is NULL for deletable inventory items
	if(del == 1)
		item->SetItem(NULL);
	else
		item->SetItem(ent);
	item->SetLootType(lt);

	if(lt != CInventoryItem::LT_NONE)
	{
		int dummy;

		it = CInventoryItem::IT_LOOT;
		v = 0;
		if(ent->spawnArgs.GetInt("inv_loot_value", "", v) != false && v != 0)
			item->SetValue(v);
		else
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Value for loot item missing on entity %s\r", ent->name.c_str());

		gameLocal.m_MissionData->InventoryCallback
									( ent, sLootTypeName[lt], v, 
										GetLoot( dummy, dummy, dummy ), 
										true );
	}

	// Store the temporary variables into the Item's settings
	item->SetItemId(id);
	item->SetType(it);
	item->SetDroppable(droppable);
	item->SetDeletable(del);
	item->SetName(name);
	item->SetStackable(stackable);			// has to come before SetCount :)
	item->SetCount(count);

	item->m_BindMaster = ent->GetBindMaster();
	item->m_Orientated = ent->fl.bindOrientated;

	// Put the item into its category
	PutItem(item, category);

	rc = item;

Quit:
	// Remove the entity from the map, if this has been requested
	if(del != -1)
		RemoveEntityFromMap(ent, del);

	return rc;
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

	// Make the item invisible
	ent->Unbind();
	ent->GetPhysics()->PutToRest();
	ent->GetPhysics()->UnlinkClip();
	ent->Hide();

	if(bDelete == true)
		ent->PostEventMS(&EV_Remove, 0);
}

void CInventory::PutItem(CInventoryItem *item, char const *category)
{
	int i;
	CInventoryCategory *gr;

	if(item == NULL)
		goto Quit;

	// Check if it is the default group or not.
	if(category == NULL || category[0] == 0)
		gr = m_Category[0];
	else
	{
		gr = GetCategory(category, &i);
		if(gr == NULL)
			gr = CreateCategory(category);
	}

	gr->PutItem(item);
	if (item->IsDeletable() && item->IsDroppable())	{
		// Prevent droppable objects from being deletable
		item->SetDeletable(false);
	}

	RemoveEntityFromMap(item->GetItemEntity(), item->IsDeletable());

	// Objective callback for non-loot items:
	// non-loot item passes in inv_name and individual item count, SuperGroupVal of 1
	gameLocal.m_MissionData->InventoryCallback
								( item->GetItemEntity(), item->GetName(), 
									item->GetCount(), 1, true );

Quit:
	return;
}

CInventoryItem *CInventory::GetItem(const char *Name, char const *Category, bool bCreateCategory)
{
	CInventoryItem *rc = NULL;
	int i, n, s;
	CInventoryCategory *gr;

	if(Category == NULL || Category[0] == 0)
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

CInventoryItem *CInventory::GetItemById(const char *id, char const *Category, bool bCreateCategory)
{
	CInventoryItem *rc = NULL;
	int i, n, s;
	CInventoryCategory *gr;

	if(id == NULL || id[0] == 0)
		goto Quit;

	if(Category == NULL || Category[0] == 0)
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

CInventoryCursor *CInventory::CreateCursor(void)
{
	CInventoryCursor *rc = NULL;

	if((rc = new CInventoryCursor(this)) != NULL)
		m_Cursor.AddUnique(rc);

	return rc;
}

void CInventory::Save(idSaveGame *savefile) const
{
	// TODO: Has to call the groups and items as well.
}

void CInventory::Restore(idRestoreGame *savefile)
{
	// TODO: Has to call the groups and items as well.
}
