/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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
 * Groups and inventories are not cleared even if they are empty. Only when 
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


/*
	Add an item from the map to the inventory.

	idEntity *ent;

		ent->Unbind();
		ent->GetPhysics()->PutToRest();
// TODO: don't forget to re-link the clipmodel if we drop the item later
		ent->GetPhysics()->UnlinkClip();
		ent->Hide();
*/

///////////////////
// CInventory //
///////////////////

CLASS_DECLARATION(idClass, CInventory)
END_CLASS

CInventory::CInventory()
: idClass()
{
	m_Owner = NULL;
	CreateGroup(TDM_INVENTORY_DEFAULT_GROUP);		// We always have a defaultgroup if nothing else
	m_GroupLock = false;		// Default behaviour ...
	m_WrapAround = true;		// ... is like standard Thief inventory.
	m_CurrentGroup = 0;
	m_CurrentItem = 0;
}

CInventory::~CInventory()
{
	int i, n;

	n = m_Group.Num();
	for(i = 0; i < n; i++)
		delete m_Group[i];
}

void CInventory::Save(idSaveGame *savefile) const
{
	// TODO: Has to call the groups and items as well.
}

void CInventory::Restore(idRestoreGame *savefile)
{
	// TODO: Has to call the groups and items as well.
}

CInventoryGroup *CInventory::GetGroup(const char *pName, int *Index)
{
	CInventoryGroup *rc = NULL;
	int i, n;

	// If the groupname is null we look for the default group
	if(pName == NULL)
		return GetGroup(TDM_INVENTORY_DEFAULT_GROUP);

	n = m_Group.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Group[i]->m_Name.Cmp(pName) == 0)
		{
			rc = m_Group[i];
			if(Index != NULL)
				*Index = i;

			goto Quit;
		}
	}

Quit:
	return rc;
}

CInventoryGroup *CInventory::CreateGroup(const char *Name, int *Index)
{
	CInventoryGroup	*rc = NULL;

	if(Name == NULL)
		goto Quit;

	if((rc = GetGroup(Name, Index)) != NULL)
		goto Quit;

	if((rc = new CInventoryGroup) == NULL)
		goto Quit;

	rc->SetInventory(this);
	rc->m_Owner = m_Owner.GetEntity();
	rc->m_Name = Name;
	m_Group.AddUnique(rc);

Quit:
	return rc;
}

int CInventory::GetGroupIndex(const char *GroupName)
{
	int i = -1;

	GetGroup(GroupName, &i);

	return i;
}

void CInventory::SetOwner(idEntity *Owner)
{
	int i, n;

	m_Owner = Owner; 
	n = m_Group.Num();
	for(i = 0; i < n; i++)
		m_Group[i]->SetOwner(Owner);
}

CInventoryItem *CInventory::PutItem(idEntity *Item, const idStr &Name, char const *Group)
{
	CInventoryItem *rc = NULL;
	int i;
	CInventoryGroup *gr;

	if(Item == NULL || Name.Length() == 0)
		goto Quit;

	// Check if it is the default group or not.
	if(Group != NULL)
		gr = m_Group[0];
	else
	{
		gr = GetGroup(Group, &i);
		if(gr == NULL)
			goto Quit;
	}

	rc = gr->PutItem(Item, Name);

Quit:
	return rc;
}

void CInventory::PutItem(CInventoryItem *Item, char const *Group)
{
	int i;
	CInventoryGroup *gr;

	if(Item == NULL)
		goto Quit;

	// Check if it is the default group or not.
	if(Group != NULL)
		gr = m_Group[0];
	else
	{
		gr = GetGroup(Group, &i);
		if(gr == NULL)
			goto Quit;
	}

	gr->PutItem(Item);

Quit:
	return;
}

int CInventory::GetGroupItemIndex(const char *ItemName, int *ItemIndex)
{
	int rc = -1;
	int i;
	int n = -1;

	if(ItemIndex != NULL)
		*ItemIndex = -1;

	if(ItemName == NULL)
		goto Quit;

	for(i = 0; i < m_Group.Num(); i++)
	{
		if((n = m_Group[i]->GetItemIndex(ItemName)) != -1)
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

int CInventory::GetGroupItemIndex(CInventoryItem *Item, int *ItemIndex)
{
	int rc = -1;
	int i;
	int n = -1;

	if(ItemIndex != NULL)
		*ItemIndex = -1;

	for(i = 0; i < m_Group.Num(); i++)
	{
		if((n = m_Group[i]->GetItemIndex(Item)) != -1)
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

	if(m_Group.Num() > 0)
		rc = m_Group[m_CurrentGroup]->GetItem(m_CurrentItem);

	return rc;
}

bool CInventory::SetCurrentItem(CInventoryItem *Item)
{
	bool rc = false;
	int group, item;

	if(Item == NULL)
		goto Quit;

	if((group = GetGroupItemIndex(Item, &item)) == -1)
		goto Quit;

	// Only change the group and item indizies, if they are valid.
	// Otherwise we might have an invalid index (-1).
	m_CurrentGroup = group;
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

	if((group = GetGroupItemIndex(Item, &item)) == -1)
		goto Quit;

	// Only change the group and item indizies, if they are valid.
	// Otherwise we might have an invalid index (-1).
	m_CurrentGroup = group;
	m_CurrentItem = item;

	rc = true;

Quit:
	return rc;
}

CInventoryItem *CInventory::GetItem(const char *Name, char const *Group)
{
	CInventoryItem *rc = NULL;
	int i, n, s;
	CInventoryGroup *gr;

	if(Group == NULL)
	{
		n = m_Group.Num();
		s = 0;
	}
	else
	{
		gr = GetGroup(Group, &i);
		if(gr == NULL)
			goto Quit;

		n = i;
		s = i;
	}

	for(i = s; i < n; i++)
	{
		gr = m_Group[i];
		if((rc = gr->GetItem(Name)) != NULL)
			goto Quit;
	}

Quit:
	return rc;
}

void CInventory::ValidateGroup(void)
{
	int n = m_Group.Num();

	if(m_CurrentGroup >= n)
	{
		if(m_WrapAround == true)
		{
			if(m_GroupLock == false)
			{
				m_CurrentItem = 0;
				m_CurrentGroup = 0;
			}
			else
			{
				if(n > 0)
					m_CurrentGroup = n-1;
				else
					m_CurrentGroup = 0;
			}
		}
		else
		{
			if(n > 0)
				m_CurrentGroup = n-1;
		}
	}
	else if(m_CurrentGroup < 0)
	{
		if(m_WrapAround == true)
		{
			if(m_GroupLock == false)
			{
				if(n > 0)
					m_CurrentGroup = n-1;
				else
					m_CurrentGroup = 0;

				n = m_Group[m_CurrentGroup]->m_Item.Num();
				if(n > 0)
					m_CurrentItem = n-1;
				else
					m_CurrentItem = 0;
			}
			else
			{
				m_CurrentGroup = 0;
				n = m_Group[m_CurrentGroup]->m_Item.Num();
				if(n > 0)
					m_CurrentItem = n-1;
				else
					m_CurrentItem = 0;
			}
		}
		else
		{
			m_CurrentGroup = 0;
		}
	}
}

CInventoryItem *CInventory::GetNextItem(void)
{
	CInventoryItem *rc = NULL;
	int ni;

	ValidateGroup();
	ni = m_Group[m_CurrentGroup]->m_Item.Num();

	m_CurrentItem++;
	if(m_CurrentItem >= ni)
	{
		if(m_GroupLock == false)
		{
			m_CurrentGroup++;
			ValidateGroup();
		}

		if(m_WrapAround == true)
			m_CurrentItem = 0;
		else 
		{
			m_CurrentItem = m_Group[m_CurrentGroup]->m_Item.Num()-1;
			goto Quit;
		}
	}

	rc = m_Group[m_CurrentGroup]->m_Item[m_CurrentItem];

Quit:
	return rc;
}

CInventoryItem *CInventory::GetPrevItem(void)
{
	CInventoryItem *rc = NULL;

	ValidateGroup();
	m_CurrentItem--;
	if(m_CurrentItem < 0)
	{
		if(m_GroupLock == false)
		{
			m_CurrentGroup--;
			ValidateGroup();
		}

		if(m_WrapAround == true)
			m_CurrentItem = m_Group[m_CurrentGroup]->m_Item.Num()-1;
		else 
		{
			m_CurrentItem = 0;
			goto Quit;
		}
	}

	rc = m_Group[m_CurrentGroup]->m_Item[m_CurrentItem];

Quit:
	return rc;
}

CInventoryGroup *CInventory::GetNextGroup(void)
{
	ValidateGroup();
	m_CurrentGroup++;
	ValidateGroup();

	return m_Group[m_CurrentGroup];
}

CInventoryGroup *CInventory::GetPrevGroup(void)
{
	ValidateGroup();
	m_CurrentGroup--;
	ValidateGroup();

	return m_Group[m_CurrentGroup];
}

int CInventory::GetLoot(int &Gold, int &Jewelry, int &Goods)
{
	int total = 0;
	int i;

	Gold = 0;
	Jewelry = 0;
	Goods = 0;

	for(i = 0; i < m_Group.Num(); i++)
		total += m_Group[i]->GetLoot(Gold, Jewelry, Goods);

	return total;
}


////////////////////////
// CInventoryGroup //
////////////////////////

CInventoryGroup::CInventoryGroup(const char* name)
{
	m_Name = name;
}

CInventoryGroup::~CInventoryGroup() 
{
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
		delete m_Item[i];
}

void CInventoryGroup::Save(idSaveGame *savefile) const
{
}

void CInventoryGroup::Restore(idRestoreGame *savefile)
{
}

void CInventoryGroup::SetOwner(idEntity *Owner)
{
	int i, n;

	m_Owner = Owner; 
	n = m_Item.Num();
	for(i = 0; i < n; i++)
		m_Item[i]->m_Owner = Owner;
}

CInventoryItem *CInventoryGroup::PutItem(idEntity *Item, const idStr &Name)
{
	CInventoryItem *rc = NULL;

	if(Item == NULL || Name.Length() == 0)
		goto Quit;

	rc = new CInventoryItem();
	m_Item.AddUnique(rc);
	rc->m_Owner = m_Owner.GetEntity();
	rc->m_Name = Name;
	rc->m_Item = Item;
	Item->SetInventoryItem(rc);

Quit:
	return rc;
}

void CInventoryGroup::PutItem(CInventoryItem *Item)
{
	if(Item == NULL)
		goto Quit;

	m_Item.AddUnique(Item);
	Item->m_Owner = m_Owner.GetEntity();

Quit:
	return;
}


CInventoryItem *CInventoryGroup::GetItem(int i)
{
	CInventoryItem *rc = NULL;

	if(i >= 0 && i < m_Item.Num())
		rc = m_Item[i];

	return rc;
}

CInventoryItem *CInventoryGroup::GetItem(const idStr &Name)
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

int CInventoryGroup::GetItemIndex(const idStr &Name)
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

int CInventoryGroup::GetItemIndex(CInventoryItem *it)
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

int CInventoryGroup::GetLoot(int &Gold, int &Jewelry, int &Goods)
{
	int i;
	CInventoryItem *it;

	for(i = 0; i < m_Item.Num(); i++)
	{
		it = m_Item[i];

		switch(it->GetLootType())
		{
			case CInventoryItem::JEWELS:
				Jewelry += it->GetValue();
			break;

			case CInventoryItem::GOLD:
				Gold += it->GetValue();
			break;

			case CInventoryItem::GOODS:
				Goods += it->GetValue();
			break;
		}
	}

	return Gold + Jewelry + Goods;
}

///////////////////////
// CInventoryItem //
///////////////////////

CInventoryItem::CInventoryItem()
{
	m_Owner = NULL;
	m_Item = NULL;
	m_Inventory = NULL;
	m_Group = NULL;
	m_Type = ITEM;
	m_LootType = NONE;
	m_Value = 0;
	m_Stackable = false;
	m_Count = 0;
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
	if(t >= CInventoryItem::NONE && t <= CInventoryItem::COUNT)
		m_LootType = t;
	else
		m_LootType = CInventoryItem::NONE;
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

