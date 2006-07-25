/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../game/Game_local.h"

#include "tdmInventory.h"

const idEventDef EV_PostRestore( "postRestore", NULL );

static idLinkList<idClass>	tdmInventoryObjList;

void tdmInventorySaveObjectList( idSaveGame *savefile ) {

	idLinkList<idClass>* iNode = tdmInventoryObjList.NextNode();
	while ( iNode != NULL ) {
		savefile->AddObject( iNode->Owner() );
		iNode = iNode->NextNode();
	}

}


  ///////////////////
 // CtdmInventory //
///////////////////

CLASS_DECLARATION( idClass, CtdmInventory )
END_CLASS

CtdmInventory::CtdmInventory() {
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );
}

CtdmInventory::~CtdmInventory() {
	// Remove all cursors.
	while ( m_cursors.NextNode() ) {
		m_cursors.NextNode()->Owner()->setInventory( NULL );
	}

	// Remove all items.
	// Because all cursors have been removed, every inventory slot
	// should contain an item.
	while ( m_itemList.NextNode() ) {
		m_itemList.NextNode()->Owner()->m_item->setInventory( NULL );
	}
}

void CtdmInventory::Save( idSaveGame *savefile ) const {

	m_owner.Save( savefile );

	// Write out our ungrouped slots.
	savefile->WriteInt( m_itemList.Num() );
	idLinkList<CtdmInventorySlot>* sNode = m_itemList.NextNode();
	while ( sNode != NULL ) {
		savefile->WriteObject( sNode->Owner()->m_item );
		sNode = sNode->NextNode();
	}

	// Write out our groups and grouped slots.
	savefile->WriteInt( m_groupList.Num() );
	idLinkList<CtdmInventoryGroup>* gNode = m_groupList.NextNode();
	while ( gNode != NULL ) {

		savefile->WriteString( gNode->Owner()->m_name );

		// Write out the group's slots.
		savefile->WriteInt( gNode->Owner()->m_itemList.Num() );
		sNode = gNode->Owner()->m_itemList.NextNode();
		while ( sNode != NULL ) {
			savefile->WriteObject( sNode->Owner()->m_item );
			sNode = sNode->NextNode();
		}

		gNode = gNode->NextNode();
	}
}

void CtdmInventory::Restore( idRestoreGame *savefile ) {
	unsigned int numSlots;
	unsigned int numGroups;
	CtdmInventorySlot* slot;

	m_owner.Restore( savefile );

	// Read in our ungrouped slots.
	savefile->ReadInt( reinterpret_cast<int &>( numSlots ) );
	while ( numSlots-- ) {

		slot = new CtdmInventorySlot();
		if ( slot == NULL ) {
			gameLocal.Error("Unable to allocate memory for ungrouped slot.");
			goto Quit;
		}

		// Load the slot's item.
		savefile->ReadObject( reinterpret_cast<idClass *&>( slot->m_item ) );

		slot->m_node.AddToEnd( m_itemList );
	}

	CtdmInventoryGroup* group;

	// Read in our groups and grouped slots.
	savefile->ReadInt( reinterpret_cast<int &>( numGroups ) );
	while ( numGroups-- ) {

		group = new CtdmInventoryGroup();
		if ( group == NULL ) {
			gameLocal.Error("Unable to allocate memory for group.");
			goto Quit;
		}

		// Setup the new group.
		savefile->ReadString( group->m_name );
		group->m_node.AddToEnd( m_groupList );

		// Read in the group's slots.
		savefile->ReadInt( reinterpret_cast<int &>( numSlots ) );
		while ( numSlots-- ) {

			slot = new CtdmInventorySlot();
			if ( slot == NULL ) {
				gameLocal.Error("Unable to allocate memory for ungrouped slot.");
				goto Quit;
			}

			// Load the slot's item.
			savefile->ReadObject( reinterpret_cast<idClass *&>( slot->m_item ) );

			slot->m_node.AddToEnd( group->m_itemList );
			if ( slot->m_item != NULL ) {
				group->m_numItems++;
			}

		}

	}

	Quit:
	return;
}

/// Return the group with the given name. Create it if neccessary.
CtdmInventoryGroup* CtdmInventory::obtainGroup( const char* groupName )
{
	// Try to find the requested group.
	// This loop will either set gNode to the requested group,
	// or the group node we would need to add a new group before.
	idLinkList<CtdmInventoryGroup>* gNode = m_groupList.NextNode();
	while ( gNode != NULL && gNode->Owner()->m_name.Cmp( groupName ) < 0 ) {
		gNode = gNode->NextNode();
	}

	CtdmInventoryGroup* group = NULL;

	// Did we find our correct group?
	if ( gNode != NULL && gNode->Owner()->m_name.Cmp( groupName ) == 0 ) {

		// We found the group. Let's return it.
		group = gNode->Owner();

	} else {

		// No, we need to create a new group of the correct type, and add it.

		// Alloc a new group.
		group = new CtdmInventoryGroup( groupName );
		if ( group == NULL ) {
			gameLocal.Error("Unable to allocate memory for inventory group.");
			goto Quit;
		}

		// Insert the group into the inventory.
		if ( gNode != NULL ) {
			group->m_node.InsertBefore( *gNode );
		} else {
			group->m_node.AddToEnd( m_groupList );
		}

	}
	Quit:
	return group;
}

/// Checks if an ungrouped slot should be deallocated.
void CtdmInventory::check( CtdmInventorySlot* slot ) {
	// If the slot is empty and no cursors depend on it, delete it.
	if ( slot->m_item == NULL && slot->m_numCursors == 0 ) {
		delete slot;
	}
}

/// Checks if a group and slot should be deallocated.
void CtdmInventory::check( CtdmInventoryGroup* group, CtdmInventorySlot* slot ) {
	if ( slot != NULL ) {
		// Possible optimization: Potentially move history cursors to
		// previous slot, if both are empty? Note: If you implement it,
		// be sure to check over the cursor code to ensure it doesn't
		// fubar it.

		// If the slot is empty and no cursors depend on it, delete it.
		if ( slot->m_item == NULL &&
			 slot->m_numCursors == 0 &&
			 slot->m_historyCursors.IsListEmpty() ) {
			delete slot;
		}
	}

	// If the group is empty and has no active cursors, remove it.
	if ( group->m_numItems == 0 && group->m_numCursors == 0 ) {
		delete group;
	}
}

/// Converts a slot into an index.
unsigned int CtdmInventory::SlotToIndex( const CtdmInventorySlot* slot ) const {
	if ( slot == NULL ) {
		return 0;
	}

	int index = 1;

	idLinkList<CtdmInventorySlot>* sNode = slot->m_node.PrevNode();
	while ( sNode != NULL ) {
		index++;
		sNode = sNode->PrevNode();
	}

	return index;
}

/// Converts an index into a slot.
CtdmInventorySlot* CtdmInventory::IndexToSlot( unsigned int index, const CtdmInventoryGroup* group ) const {
	if ( index == 0 ) {
		return NULL;
	}

	idLinkList<CtdmInventorySlot>* sNode;
	if ( group == NULL ) {
		sNode = m_itemList.NextNode();
	} else {
		sNode = group->m_itemList.NextNode();
	}
	while ( --index ) {
		sNode = sNode->NextNode();
	}

	return sNode->Owner();
}

/// Converts a group into an index.
unsigned int CtdmInventory::GroupToIndex( const CtdmInventoryGroup* group ) const {
	if ( group == NULL ) {
		return 0;
	}

	int index = 1;

	idLinkList<CtdmInventoryGroup>* gNode = group->m_node.PrevNode();
	while ( gNode != NULL ) {
		index++;
		gNode = gNode->PrevNode();
	}

	return index;
}

/// Converts an index into a group.
CtdmInventoryGroup* CtdmInventory::IndexToGroup( unsigned int index ) const {
	if ( index == 0 ) {
		return NULL;
	}

	idLinkList<CtdmInventoryGroup>* gNode = m_groupList.NextNode();
	while ( --index ) {
		gNode = gNode->NextNode();
	}

	return gNode->Owner();
}


  ///////////////////////
 // CtdmInventoryItem //
///////////////////////

CLASS_DECLARATION( idClass, CtdmInventoryItem )
	EVENT( EV_PostRestore,	CtdmInventoryItem::Event_PostRestore )
END_CLASS

CtdmInventoryItem::CtdmInventoryItem() {
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_inventory		= NULL;
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;
}

CtdmInventoryItem::~CtdmInventoryItem() {
	setInventory( NULL );
}

void CtdmInventoryItem::Save( idSaveGame *savefile ) const {
	m_owner.Save( savefile );
	savefile->WriteObject( m_inventory );
	savefile->WriteInt( m_inventory->GroupToIndex( m_group ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_groupedSlot ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_ungroupedSlot ) );
}

void CtdmInventoryItem::Restore( idRestoreGame *savefile ) {
	m_owner.Restore( savefile );
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_inventory ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_groupNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_groupedSlotNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_ungroupedSlotNum ) );
	PostEventMS( &EV_PostRestore, 0 );
}

void CtdmInventoryItem::Event_PostRestore() {
	m_group = m_inventory->IndexToGroup( m_groupNum );
	m_groupedSlot = m_inventory->IndexToSlot( m_groupedSlotNum, m_group );
	m_ungroupedSlot = m_inventory->IndexToSlot( m_ungroupedSlotNum );
}

/// Puts the item in a specific location in an inventory.
/**	This is used to add/remove an item to/from an inventory, or move it
 *	around within an inventory.
 *	
 *	When called, setInventory() moves the item into 'inventory', in the
 *	group of 'groupName'. Its ungrouped position is immediately before or
 *	after the slot referenced by 'ungroupedItem'. Its grouped position is
 *	immediately before or after the slot referenced by 'groupedItem'.
 *	
 *	If 'groupName' is NULL and 'groupedItem' is NULL, 'groupName' is
 *	assumed to be "". If 'groupName' is NULL, but 'groupedItem' is not
 *	NULL, 'groupName' is assumed to refer to the same group as
 *	'groupedItem'.
 *	
 *	If 'ungroupedItem' is NULL, it is assumed to reference an imaginary
 *	slot that is (circularly) between the last and first slots of the
 *	inventory. If 'groupedItem' is NULL, it is assumed to reference an
 *	imaginary slot that is (circularly) between the last and first slots
 *	of the group referenced by 'groupName'.
 *	
 *	The average usage of setInventory is as follows:
 *		item.setInventory( inventory, group );
 *	That will place 'item' in 'group' of 'inventory', at the end.
 *
 *	It is an error if 'ungroupedItem' is non-NULL, and not in 'inventory'.
 *	It is an error if 'groupedItem' is non-NULL, and not in 'inventory'.
 *	It is an error if 'groupName' and 'groupedItem' are non-NULL, and
 *	'groupName' isn't the group of 'groupedItem'.
 */
void CtdmInventoryItem::setInventory(	CtdmInventory* inventory,
										const char* groupName,
										bool afterUngrouped,
										CtdmInventoryItem* ungroupedItem,
										bool afterGrouped,
										CtdmInventoryItem* groupedItem ) {

	CtdmInventorySlot* ungroupedSlot = NULL;
	CtdmInventorySlot* groupedSlot = NULL;
	CtdmInventoryGroup*	group;

	if ( ungroupedItem != NULL && ungroupedItem->m_inventory != inventory ) {
		gameLocal.Warning("Ungrouped target item isn't in target inventory.");
		goto Quit;
	}
	if ( groupedItem != NULL && groupedItem->m_inventory != inventory ) {
		gameLocal.Warning("Grouped target item isn't in target inventory.");
		goto Quit;
	}
	if ( groupedItem != NULL && groupName != NULL &&
		 groupedItem->m_group->m_name.Cmp( groupName ) != 0 ) {
		gameLocal.Warning("Grouped target is in a different group.");
		goto Quit;
	}

	// If we're moving to another (or the same) inventory, prepare the
	// destination slots.
	if ( inventory != NULL )
	{
		ungroupedSlot = new CtdmInventorySlot( this );
		groupedSlot   = new CtdmInventorySlot( this );
		if ( ungroupedSlot == NULL || groupedSlot == NULL ) {
			gameLocal.Error("Unable to allocate memory for inventory slot.");
			goto Quit;
		}

		// If the group name is omitted, default to groupedItem's group, or
		// if that's omitted, default to "".
		if ( groupName == NULL ) {
			groupName = (groupedItem != NULL) ? groupedItem->m_group->m_name.c_str() : "";
		}

		// Obtain the inventory group we'll be put in.
		group = inventory->obtainGroup( groupName );
		if ( group == NULL ) {
			// I'm assuming obtainGroup would have already made an error
			// message, so this doesn't need to.
			//gameLocal.Error("Unable to allocate memory for inventory group.");
			goto Quit;
		}

		// Insert the ungrouped slot in the correct spot of the inventory.
		if ( ungroupedItem != NULL ) {
			if ( afterUngrouped ) {
				ungroupedSlot->m_node.InsertAfter( ungroupedItem->m_ungroupedSlot->m_node );
			} else {
				ungroupedSlot->m_node.InsertBefore( ungroupedItem->m_ungroupedSlot->m_node );
			}
		} else {
			if ( afterUngrouped ) {
				ungroupedSlot->m_node.AddToFront( inventory->m_itemList );
			} else {
				ungroupedSlot->m_node.AddToEnd( inventory->m_itemList );
			}
		}
		// Insert the grouped slot in the correct spot of the inventory.
		if ( groupedItem != NULL ) {
			if ( afterGrouped ) {
				groupedSlot->m_node.InsertAfter( groupedItem->m_groupedSlot->m_node );
			} else {
				groupedSlot->m_node.InsertBefore( groupedItem->m_groupedSlot->m_node );
			}
		} else {
			if ( afterGrouped ) {
				groupedSlot->m_node.AddToFront( group->m_itemList );
			} else {
				groupedSlot->m_node.AddToEnd( group->m_itemList );
			}
		}

		group->m_numItems++;

		// At this point, we may potentially have two slots in a group pointing to
		// us, but that's ok... there'll only be one slot by the end of this function.
	}

	if ( m_inventory != NULL ) {
		// Remove ourself from our current inventory.

		m_groupedSlot->m_item = NULL;
		m_ungroupedSlot->m_item = NULL;
		m_group->m_numItems--;
		m_inventory->check( m_ungroupedSlot );
		m_inventory->check( m_group, m_groupedSlot );

		m_inventory		= NULL;
		m_group			= NULL;
		m_groupedSlot	= NULL;
		m_ungroupedSlot	= NULL;
	}

	if ( inventory != NULL ) {
		// We know that inventory, group, ungroupedSlot and groupedSlot
		// are non-null and don't point to garbage, due to the code
		// in the previous (inventory!=NULL) block.
		m_inventory		= inventory;
		m_group			= group;
		m_ungroupedSlot	= ungroupedSlot;
		m_groupedSlot	= groupedSlot;

		// Since we've gotten this far, the new slots we've created
		// should not be cleaned up.
		ungroupedSlot = NULL;
		groupedSlot = NULL;
	}

	// Add message propagation code.

	Quit:
	if ( ungroupedSlot != NULL ) {
		delete ungroupedSlot;
		ungroupedSlot = NULL;
	}
	if ( groupedSlot != NULL ) {
		delete groupedSlot;
		groupedSlot = NULL;
	}
}

/// Replaces one item with another.
void CtdmInventoryItem::replaceItem( CtdmInventoryItem* item ) {

	// If we're in an inventory, exit it.
	if ( m_inventory != NULL ) {
		setInventory( NULL );
	}

	// Make sure there's actually work to be done.
	if ( item == NULL || item->m_inventory == NULL ) {
		goto Quit;
	}

	// Replace the given item.
	m_inventory		= item->m_inventory;
	m_group			= item->m_group;
	m_ungroupedSlot	= item->m_ungroupedSlot;
	m_groupedSlot	= item->m_groupedSlot;

	m_ungroupedSlot->m_item = this;
	m_groupedSlot->m_item = this;

	item->m_inventory		= NULL;
	item->m_group			= NULL;
	item->m_ungroupedSlot	= NULL;
	item->m_groupedSlot		= NULL;

	// Add message propagation code.

	Quit:
	return;
}

// Consider inlining?
/// Returns the inventory this item is contained by.
CtdmInventory* CtdmInventoryItem::inventory() const {
	return m_inventory;
}

// Consider inlining?
/// Gets the item's group.
const char* CtdmInventoryItem::group() const {
	return m_group->m_name.c_str();
}


  /////////////////////////
 // CtdmInventoryCursor //
/////////////////////////

CLASS_DECLARATION( idClass, CtdmInventoryCursor )
	EVENT( EV_PostRestore,	CtdmInventoryCursor::Event_PostRestore )
END_CLASS

CtdmInventoryCursor::CtdmInventoryCursor() {
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_node.SetOwner( this );
	m_inventory		= NULL;
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;
}

CtdmInventoryCursor::~CtdmInventoryCursor() {
	setInventory( NULL );
}

CtdmInventoryCursor::CtdmInventoryCursor( const CtdmInventoryCursor& source ) {
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_node.SetOwner( this );
	m_inventory		= NULL;
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;

	*this = source;
}

CtdmInventoryCursor& CtdmInventoryCursor::operator = ( const CtdmInventoryCursor& source ) {
	if ( this == &source ) {
		goto Quit;
	}

	// Copy everything except their histories.
	copyActiveCursor( source );

	// Copy over their histories.
	CtdmInventoryGroupHistory* groupHistory;

	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL ) {
		groupHistory = new CtdmInventoryGroupHistory();
		if ( groupHistory == NULL ) {
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

void CtdmInventoryCursor::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( m_inventory );
	savefile->WriteInt( m_inventory->GroupToIndex( m_group ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_groupedSlot ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_ungroupedSlot ) );

	// Write out our grouped histories.
	savefile->WriteInt( this->m_groupHistory.Num() );
	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL ) {
		savefile->WriteInt( m_inventory->GroupToIndex( ghNode->Owner()->m_group ) );
		savefile->WriteInt( m_inventory->SlotToIndex( ghNode->Owner()->m_slot ) );

		ghNode = ghNode->NextNode();
	}
}

void CtdmInventoryCursor::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_inventory ) );
	if ( m_inventory != NULL ) {
		m_node.AddToEnd( m_inventory->m_cursors );
	}
	savefile->ReadInt( reinterpret_cast<int &>( m_groupNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_groupedSlotNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_ungroupedSlotNum ) );

	unsigned int numGroupHistories;
	CtdmInventoryGroupHistory* groupHistory;

	// Read in our group histories.
	savefile->ReadInt( reinterpret_cast<int &>( numGroupHistories ) );
	while ( numGroupHistories-- ) {

		groupHistory = new CtdmInventoryGroupHistory();
		if ( groupHistory == NULL ) {
			gameLocal.Error("Unable to allocate memory for group history.");
			goto Quit;
		}

		// Setup the new slot.
		groupHistory->m_node.SetOwner( groupHistory );

		savefile->ReadInt( reinterpret_cast<int &>( groupHistory->m_groupNum ) );
		savefile->ReadInt( reinterpret_cast<int &>( groupHistory->m_slotNum ) );

		groupHistory->m_node.AddToEnd( m_groupHistory );
	}

	PostEventMS( &EV_PostRestore, 0 );

	Quit:
	return;
}

void CtdmInventoryCursor::Event_PostRestore() {
	m_group = m_inventory->IndexToGroup( m_groupNum );
	if ( m_group != NULL ) {
		m_group->m_numCursors++;
	}
	m_groupedSlot = m_inventory->IndexToSlot( m_groupedSlotNum, m_group );
	if ( m_groupedSlot != NULL ) {
		m_groupedSlot->m_numCursors++;
	}
	m_ungroupedSlot = m_inventory->IndexToSlot( m_ungroupedSlotNum );
	if ( m_ungroupedSlot != NULL ) {
		m_ungroupedSlot->m_numCursors++;
	}

	CtdmInventoryGroupHistory* groupHistory;
	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL ) {

		groupHistory = ghNode->Owner();
		groupHistory->m_group = m_inventory->IndexToGroup( groupHistory->m_groupNum );
		groupHistory->m_slot = m_inventory->IndexToSlot( groupHistory->m_slotNum, groupHistory->m_group );
		groupHistory->m_slotNode.AddToEnd( groupHistory->m_slot->m_historyCursors );

		ghNode = ghNode->NextNode();
	}
}

/// Copies only the active cursor position, not any cursor histories.
void CtdmInventoryCursor::copyActiveCursor(	const CtdmInventoryCursor& source,
											bool noHistory ) {
	if ( this == &source ) {
		goto Quit;
	}

	// Switch to the same inventory.
	setInventory( source.m_inventory );
	// Copy over their active cursor.
	select( source.m_group, source.m_groupedSlot, source.m_ungroupedSlot, noHistory );

	Quit:
	return;
}

void CtdmInventoryCursor::setInventory( CtdmInventory* inventory ) {

	if ( m_inventory != NULL ) {
		// Remove ourself from our current inventory.

		// Remove our active cursor from the inventory.
		select( NULL, NULL, NULL );
		m_node.Remove();

		// Remove our group histories.
		idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
		CtdmInventoryGroupHistory* tempGroupHistory;
		CtdmInventoryGroup* tempGroup;
		CtdmInventorySlot* tempSlot;
		while ( ghNode != NULL ) {
			tempGroupHistory = ghNode->Owner();
			tempGroup = tempGroupHistory->m_group;
			tempSlot = tempGroupHistory->m_slot;
			ghNode = ghNode->NextNode();

			delete tempGroupHistory;
			m_inventory->check( tempGroup, tempSlot );
		}

		m_inventory = NULL;
	}

	if ( inventory != NULL ) {
		// Add ourself to the new inventory.
		m_node.AddToEnd( inventory->m_cursors );
		m_inventory	= inventory;
	}

}

// Consider inlining?
/// Returns the inventory this cursor points to.
CtdmInventory* CtdmInventoryCursor::inventory() const {
	return m_inventory;
}

// Consider inlining?
/// Returns the name of the current inventory group. 'nullOk' causes it to return NULL instead of "" when outside any group.
const char* CtdmInventoryCursor::group( bool nullOk ) const {
	if ( m_group == NULL ) {
		return nullOk ? NULL : "";
	} else {
		return m_group->m_name.c_str();
	}
}

/// Selects a specific item. (note: cursor must be pointing to the correct inventory)
void CtdmInventoryCursor::selectItem( CtdmInventoryItem* item, bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("selectItem() called on an independant tdmInventoryCursor.");
		goto Quit;
	}
	if ( item->m_inventory != m_inventory ) {
		gameLocal.Warning("Attempted to move cursor to item outside current inventory.");
		goto Quit;
	}

	select( item->m_group, item->m_groupedSlot, item->m_ungroupedSlot, noHistory );

	Quit:
	return;
}

// Consider inlining?
/// Returns the item this cursor is pointing to.
CtdmInventoryItem* CtdmInventoryCursor::item() const {
	// It's important that this queries the grouped slot and not the
	// ungrouped one; the grouped slot is always up-to-date, but the
	// ungrouped slot might not be. How is that possible, you may wonder?
	// If you have a group history pointing to a slot that's now empty,
	// and you switch groups to the one containing that history, the
	// grouped slot will be changed to point to an empty slot...
	// unfortunately, since there's no item, we have no way of knowing
	// the associated ungrouped slot (and it might have even been
	// cleaned up), so we are forced to leave the ungrouped slot as it
	// is, or set it to NULL. Either way, it won't be pointing to the
	// current item. The reason the grouped slot can't be out of
	// date, is because ungrouped slots don't have histories, so you
	// can only select an ungrouped slot if it contains an item, which
	// allows the grouped slot to be updated.
	return ( m_groupedSlot != NULL ) ? m_groupedSlot->m_item : NULL;
}

/// Iterates through an inventory.
/**	Iterates this cursor through an inventory. The cursor scan through the
 *	inventory in the specified order, stopping when it finds an item.
 *	
 *	'type' specifies the style of iteration:
 *	
 *		TDMINV_UNGROUPED: Iterate through the inventory while completely
 *		ignoring grouping. The inventory is treated as circular, with an
 *		imaginary empty slot between the last and first items.
 *	
 *		TDMINV_HYBRID: Iterate through the inventory with items from the
 *		same group placed consecutively. The inventory is treated as
 *		circular, with an imaginary slot between the last and first items.
 *	
 *		TDMINV_GROUP: Iterate through the groups of the inventory.
 *	
 *		TDMINV_ITEM: Iterate through the items of the current group. The
 *		group is treated as circular, with an imaginary empty slot between
 *		the last and first items.
 *	
 *	'backwards' indicates that the iteration should be done in the reverse
 *	direction.
 *	
 *	'noHistory' indicates that the interation should have no effect on
 *	group histories.
 *	
 *	'filter' is a function that can be used to skip over certain items
 *	during the iteration. One possible use it to prevent the iteration
 *	from landing on an imaginary empty slot. It is important to note that
 *	the filter function should not do anything that could interfere with
 *	the current inventory or any of its items or cursors pointing to it.
 *	In particular, this means you probably should not let a D3script
 *	function act as a filter-function, since a malicious scripter might
 *	be able to cause memory corruption.
 */
void CtdmInventoryCursor::iterate(	EtdmInventoryIterationMethod type,
									bool backwards,
									bool noHistory,
									bool (*filter)( CtdmInventoryItem* ) ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("iterate() called on an independant tdmInventoryCursor.");
		goto Quit;
	}

	switch (type) {
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
									bool noHistory ) {
	// Since this function is a central cog of much of the cursor and hence
	// inventory code, I figured it'd be a good place to put many assertions
	// that I'd like to ensure.
	assert( m_inventory != NULL );
	// Assert correctness of current position
	assert( m_group != NULL || (m_groupedSlot == NULL && m_ungroupedSlot == NULL) ); // (m_group == NULL) implies (m_groupedSlot == NULL && m_ungroupedSlot == NULL)
	assert( m_ungroupedSlot == NULL || m_groupedSlot != NULL );  // (m_ungroupedSlot != NULL) implies (m_groupedSlot != NULL)
	assert( m_group == NULL || m_group->m_node.ListHead() == &m_inventory->m_groupList ); // (m_group != NULL) implies (m_group exists in m_inventory)
	assert( m_groupedSlot == NULL || m_groupedSlot->m_node.ListHead() == &m_group->m_itemList ); // (m_groupedSlot != NULL) implies (m_groupedSlot exists in m_group)
	assert( m_ungroupedSlot == NULL || m_ungroupedSlot->m_node.ListHead() == &m_inventory->m_itemList ); // (m_ungroupedSlot != NULL) implies (m_ungroupedSlot exists in m_inventory)
	// Assert correctness of new position
	assert( group != NULL || (groupedSlot == NULL && ungroupedSlot == NULL) ); // (group == NULL) implies (groupedSlot == NULL && ungroupedSlot == NULL)
	assert( ungroupedSlot == NULL || groupedSlot != NULL );  // (ungroupedSlot != NULL) implies (groupedSlot != NULL)
	assert( group == NULL || group->m_node.ListHead() == &m_inventory->m_groupList ); // (group != NULL) implies (group exists in m_inventory)
	assert( groupedSlot == NULL || groupedSlot->m_node.ListHead() == &group->m_itemList ); // (groupedSlot != NULL) implies (groupedSlot exists in group)
	assert( ungroupedSlot == NULL || ungroupedSlot->m_node.ListHead() == &m_inventory->m_itemList ); // (ungroupedSlot != NULL) implies (ungroupedSlot exists in m_inventory)

	// Change positions.
	CtdmInventoryGroup* prevGroup			= m_group;
	CtdmInventorySlot* prevGroupedSlot		= m_groupedSlot;
	CtdmInventorySlot* prevUngroupedSlot	= m_ungroupedSlot;
	m_group			= group;
	m_groupedSlot	= groupedSlot;
	m_ungroupedSlot	= ungroupedSlot;

	// Update group histories.
	if ( group != NULL && noHistory == false ) {

		CtdmInventoryGroupHistory* groupHistory = getGroupHistory( group );

		if ( groupedSlot != NULL ) {

			// We're moving to a slot and need to save a history.

			// Ensure that a correct history exists.
			if ( groupHistory == NULL ) {
				// There isn't yet a group history. Create one.
				groupHistory = new CtdmInventoryGroupHistory( group );
				if ( groupHistory == NULL ) {
					gameLocal.Error("Unable to allocate memory for group history.");
					goto Quit;
				}

				groupHistory->m_node.AddToEnd( m_groupHistory );
			}

			groupHistory->m_slot = groupedSlot;
			groupHistory->m_slotNode.AddToEnd( groupedSlot->m_historyCursors );

		} else {

			// We're moving to the imaginary empty slot of some group.
			// We should save that as a history by deleting
			// the group history for that group.
			if ( groupHistory != NULL ) {
				delete groupHistory;
			}

		}
	}

	// Increment reference counts in the new position.
	if ( group != NULL ) {
		group->m_numCursors++;
	}
	if ( groupedSlot != NULL ) {
		groupedSlot->m_numCursors++;
	}
	if ( ungroupedSlot != NULL ) {
		ungroupedSlot->m_numCursors++;
	}

	// Decrement reference counts in the old position.
	if ( prevGroup != NULL ) {
		prevGroup->m_numCursors--;
	}
	if ( prevGroupedSlot != NULL ) {
		prevGroupedSlot->m_numCursors--;
	}
	if ( prevUngroupedSlot != NULL ) {
		prevUngroupedSlot->m_numCursors--;
	}


	// Perform cleanup.
	if ( prevGroup != NULL ) {
		m_inventory->check( prevGroup, prevGroupedSlot );
	}
	if ( prevUngroupedSlot != NULL ) {
		m_inventory->check( prevUngroupedSlot );
	}

	Quit:
	return;
}

/// Find a group history, if it exists. May return NULL.
CtdmInventoryGroupHistory* CtdmInventoryCursor::getGroupHistory( CtdmInventoryGroup* group ) const {
	// Find the requested node. Maybe return NULL.
	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL && ghNode->Owner()->m_group != group ) {
		ghNode = ghNode->NextNode();
	}
	return (ghNode != NULL) ? ghNode->Owner() : NULL;
}

/// Iterates without regard to groups.
void CtdmInventoryCursor::iterateUngrouped( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) ) {
	// The slot we're currently scanning. Initialize it to our current
	// ungrouped position. Use the head of the linked list to signify
	// the imaginary empty slot.
	idLinkList<CtdmInventorySlot>* sNode;
	if ( m_ungroupedSlot != NULL ) {
		sNode = &m_ungroupedSlot->m_node; // current slot
	} else {
		sNode = &m_inventory->m_itemList; // imaginary empty slot
	}

	while ( true ) {

		// Step to the next slot.
		sNode = backwards ? sNode->PrevNode() : sNode->NextNode();
		if ( sNode == NULL ) {
			sNode = &m_inventory->m_itemList;
		}

		// If we've passed full-circle through the inventory, we need
		// to just quit and not iterate at all.
		if ( sNode->Owner() == m_ungroupedSlot ) {
			break;
		}

		// If we're in an empty slot, but it's not the imaginary empty slot,
		// then we need to continue scanning.
		if ( sNode->Owner() != NULL && sNode->Owner()->m_item == NULL ) {
			continue;
		}

		// Did the user supply an item filter?
		if ( filter != NULL ) {

			// Construct the value we'll be sending to the filter.
			// (NULL signifies imaginary empty slot)
			CtdmInventoryItem* item = NULL;
			if ( sNode->Owner() != NULL ) {
				item = sNode->Owner()->m_item;
			}

			// If the item filter didn't like the item, then we need to
			// continue scanning.
			if ( (*filter)( item ) == false ) {
				continue;
			}
		}

		// Ok, we've found a slot we can iterate to!
		if ( sNode->Owner() != NULL ) {
			selectItem( sNode->Owner()->m_item, noHistory );
		} else {
			select( NULL, NULL, NULL, noHistory );
		}
		break;
	}
}

/// Iterates with items grouped together.
void CtdmInventoryCursor::iterateHybrid( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) ) {
	// The group and slot we're currently scanning. Initialize it to our
	// current grouped position. Use the head of the linked list for groups
	// groups to signify the imaginary empty slot.
	idLinkList<CtdmInventoryGroup>* gNode;
	idLinkList<CtdmInventorySlot>* sNode;
	if ( m_groupedSlot != NULL ) {
		// current grouped position
		gNode = &m_group->m_node;
		sNode = &m_groupedSlot->m_node;
	} else {
		// imaginary empty slot
		gNode = &m_inventory->m_groupList;
		// sNode = doesn't matter
	}

	// There are 3 possible states:
	// Imaginary empty slot:
	//   gNode = &m_inventory->m_groupList
	//   sNode = *
	//   Used for when the cursor is at the imaginary empty slot.
	// At a group:
	//   gNode = &group->m_node
	//   sNode = &group->m_itemList
	//   A temporary state used to check over a group when we don't yet care about any specific slot.
	// At a specific slot:
	//   gNode = &group->m_node
	//   sNode = &slot->m_node
	//   Used for when the cursor is pointing to a specific slot.

	while ( true ) {

		if ( gNode->Owner() != NULL ) {

			// We're entering/in a group.

			// If we're entering the same empty group we started at,
			// this is our only chance to catch it.
			if ( sNode->Owner() == NULL &&
				gNode->Owner()->m_numItems == 0 &&
				gNode->Owner() == m_group &&
				m_groupedSlot != NULL) {
				// We've gone full-circle and need to quit.
				break;
			}

			// Move onto the first/next slot. (may set sNode to NULL)
			sNode = backwards ? sNode->PrevNode() : sNode->NextNode();
			// Do we need to go onto the next group?
			if ( sNode == NULL || gNode->Owner()->m_numItems == 0 ) {

				gNode = backwards ? gNode->PrevNode() : gNode->NextNode();
				if ( gNode == NULL ) {
					gNode = &m_inventory->m_groupList; // imaginary empty slot
					// We need to fall down to the rest of the code, so the
					// cursor can potentially iterate to this spot.
				} else {
					sNode = &gNode->Owner()->m_itemList; // entering new group
					// We're entering a new group so we have nothing to check for. Loop back.
					continue;
				}
			}

		} else {

			// We're currently at the imaginary empty slot.

			// Go to the first group.
			gNode = backwards ? gNode->PrevNode() : gNode->NextNode();
			// If there's nothing in the inventory, then we need to quit.
			if ( gNode == NULL ) {
				break;
			}
			sNode = &gNode->Owner()->m_itemList;

			// We're entering a new group so we have nothing to check for. Loop back.
			continue;
		}

		// It shouldn't be possible for the execution to reach this point
		// while in an "entering new group" state.
		assert( gNode->Owner() == NULL || sNode != &gNode->Owner()->m_itemList );

		if ( gNode->Owner() == NULL ) {
			// Have we looped back to the imaginary empty slot?
			if (m_groupedSlot == NULL) {
				// We've gone full-circle and need to quit.
				break;
			}
		} else if ( sNode->Owner() == m_groupedSlot ) {
			// We've gone full-circle and need to quit.
			break;
		}

		// If we're in an empty slot, but it's not the imaginary empty slot,
		// then we need to continue scanning.
		if ( gNode->Owner() != NULL && sNode->Owner()->m_item == NULL ) {
			continue;
		}

		// Did the user supply an item filter?
		if ( filter != NULL ) {

			// Construct the value we'll be sending to the filter.
			// (NULL signifies imaginary empty slot)
			CtdmInventoryItem* item = NULL;
			if ( gNode->Owner() != NULL ) {
				item = sNode->Owner()->m_item;
			}

			// If the item filter didn't like the item, then we need to
			// continue scanning.
			if ( (*filter)( item ) == false ) {
				continue;
			}
		}

		// Ok, we've found a slot we can iterate to!
		if ( gNode->Owner() != NULL ) {
			selectItem( sNode->Owner()->m_item, noHistory );
		} else {
			select( NULL, NULL, NULL, noHistory );
		}
		break;
	}
}

/// Iterates through the list of groups.
void CtdmInventoryCursor::iterateGroup( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) ) {
	// Set gNode to the node of our current group.
	idLinkList<CtdmInventoryGroup>* gNode;
	if ( m_group != NULL ) {
		gNode = &m_group->m_node; // some group
	} else {
		gNode = &m_inventory->m_groupList; // no group
	}

	while ( true ) {

		// Step to the next slot.
		gNode = backwards ? gNode->PrevNode() : gNode->NextNode();
		if ( gNode == NULL ) {
			gNode = &m_inventory->m_groupList;
		}

		// If we've passed full-circle through the inventory we need
		// to quit.
		if ( gNode->Owner() == m_group ) {
			// If there are no items in the starting group, then
			// there are no items in the inventory, so we should
			// unselect everything.
			if ( m_group->m_numItems == 0 ) {
				select( NULL, NULL, NULL, noHistory );
			}
			break;
		}

		// If we're in an empty group or none at all, then we need to
		// continue scanning.
		if ( gNode->Owner() == NULL || gNode->Owner()->m_numItems == 0 ) {
			continue;
		}

		// Ok, we've found a group we can iterate to!

		// Do we have a group history for it?
		CtdmInventoryGroupHistory* groupHistory = getGroupHistory( gNode->Owner() );
		if ( groupHistory != NULL ) {

			// Yes, we have a group history.

			// Set our grouped slot to its last value in this group.
			select( gNode->Owner(), groupHistory->m_slot, NULL );

		} else {

			// No, we don't have a group history.

			select( NULL, NULL, NULL, noHistory );
		}

		break;
	}
}

/// Iterates through the items in a group.
void CtdmInventoryCursor::iterateItem( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) ) {
	// Make sure we're in a group, or we can't iterate.
	if ( m_group == NULL ) {
		goto Quit;
	}

	// The slot we're currently scanning. Initialize it to our current
	// ungrouped position. Use the head of the linked list to signify
	// the imaginary empty slot.
	idLinkList<CtdmInventorySlot>* sNode;
	if ( m_groupedSlot != NULL ) {
		sNode = &m_groupedSlot->m_node; // current slot
	} else {
		sNode = &m_group->m_itemList; // imaginary empty slot
	}

	while ( true ) {

		// Step to the next slot.
		sNode = backwards ? sNode->PrevNode() : sNode->NextNode();
		if ( sNode == NULL ) {
			sNode = &m_group->m_itemList;
		}

		// If we've passed full-circle through the inventory, we need
		// to just quit and not iterate at all.
		if ( sNode->Owner() == m_groupedSlot ) {
			break;
		}

		// If we're in an empty slot, but it's not the imaginary empty slot,
		// then we need to continue scanning.
		if ( sNode->Owner() != NULL && sNode->Owner()->m_item == NULL ) {
			continue;
		}

		// Did the user supply an item filter?
		if ( filter != NULL ) {

			// Construct the value we'll be sending to the filter.
			// (NULL signifies imaginary empty slot)
			CtdmInventoryItem* item = NULL;
			if ( sNode->Owner() != NULL ) {
				item = sNode->Owner()->m_item;
			}

			// If the item filter didn't like the item, then we need to
			// continue scanning.
			if ( (*filter)( item ) == false ) {
				continue;
			}
		}

		// Ok, we've found a slot we can iterate to!
		if ( sNode->Owner() != NULL ) {
			// Select the item at our slot.
			selectItem( sNode->Owner()->m_item, noHistory );
		} else {
			select( NULL, NULL, NULL, noHistory );
		}
		break;
	}

	Quit:
	return;
}


  ////////////////////////
 // CtdmInventoryGroup //
////////////////////////

#ifdef TDMINVENTORY_DEBUG
int CtdmInventoryGroup::numGroups = 0;
#endif

CtdmInventoryGroup::CtdmInventoryGroup( const char* name ) {
	m_node.SetOwner( this );
	m_name = name;
	m_numItems = 0;
	m_numCursors = 0;

#ifdef TDMINVENTORY_DEBUG
	numGroups++;
#endif
}

CtdmInventoryGroup::~CtdmInventoryGroup() {
	// A group should never be removed if it has items or cursors.
	// If it is, it's caused by a bug.
	assert( m_numItems == 0 );
	assert( m_numCursors == 0 );

	// If there's still history cursors pointing to us, delete them.
	idLinkList<CtdmInventorySlot>* sNode = m_itemList.NextNode();
	CtdmInventorySlot* slot;
	while ( sNode != NULL ) {
		slot = sNode->Owner();
		sNode = sNode->NextNode();

		assert( slot->m_item == NULL );
		assert( slot->m_numCursors == 0 );

		// Go through the group histories pointing to this slot and delete them.
		idLinkList<CtdmInventoryGroupHistory>* ghNode = slot->m_historyCursors.NextNode();
		CtdmInventoryGroupHistory* groupHistory;
		while ( ghNode != NULL ) {
			groupHistory = ghNode->Owner();
			ghNode = ghNode->NextNode();

			assert( groupHistory->m_group == this );
			assert( groupHistory->m_slot == slot );

			delete groupHistory;
		}

		delete slot;
	}

#ifdef TDMINVENTORY_DEBUG
	numGroups--;
#endif
}

#ifdef TDMINVENTORY_DEBUG
int CtdmInventoryGroup::groups() {
	return numGroups;
}
#endif

  ///////////////////////
 // CtdmInventorySlot //
///////////////////////

#ifdef TDMINVENTORY_DEBUG
int CtdmInventorySlot::numSlots = 0;
#endif

CtdmInventorySlot::CtdmInventorySlot( CtdmInventoryItem* item ) {
	m_node.SetOwner( this );
	m_item = item;
	m_numCursors = 0;

#ifdef TDMINVENTORY_DEBUG
	numSlots++;
#endif
}

#ifdef TDMINVENTORY_DEBUG
CtdmInventorySlot::~CtdmInventorySlot() {
	numSlots--;
}

int CtdmInventorySlot::slots() {
	return numSlots;
}
#endif

  ///////////////////////////////
 // CtdmInventoryGroupHistory //
///////////////////////////////

CtdmInventoryGroupHistory::CtdmInventoryGroupHistory( CtdmInventoryGroup* group ) {
	m_node.SetOwner( this );
	m_slotNode.SetOwner( this );
	m_group = group;
	m_slot = NULL;
}
