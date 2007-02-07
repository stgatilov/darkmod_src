/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../game/Game_local.h"

#include "tdmInventory.h"

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
	if(pName == NULL)
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

	if(Name == NULL)
		goto Quit;

	if((rc = GetCategory(Name, Index)) != NULL)
		goto Quit;

	if((rc = new CInventoryCategory) == NULL)
		goto Quit;

	rc->SetInventory(this);
	rc->m_Owner = m_Owner.GetEntity();
	rc->m_Name = Name;
	m_Category.AddUnique(rc);

Quit:
	return rc;
}

int CInventory::GetCategoryIndex(const char *CategoryName)
{
	int i = -1;

	GetCategory(CategoryName, &i);

	return i;
}

void CInventory::SetOwner(idEntity *Owner)
{
	int i, n;

	m_Owner = Owner; 
	n = m_Category.Num();
	for(i = 0; i < n; i++)
		m_Category[i]->SetOwner(Owner);
}

CInventoryItem *CInventory::PutItem(idEntity *Item, const idStr &Name, char const *Category)
{
	CInventoryItem *rc = NULL;
	int i;
	CInventoryCategory *gr;

	if(Item == NULL || Name.Length() == 0)
		goto Quit;

	// Check if it is the default group or not.
	if(Category != NULL)
		gr = m_Category[0];
	else
	{
		gr = GetCategory(Category, &i);
		if(gr == NULL)
			goto Quit;
	}

	rc = gr->PutItem(Item, Name);

Quit:
	return rc;
}

void CInventory::PutItem(CInventoryItem *Item, char const *Category)
{
	int i;
	CInventoryCategory *gr;

	if(Item == NULL)
		goto Quit;

	// Check if it is the default group or not.
	if(Category != NULL)
		gr = m_Category[0];
	else
	{
		gr = GetCategory(Category, &i);
		if(gr == NULL)
			goto Quit;
	}

	gr->PutItem(Item);

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

CInventoryItem *CInventory::GetItem(const char *Name, char const *Category)
{
	CInventoryItem *rc = NULL;
	int i, n, s;
	CInventoryCategory *gr;

	if(Category == NULL)
	{
		n = m_Category.Num();
		s = 0;
	}
	else
	{
		gr = GetCategory(Category, &i);
		if(gr == NULL)
			goto Quit;

		n = i;
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
	int total = 0;
	int i;

	Gold = 0;
	Jewelry = 0;
	Goods = 0;

	for(i = 0; i < m_Category.Num(); i++)
		total += m_Category[i]->GetLoot(Gold, Jewelry, Goods);

	return total;
}


////////////////////////
// CInventoryCategory //
////////////////////////

CInventoryCategory::CInventoryCategory(const char* name)
{
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

void CInventoryCategory::SetOwner(idEntity *Owner)
{
	int i, n;

	m_Owner = Owner; 
	n = m_Item.Num();
	for(i = 0; i < n; i++)
		m_Item[i]->m_Owner = Owner;
}

CInventoryItem *CInventoryCategory::PutItem(idEntity *Item, const idStr &Name)
{
	CInventoryItem *rc = NULL;

	if(Item == NULL || Name.Length() == 0)
		goto Quit;

	rc = new CInventoryItem(m_Owner.GetEntity());
	m_Item.AddUnique(rc);
	rc->m_Name = Name;
	rc->m_Item = Item;
	Item->SetInventoryItem(rc);

Quit:
	return rc;
}

void CInventoryCategory::PutItem(CInventoryItem *Item)
{
	if(Item == NULL)
		goto Quit;

	m_Item.AddUnique(Item);
	Item->m_Owner = m_Owner.GetEntity();

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

CInventoryItem *CInventoryCategory::GetItem(const idStr &Name)
{
	CInventoryItem *rc = NULL;
	CInventoryItem *e;
	int i, n;

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

