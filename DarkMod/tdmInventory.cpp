/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.25  2007/02/12 22:19:36  sparhawk
 * Added additional objective callback and refactored some of the inventory code.
 * Also changed the scope of the category constructor, so that it can only be used from the inventory.
 *
 * Revision 1.24  2007/02/12 07:51:02  ishtvan
 * added inventory callback to objectives
 *
 * Revision 1.23  2007/02/11 21:35:02  sparhawk
 * Fixed bugs in the inventory.
 * Stackable items are now collected only once and afterwards the counter is increased (as it should be).
 * Fixed a bug that loot only counted to totals (at least on screen).
 * Fixed some bugs and behaviour for GetItem() on the inventory.
 * Category can now be created if it doesn't already exists.
 *
 * Revision 1.22  2007/02/10 22:57:37  sparhawk
 * 1. Multiple frobs fixed.
 * 2. Having invisible items in the inventory is fixed.
 * 3. Select frobbed item after it went into the inventory
 * 4. Overlap of old and new item fixed.
 *
 * Revision 1.21  2007/02/10 14:10:39  sparhawk
 * Custom HUDs implemented. Also fixed the bug that the total for loot was alwyas doubled.
 *
 * Revision 1.20  2007/02/07 22:06:25  sparhawk
 * Items can now be frobbed and added to the inventory
 *
 * Revision 1.19  2007/02/03 21:56:21  sparhawk
 * Removed old inventories and fixed a bug in the new one.
 *
 * Revision 1.18  2007/02/03 18:07:39  sparhawk
 * Loot items implemented and various improvements to the interface.
 *
 * Revision 1.17  2007/02/01 19:47:35  sparhawk
 * Callback for inventory added.
 *
 * Revision 1.16  2007/01/31 23:41:49  sparhawk
 * Inventory updated
 *
 * Revision 1.15  2007/01/29 21:50:06  sparhawk
 * Inventory updates
 *
 * Revision 1.14  2007/01/27 11:09:10  sparhawk
 * Fixed a crash in the inventory GetNext/PrevItem
 *
 * Revision 1.13  2007/01/26 12:52:50  sparhawk
 * New inventory concept.
 *
 * Revision 1.9  2006/09/21 00:43:45  gildoran
 * Added inventory hotkey support.
 *
 * Revision 1.8  2006/08/12 14:44:23  gildoran
 * Fixed some minor bugs with inventory group iteration.
 *
 * Revision 1.7  2006/08/12 12:47:13  gildoran
 * Added a couple of inventory related cvars: tdm_inv_grouping and tdm_inv_opacity. Also fixed a bug with item iteration.
 *
 * Revision 1.6  2006/08/11 20:03:48  gildoran
 * Another update for inventories.
 *
 * Revision 1.5  2006/07/25 01:40:28  gildoran
 * Completely revamped inventory code.
 *
 * Revision 1.4  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.3  2006/03/31 23:52:31  gildoran
 * Renamed inventory objects, and added cursor script functions.
 *
 * Revision 1.2  2006/03/31 00:41:02  gildoran
 * Linked entities to inventories, and added some basic script functions to interact
 * with them.
 *
 * Revision 1.1  2006/03/30 19:45:50  gildoran
 * I made three main changes:
 * 1. I moved the new decl headers out of game_local.h and into the few files
 * that actually use them.
 * 2. I added two new functions to idLinkList: next/prevNodeCircular().
 * 3. I added the first version of the tdmInventory objects. I've been working on
 * these on a vanilla 1.3 SDK, so I could test saving/loading. They appear to work
 * just fine.
 *
 *
 * DESCRIPTION: This file contains the inventory handling for TDM. The inventory 
 * has nothing in common with the original idInventory and is totally independent
 * from it. It contains the inventory itself, the groups, items and cursors for
 * interaction with the inventory.
 * Each entity has exactly one inventory. An inventory is created when the entitie's
 * inventory is accessed for th first time and also one default group is added 
 * named "DEFAULT".
 *
 * Each item belongs to a group. If no group is specified, then it will be 
 * put in the default group. Each item also knows it's owning entity and the
 * entity it references. When an entity is destroyed, it will also destroy
 * it's item. Therefore you should never keep a pointer of an item in memory
 * and always fetch it from the inventory when you need it, as you can never 
 * know for sure, that the respective entity hasn't been destroyed yet (or
 * the item itself).
 *
 * Categories and inventories are not cleared even if they are empty. Only when 
 * the owning entity is destroyed, it will destory it's inventory along with
 * all groups and items. Keep in mind that destroying an item does NOT mean 
 * that the entity is also destroyed. After all, an entity can be an item
 * but it doesn't need to and it can exist without being one. Only the item
 * pointer is cleared, when the item si destroyed.
 * You should also not make any assumptions about item or group orderings as
 * they can be created and destroyed in arbitrary order. Only the default
 * group is always at index 0.
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#pragma warning(disable : 4533 4800)

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../game/Game_local.h"

#include "tdmInventory.h"
#include "MissionData.h"

const idEventDef EV_PostRestore( "postRestore", NULL );

static idLinkList<idClass>	tdmInventoryObjList;


///////////////////
// CInventory
///////////////////

CLASS_DECLARATION(idClass, CInventory)
END_CLASS

CInventory::CInventory()
: idClass()
{
	m_Owner = NULL;
	CreateCategory(TDM_INVENTORY_DEFAULT_GROUP);	// We always have a defaultgroup if nothing else
	m_CategoryLock = false;							// Default behaviour ...
	m_WrapAround = true;							// ... is like standard Thief inventory.
	m_CurrentCategory = 0;
	m_CurrentItem = 0;
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

void CInventory::Save(idSaveGame *savefile) const
{
	// TODO: Has to call the groups and items as well.
}

void CInventory::Restore(idRestoreGame *savefile)
{
	// TODO: Has to call the groups and items as well.
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

int CInventory::GetCategoryIndex(const char *CategoryName)
{
	int i = -1;

	GetCategory(CategoryName, &i);

	return i;
}

void CInventory::SetOwner(idEntity *owner)
{
	int i, n;

	m_Owner = owner; 
	n = m_Category.Num();
	for(i = 0; i < n; i++)
		m_Category[i]->SetOwner(owner);
}

CInventoryItem *CInventory::ValidateLoot(idEntity *ent, CInventoryItem::LootType lt, int v)
{
	CInventoryItem *rc = NULL;
	idStr LTypeName;
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
				LTypeName = "gold";
				LGroupVal = m_Gold;
			break;

			case CInventoryItem::LT_GOODS:
				m_Goods += v;
				LTypeName = "goods";
				LGroupVal = m_Goods;
			break;

			case CInventoryItem::LT_JEWELS:
				m_Jewelry += v;
				LTypeName = "jewels";
				LGroupVal = m_Jewelry;
			break;
		}

		m_LootItemCount++;

		rc = GetItem(TDM_LOOT_INFO_ITEM);

		// Objective Callback for loot:
		// Pass the loot type name and the net loot value of that group
		LTypeName = "loot_" + LTypeName;

		gameLocal.m_MissionData->InventoryCallback
									( ent, LTypeName, LGroupVal, 
										GetLoot( dummy, dummy, dummy ), 
										true );
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Item %s doesn't have an inventory name and is not anonymous.\r", ent->name.c_str());
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
	bool bValidItem = false;
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

	item = new CInventoryItem(owner);

	if(del == 1)
		item->SetItem(NULL);
	else
		item->SetItem(ent);
	item->SetLootType(lt);

	if(lt != CInventoryItem::LT_NONE)
	{
		it = CInventoryItem::IT_LOOT;
		v = 0;
		if(ent->spawnArgs.GetInt("inv_loot_value", "", v) != false && v != 0)
			item->SetValue(v);
		else
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Value for loot item missing on entity %s\r", ent->name.c_str());
	}

	item->SetItemId(id);
	item->SetType(it);
	item->SetDroppable(droppable);
	item->SetDeleteable(del);
	item->SetName(name);
	item->SetStackable(stackable);			// has to come before SetCount :)
	item->SetCount(count);

	item->m_BindMaster = ent->GetBindMaster();
	item->m_Orientated = ent->fl.bindOrientated;

	PutItem(item, category);

	rc = item;

Quit:
	if(del != -1)
		RemoveEntityFromMap(ent, del);

	return rc;
}

void CInventory::PutEntityInMap(idEntity *ent, idEntity *owner, CInventoryItem *item)
{
	if(ent == NULL || owner == NULL || item == NULL)
		return;

	// Make the item invisible
	ent->GetPhysics()->LinkClip();
	ent->Bind(item->m_BindMaster.GetEntity(), item->m_Orientated);
	ent->Show();

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
	if(item->IsDeletable() == true)
	{
		if(item->IsDroppable() == true)
			item->SetDeleteable(false);
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

CInventoryItem *CInventory::GetCurrentItem()
{
	CInventoryItem *rc = NULL;

	if(m_Category.Num() > 0)
		rc = m_Category[m_CurrentCategory]->GetItem(m_CurrentItem);

	return rc;
}

bool CInventory::SetCurrentItem(CInventoryItem *Item)
{
	bool rc = false;
	int group, item;

	if(Item == NULL)
	{
		if((Item = GetItem(TDM_DUMMY_ITEM)) == NULL)
			goto Quit;
	}

	if((group = GetCategoryItemIndex(Item, &item)) == -1)
		goto Quit;

	// Only change the group and item indizies, if they are valid.
	// Otherwise we might have an invalid index (-1).
	m_CurrentCategory = group;
	m_CurrentItem = item;

	rc = true;

Quit:
	return rc;
}

bool CInventory::SetCurrentItem(const char *Item)
{
	bool rc = false;
	int group, item;

	if(Item == NULL)
		goto Quit;

	if((group = GetCategoryItemIndex(Item, &item)) == -1)
		goto Quit;

	// Only change the group and item indizies, if they are valid.
	// Otherwise we might have an invalid index (-1).
	m_CurrentCategory = group;
	m_CurrentItem = item;

	rc = true;

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

void CInventory::ValidateCategory(void)
{
	int n = m_Category.Num();

	if(m_CurrentCategory >= n)
	{
		if(m_WrapAround == true)
		{
			if(m_CategoryLock == false)
			{
				m_CurrentItem = 0;
				m_CurrentCategory = 0;
			}
			else
			{
				if(n > 0)
					m_CurrentCategory = n-1;
				else
					m_CurrentCategory = 0;
			}
		}
		else
		{
			if(n > 0)
				m_CurrentCategory = n-1;
		}
	}
	else if(m_CurrentCategory < 0)
	{
		if(m_WrapAround == true)
		{
			if(m_CategoryLock == false)
			{
				if(n > 0)
					m_CurrentCategory = n-1;
				else
					m_CurrentCategory = 0;

				n = m_Category[m_CurrentCategory]->m_Item.Num();
				if(n > 0)
					m_CurrentItem = n-1;
				else
					m_CurrentItem = 0;
			}
			else
			{
				m_CurrentCategory = 0;
				n = m_Category[m_CurrentCategory]->m_Item.Num();
				if(n > 0)
					m_CurrentItem = n-1;
				else
					m_CurrentItem = 0;
			}
		}
		else
		{
			m_CurrentCategory = 0;
		}
	}
}

CInventoryItem *CInventory::GetNextItem(void)
{
	CInventoryItem *rc = NULL;
	int ni;

	ValidateCategory();
	ni = m_Category[m_CurrentCategory]->m_Item.Num();

	m_CurrentItem++;
	if(m_CurrentItem >= ni)
	{
		if(m_CategoryLock == false)
		{
			m_CurrentCategory++;
			ValidateCategory();
		}

		if(m_WrapAround == true)
			m_CurrentItem = 0;
		else 
		{
			m_CurrentItem = m_Category[m_CurrentCategory]->m_Item.Num()-1;
			goto Quit;
		}
	}

	rc = m_Category[m_CurrentCategory]->m_Item[m_CurrentItem];

Quit:
	return rc;
}

CInventoryItem *CInventory::GetPrevItem(void)
{
	CInventoryItem *rc = NULL;

	ValidateCategory();
	m_CurrentItem--;
	if(m_CurrentItem < 0)
	{
		if(m_CategoryLock == false)
		{
			m_CurrentCategory--;
			ValidateCategory();
		}

		if(m_WrapAround == true)
			m_CurrentItem = m_Category[m_CurrentCategory]->m_Item.Num()-1;
		else 
		{
			m_CurrentItem = 0;
			goto Quit;
		}
	}

	rc = m_Category[m_CurrentCategory]->m_Item[m_CurrentItem];

Quit:
	return rc;
}

CInventoryCategory *CInventory::GetNextCategory(void)
{
	ValidateCategory();
	m_CurrentCategory++;
	ValidateCategory();

	return m_Category[m_CurrentCategory];
}

CInventoryCategory *CInventory::GetPrevCategory(void)
{
	ValidateCategory();
	m_CurrentCategory--;
	ValidateCategory();

	return m_Category[m_CurrentCategory];
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

void CInventory::DropCurrentItem(void)
{
	CInventoryItem *item = GetCurrentItem();
	idEntity *ent = NULL;
	idEntity *owner = NULL;

	if(item && item->IsDroppable() == true)
	{
		ent = item->GetItemEntity();
		owner = item->GetOwner();
		PutEntityInMap(ent, owner, item);
	}

//	if(ent)
//		idThread* thread = useEnt->CallScriptFunctionArgs( "inventoryDrop", true, 0, "ee", useEnt, this );
}

////////////////////////
// CInventoryCategory //
////////////////////////

CInventoryCategory::CInventoryCategory(idEntity *owner, const char* name)
{
	m_Owner = owner;
	m_Name = name;
}

CInventoryCategory::~CInventoryCategory() 
{
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
		delete m_Item[i];
}

void CInventoryCategory::Save(idSaveGame *savefile) const
{
}

void CInventoryCategory::Restore(idRestoreGame *savefile)
{
}

void CInventoryCategory::SetOwner(idEntity *owner)
{
	int i, n;

	m_Owner = owner; 
	n = m_Item.Num();
	for(i = 0; i < n; i++)
		m_Item[i]->m_Owner = owner;
}

void CInventoryCategory::PutItem(CInventoryItem *item)
{
	if(item == NULL)
		goto Quit;

	item->m_Owner = m_Owner;
	item->m_Category = this;
	item->m_Inventory = m_Inventory;

	m_Item.AddUnique(item);

Quit:
	return;
}


CInventoryItem *CInventoryCategory::GetItem(int i)
{
	CInventoryItem *rc = NULL;

	if(i >= 0 && i < m_Item.Num())
		rc = m_Item[i];

	return rc;
}

CInventoryItem *CInventoryCategory::GetItem(const char *Name)
{
	CInventoryItem *rc = NULL;
	CInventoryItem *e;
	int i, n;

	if(Name == NULL || Name[0] == 0)
		goto Quit;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		e = m_Item[i];
		if(Name == e->m_Name)
		{
			rc = e;
			goto Quit;
		}
	}

Quit:
	return rc;
}

CInventoryItem *CInventoryCategory::GetItemById(const char *id)
{
	CInventoryItem *rc = NULL;
	CInventoryItem *e;
	int i, n;

	if(id == NULL || id[0] == 0)
		goto Quit;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		e = m_Item[i];
		if(id == e->m_ItemId)
		{
			rc = e;
			goto Quit;
		}
	}

Quit:
	return rc;
}

int CInventoryCategory::GetItemIndex(const idStr &Name)
{
	int rc = -1;
	CInventoryItem *e;
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		e = m_Item[i];
		if(Name == e->m_Name)
		{
			rc = i;
			goto Quit;
		}
	}

Quit:
	return rc;
}

int CInventoryCategory::GetItemIndex(CInventoryItem *it)
{
	int rc = -1;
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Item[i] == it)
		{
			rc = i;
			goto Quit;
		}
	}

Quit:
	return rc;
}

int CInventoryCategory::GetLoot(int &Gold, int &Jewelry, int &Goods)
{
	int i;
	CInventoryItem *it;

	for(i = 0; i < m_Item.Num(); i++)
	{
		it = m_Item[i];

		switch(it->GetLootType())
		{
			case CInventoryItem::LT_JEWELS:
				Jewelry += it->GetValue();
			break;

			case CInventoryItem::LT_GOLD:
				Gold += it->GetValue();
			break;

			case CInventoryItem::LT_GOODS:
				Goods += it->GetValue();
			break;
		}
	}

	return Gold + Jewelry + Goods;
}

///////////////////////
// CInventoryItem //
///////////////////////

CInventoryItem::CInventoryItem(idEntity *owner)
{
	m_Owner = owner;
	m_Item = NULL;
	m_Inventory = NULL;
	m_Category = NULL;
	m_Type = IT_ITEM;
	m_LootType = LT_NONE;
	m_Value = 0;
	m_Stackable = false;
	m_Count = 0;
	m_Droppable = false;
	m_Overlay = OVERLAYS_INVALID_HANDLE;
	m_Hud = false;
	m_Orientated = false;
	m_Deleteable = false;
}

CInventoryItem::~CInventoryItem()
{
	idEntity *e = m_Item.GetEntity();

	if(e != NULL)
		e->SetInventoryItem(NULL);
}

void CInventoryItem::Save( idSaveGame *savefile ) const
{
}

void CInventoryItem::Restore( idRestoreGame *savefile )
{
}

void CInventoryItem::SetLootType(CInventoryItem::LootType t)
{
	// Only positive values are allowed
	if(t >= CInventoryItem::LT_NONE && t <= CInventoryItem::LT_COUNT)
		m_LootType = t;
	else
		m_LootType = CInventoryItem::LT_NONE;
}

void CInventoryItem::SetValue(int n)
{
	// Only positive values are allowed
	if(n >= 0)
		m_Value = n;
}

void CInventoryItem::SetCount(int n)
{
	// Only positive values are allowed if stackable is true
	if(n >= 0 && m_Stackable == true)
		m_Count = n;
	else
		m_Count = 0;
}

void CInventoryItem::SetStackable(bool stack)
{
	if(stack == true || stack == false)
		m_Stackable = stack;
}

void CInventoryItem::SetHUD(const idStr &HudName, int layer)
{
	if(m_Overlay == OVERLAYS_INVALID_HANDLE || m_HudName != HudName)
	{
		idEntity *it;
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = owner->CreateOverlay(HudName, layer);
		if((it = m_Item.GetEntity()) != NULL)
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
	}
}

void CInventoryItem::SetOverlay(const idStr &HudName, int Overlay)
{
	if(Overlay != OVERLAYS_INVALID_HANDLE)
	{
		idEntity *it;
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = Overlay;
		if((it = m_Item.GetEntity()) != NULL)
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
	}
	else
		m_Hud = false;
}

void CInventoryItem::SetDeleteable(bool bDeleteable)
{
	if(m_Droppable == true)
		m_Deleteable = false;
	else
		m_Deleteable = bDeleteable;
}
