/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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

void tdmInventorySaveObjectList( idSaveGame *savefile )
{
	idLinkList<idClass>* iNode = tdmInventoryObjList.NextNode();
	while ( iNode != NULL ) {
		savefile->AddObject( iNode->Owner() );
		iNode = iNode->NextNode();
	}
}

///////////////////
// CtdmInventory //
///////////////////

CLASS_DECLARATION(idClass, CtdmInventory)
END_CLASS

CtdmInventory::CtdmInventory()
: idClass()
{
	m_Owner = NULL;
	CreateGroup(TDM_INVENTORY_DEFAULT_GROUP);		// We always have a defaultgroup if nothing else
	m_GroupLock = false;		// Default behaviour ...
	m_WrapAround = true;		// ... is like standard Thief inventory.
	m_CurrentGroup = 0;
	m_CurrentItem = 0;
}

CtdmInventory::~CtdmInventory()
{
	int i, n;

	n = m_Group.Num();
	for(i = 0; i < n; i++)
		delete m_Group[i];
}

void CtdmInventory::Save(idSaveGame *savefile) const
{
	// TODO: Has to call the groups and items as well.
}

void CtdmInventory::Restore(idRestoreGame *savefile)
{
	// TODO: Has to call the groups and items as well.
}

CtdmInventoryGroup *CtdmInventory::GetGroup(const char *pName, int *Index)
{
	CtdmInventoryGroup *rc = NULL;
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

CtdmInventoryGroup *CtdmInventory::CreateGroup(const char *Name, int *Index)
{
	CtdmInventoryGroup	*rc = NULL;

	if(Name == NULL)
		goto Quit;

	if((rc = GetGroup(Name, Index)) != NULL)
		goto Quit;

	if((rc = new CtdmInventoryGroup) == NULL)
		goto Quit;

	rc->SetInventory(this);
	rc->m_Owner = m_Owner.GetEntity();
	rc->m_Name = Name;
	m_Group.AddUnique(rc);

Quit:
	return rc;
}

int CtdmInventory::GetGroupIndex(const char *GroupName)
{
	int i = -1;

	GetGroup(GroupName, &i);

	return i;
}

void CtdmInventory::SetOwner(idEntity *Owner)
{
	int i, n;

	m_Owner = Owner; 
	n = m_Group.Num();
	for(i = 0; i < n; i++)
		m_Group[i]->SetOwner(Owner);
}

CtdmInventoryItem *CtdmInventory::PutItem(idEntity *Item, const idStr &Name, char const *Group)
{
	CtdmInventoryItem *rc = NULL;
	int i;
	CtdmInventoryGroup *gr;

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

void CtdmInventory::PutItem(CtdmInventoryItem *Item, char const *Group)
{
	int i;
	CtdmInventoryGroup *gr;

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

int CtdmInventory::GetGroupItemIndex(const char *ItemName, int *ItemIndex)
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

int CtdmInventory::GetGroupItemIndex(CtdmInventoryItem *Item, int *ItemIndex)
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

CtdmInventoryItem *CtdmInventory::GetCurrentItem()
{
	CtdmInventoryItem *rc = NULL;

	if(m_Group.Num() > 0)
		rc = m_Group[m_CurrentGroup]->GetItem(m_CurrentItem);

	return rc;
}

bool CtdmInventory::SetCurrentItem(CtdmInventoryItem *Item)
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

bool CtdmInventory::SetCurrentItem(const char *Item)
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

CtdmInventoryItem *CtdmInventory::GetItem(const char *Name, char const *Group)
{
	CtdmInventoryItem *rc = NULL;
	int i, n, s;
	CtdmInventoryGroup *gr;

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

void CtdmInventory::ValidateGroup(void)
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

CtdmInventoryItem *CtdmInventory::GetNextItem(void)
{
	CtdmInventoryItem *rc = NULL;
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

CtdmInventoryItem *CtdmInventory::GetPrevItem(void)
{
	CtdmInventoryItem *rc = NULL;

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

CtdmInventoryGroup *CtdmInventory::GetNextGroup(void)
{
	ValidateGroup();
	m_CurrentGroup++;
	ValidateGroup();

	return m_Group[m_CurrentGroup];
}

CtdmInventoryGroup *CtdmInventory::GetPrevGroup(void)
{
	ValidateGroup();
	m_CurrentGroup--;
	ValidateGroup();

	return m_Group[m_CurrentGroup];
}

int CtdmInventory::GetLoot(int &Gold, int &Jewelry, int &Goods)
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
// CtdmInventoryGroup //
////////////////////////

CtdmInventoryGroup::CtdmInventoryGroup(const char* name)
: idClass()
{
	m_Name = name;
}

CtdmInventoryGroup::~CtdmInventoryGroup() 
{
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
		delete m_Item[i];
}

void CtdmInventoryGroup::Save(idSaveGame *savefile) const
{
}

void CtdmInventoryGroup::Restore(idRestoreGame *savefile)
{
}

void CtdmInventoryGroup::SetOwner(idEntity *Owner)
{
	int i, n;

	m_Owner = Owner; 
	n = m_Item.Num();
	for(i = 0; i < n; i++)
		m_Item[i]->m_Owner = Owner;
}

CtdmInventoryItem *CtdmInventoryGroup::PutItem(idEntity *Item, const idStr &Name)
{
	CtdmInventoryItem *rc = NULL;

	if(Item == NULL || Name.Length() == 0)
		goto Quit;

	rc = new CtdmInventoryItem();
	m_Item.AddUnique(rc);
	rc->m_Owner = m_Owner.GetEntity();
	rc->m_Name = Name;
	rc->m_Item = Item;
	Item->SetInventoryItem(rc);

Quit:
	return rc;
}

void CtdmInventoryGroup::PutItem(CtdmInventoryItem *Item)
{
	if(Item == NULL)
		goto Quit;

	m_Item.AddUnique(Item);
	Item->m_Owner = m_Owner.GetEntity();

Quit:
	return;
}


CtdmInventoryItem *CtdmInventoryGroup::GetItem(int i)
{
	CtdmInventoryItem *rc = NULL;

	if(i >= 0 && i < m_Item.Num())
		rc = m_Item[i];

	return rc;
}

CtdmInventoryItem *CtdmInventoryGroup::GetItem(const idStr &Name)
{
	CtdmInventoryItem *rc = NULL;
	CtdmInventoryItem *e;
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

int CtdmInventoryGroup::GetItemIndex(const idStr &Name)
{
	int rc = -1;
	CtdmInventoryItem *e;
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

int CtdmInventoryGroup::GetItemIndex(CtdmInventoryItem *it)
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

int CtdmInventoryGroup::GetLoot(int &Gold, int &Jewelry, int &Goods)
{
	int i;
	CtdmInventoryItem *it;

	for(i = 0; i < m_Item.Num(); i++)
	{
		it = m_Item[i];

		switch(it->GetLootType())
		{
			case CtdmInventoryItem::JEWELS:
				Jewelry += it->GetValue();
			break;

			case CtdmInventoryItem::GOLD:
				Gold += it->GetValue();
			break;

			case CtdmInventoryItem::GOODS:
				Goods += it->GetValue();
			break;
		}
	}

	return Gold + Jewelry + Goods;
}

///////////////////////
// CtdmInventoryItem //
///////////////////////

CtdmInventoryItem::CtdmInventoryItem()
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

CtdmInventoryItem::~CtdmInventoryItem()
{
	idEntity *e = m_Item.GetEntity();

	if(e != NULL)
		e->SetInventoryItem(NULL);
}

void CtdmInventoryItem::Save( idSaveGame *savefile ) const
{
}

void CtdmInventoryItem::Restore( idRestoreGame *savefile )
{
}

void CtdmInventoryItem::SetLootType(CtdmInventoryItem::LootType t)
{
	// Only positive values are allowed
	if(t >= CtdmInventoryItem::NONE && t <= CtdmInventoryItem::COUNT)
		m_LootType = t;
	else
		m_LootType = CtdmInventoryItem::NONE;
}

void CtdmInventoryItem::SetValue(int n)
{
	// Only positive values are allowed
	if(n >= 0)
		m_Value = n;
}

void CtdmInventoryItem::SetCount(int n)
{
	// Only positive values are allowed if stackable is true
	if(n >= 0 && m_Stackable == true)
		m_Count = n;
	else
		m_Count = 0;
}

void CtdmInventoryItem::SetStackable(bool stack)
{
	if(stack == true || stack == false)
		m_Stackable = stack;
}

/////////////////////////
// CtdmInventoryCursor //
/////////////////////////

CLASS_DECLARATION( idClass, CtdmInventoryCursor )
	EVENT( EV_PostRestore,	CtdmInventoryCursor::Event_PostRestore )
END_CLASS

CtdmInventoryCursor::CtdmInventoryCursor()
{
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_node.SetOwner( this );
	m_inventory		= NULL;
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;
}

CtdmInventoryCursor::~CtdmInventoryCursor()
{
	setInventory( NULL );
}

CtdmInventoryCursor::CtdmInventoryCursor( const CtdmInventoryCursor& source )
{
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_node.SetOwner( this );
	m_inventory		= NULL;
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;

	*this = source;
}

CtdmInventoryCursor& CtdmInventoryCursor::operator = ( const CtdmInventoryCursor& source )
{
	if ( this == &source )
		goto Quit;

	// Copy everything except their histories.
	copyActiveCursor( source );

	// Copy over their histories.
	CtdmInventoryGroupHistory* groupHistory;

	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL )
	{
		groupHistory = new CtdmInventoryGroupHistory();
		if ( groupHistory == NULL )
		{
			gameLocal.Error("Unable to allocate memory for group history; group histories only partially copied.");
			goto Quit;
		}

		// Copy over the group history information.
		groupHistory->m_group = ghNode->Owner()->m_group;
		groupHistory->m_slot = ghNode->Owner()->m_slot;
		groupHistory->m_slotNode.InsertAfter( ghNode->Owner()->m_slotNode );
		groupHistory->m_node.AddToEnd( m_groupHistory );

		ghNode = ghNode->NextNode();
	}

Quit:
	return *this;
}

void CtdmInventoryCursor::Save( idSaveGame *savefile ) const
{
}

void CtdmInventoryCursor::Restore( idRestoreGame *savefile )
{
}

void CtdmInventoryCursor::Event_PostRestore()
{
}

/// Copies only the active cursor position, not any cursor histories.
void CtdmInventoryCursor::copyActiveCursor(	const CtdmInventoryCursor& source,
											bool noHistory )
{
	if ( this == &source )
		goto Quit;

	// Switch to the same inventory.
	setInventory( source.m_inventory );
	// Copy over their active cursor.
	if ( m_inventory != NULL )
		select( source.m_group, source.m_groupedSlot, source.m_ungroupedSlot, noHistory );

	assert( m_inventory == source.m_inventory );
	assert( m_group == source.m_group );
	assert( m_groupedSlot == source.m_groupedSlot );
	assert( m_ungroupedSlot == source.m_ungroupedSlot );

	Quit:
	return;
}

void CtdmInventoryCursor::setInventory( CtdmInventory* inventory )
{
}

// Consider inlining?
/// Returns the inventory this cursor points to.
CtdmInventory* CtdmInventoryCursor::inventory() const
{
	return m_inventory;
}

// Consider inlining?
/// Returns the name of the current inventory group. 'nullOk' causes it to return NULL instead of "" when outside any group.
const char* CtdmInventoryCursor::group( bool nullOk ) const
{
/*
	if ( m_group == NULL ) {
		return nullOk ? NULL : "";
	} else {
		return m_group->m_name.c_str();
	}
*/
	return NULL;
}

/// Selects a specific item. (note: cursor must be pointing to the correct inventory)
void CtdmInventoryCursor::selectItem( CtdmInventoryItem* item, bool noHistory )
{
}

// Consider inlining?
/// Returns the item this cursor is pointing to.
CtdmInventoryItem* CtdmInventoryCursor::item() const {
	return ( m_groupedSlot != NULL ) ? m_groupedSlot->m_item : NULL;
}

void CtdmInventoryCursor::iterate(	EtdmInventoryIterationMethod type,
									bool backwards,
									bool noHistory,
									bool (*filter)( CtdmInventoryItem* ) )
{
	if ( m_inventory == NULL )
	{
		gameLocal.Warning("iterate() called on an independant tdmInventoryCursor.");
		goto Quit;
	}

	switch (type)
	{
		case TDMINV_UNGROUPED:
			iterateUngrouped( backwards, noHistory, filter );
		break;
		case TDMINV_HYBRID:
			iterateHybrid( backwards, noHistory, filter );
		break;
		case TDMINV_GROUP:
			iterateGroup( backwards, noHistory, filter );
		break;
		case TDMINV_ITEM:
			iterateItem( backwards, noHistory, filter );
		break;
		default:
			gameLocal.Warning("Invalid inventory iteration type.");
	}

Quit:
	return;
}

/// Selects the given group and slots, updating reference counts.
void CtdmInventoryCursor::select(	CtdmInventoryGroup* group,
									CtdmInventorySlot* groupedSlot,
									CtdmInventorySlot* ungroupedSlot,
									bool noHistory )
{
}

/// Find a group history, if it exists. May return NULL.
CtdmInventoryGroupHistory* CtdmInventoryCursor::getGroupHistory( CtdmInventoryGroup* group ) const
{
	// Find the requested node. Maybe return NULL.
	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL && ghNode->Owner()->m_group != group )
		ghNode = ghNode->NextNode();

	return (ghNode != NULL) ? ghNode->Owner() : NULL;
}

/// Iterates without regard to groups.
void CtdmInventoryCursor::iterateUngrouped( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) )
{
}

/// Iterates with items grouped together.
void CtdmInventoryCursor::iterateHybrid( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) )
{
}

/// Iterates through the list of groups.
void CtdmInventoryCursor::iterateGroup( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) )
{
}

/// Iterates through the items in a group.
void CtdmInventoryCursor::iterateItem( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) ) 
{
}


///////////////////////
// CtdmInventorySlot //
///////////////////////

#ifdef TDMINVENTORY_DEBUG
int CtdmInventorySlot::numSlots = 0;
#endif

CtdmInventorySlot::CtdmInventorySlot( CtdmInventoryItem* item )
{
	m_node.SetOwner( this );
	m_item = item;
	m_numCursors = 0;

#ifdef TDMINVENTORY_DEBUG
	numSlots++;
#endif
}

#ifdef TDMINVENTORY_DEBUG
CtdmInventorySlot::~CtdmInventorySlot()
{
	numSlots--;
}

int CtdmInventorySlot::slots() {
	return numSlots;
}
#endif

///////////////////////////////
// CtdmInventoryGroupHistory //
///////////////////////////////

CtdmInventoryGroupHistory::CtdmInventoryGroupHistory( CtdmInventoryGroup* group )
{
	m_node.SetOwner( this );
	m_slotNode.SetOwner( this );
	m_group = group;
	m_slot = NULL;
}
