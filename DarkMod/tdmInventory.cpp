/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.12  2007/01/15 16:37:25  gildoran
 * Added some documentation about the inventory. (though the documentation isn't finished)
 *
 * Revision 1.11  2006/12/14 01:38:28  gildoran
 * Fixed two bugs in cursor's operator =
 * 1. CopyActiveCursor was called without turning off group histories, which could create a duplicate group history.
 * 2. It tried to copy group histories from itself instead of source.
 *
 * Revision 1.10  2006/12/10 04:53:18  gildoran
 * Completely revamped the inventory code again. I took out the other iteration methods leaving only hybrid (and grouped) iteration. This allowed me to slim down and simplify much of the code, hopefully making it easier to read. It still needs to be improved some, but it's much better than before.
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
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../game/Game_local.h"

#include "tdmInventory.h"

const idEventDef EV_PostRestore( "postRestore", NULL );

// The list used by tdmInventorySaveObjectList()
static idLinkList<idClass>	tdmInventoryObjList;

///	Adds all inventory-related objects to a savefile's object list.
/**	When called, this adds all inventories, items and cursors into the
 *	object list of 'savefile', allowing pointers to them to be saved.
 *	This should be called at least once from idGameLocal::SaveGame().
 *	@param savefile The savefile to add the inventories to.
 *	@see idGameLocal::SaveGame()
 *	@see CtdmInventory::m_inventoryObjListNode
 *	@see CtdmInventoryItem::m_inventoryObjListNode
 *	@see CtdmInventoryCursor::m_inventoryObjListNode
 */
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
	// Add us to the list of things that are saved/restored.
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );
}

CtdmInventory::~CtdmInventory() {
	// Remove all cursors.
	while ( m_cursors.Next() ) {
		m_cursors.Next()->SetInventory( NULL );
	}

	assert( !m_cursors.NextNode() );

	// Remove all items.
	// Because all cursors have been removed, every inventory slot
	// should contain an item.
	while ( m_slots.Next() ) {
		m_slots.Next()->m_item->EnterInventory( NULL );
	}

	assert( !m_slots.NextNode() );
}

void CtdmInventory::Save( idSaveGame *savefile ) const {
	m_owner.Save( savefile );

	savefile->WriteInt( m_groups.Num() );
	idLinkList<CtdmInventoryGroup>* gNode = m_groups.NextNode();
	while ( gNode != NULL ) {

		savefile->WriteString( gNode->Owner()->m_name );
		savefile->WriteInt( gNode->Owner()->m_slots.Num() );

		gNode = gNode->NextNode();
	}
}

void CtdmInventory::Restore( idRestoreGame *savefile ) {
	int numGroups;
	int numSlots;
	CtdmInventoryGroup* group;
	CtdmInventorySlot* slot;

	m_owner.Restore( savefile );

	// Read in our groups.
	savefile->ReadInt( numGroups );
	while ( numGroups-- ) {

		group = new CtdmInventoryGroup();
		if ( group == NULL ) {
			gameLocal.Error("Unable to allocate memory for group.");
			goto Quit;
		}

		// Setup the new group.
		savefile->ReadString( group->m_name );
		group->m_inventoryNode.AddToEnd( m_groups );

		// Read in the group's slots.
		savefile->ReadInt( numSlots );
		while ( numSlots-- ) {

			slot = new CtdmInventorySlot( group );
			if ( slot == NULL ) {
				gameLocal.Error("Unable to allocate memory for ungrouped slot.");
				goto Quit;
			}

			slot->m_inventoryNode.AddToEnd( m_slots );
			slot->m_groupNode.AddToEnd( group->m_slots );
		}
	}

	Quit:
	return;
}

///	Returns a pointer to the group with the given name.
/**	Given the name of a group, it will return a pointer to that group.
 *	If no group exists with the name 'groupName', one will be created.
 *	It only returns NULL if it was unable to allocate a group.
 *	@param groupName The name of the group to return.
 *	@return The group named 'groupName'.
 */
CtdmInventoryGroup* CtdmInventory::ObtainGroup( const char* groupName )
{
	// Try to find the requested group.
	// This loop will either set gNode to the requested group,
	// or the group node we would need to add a new group before.
	idLinkList<CtdmInventoryGroup>* gNode = m_groups.NextNode();
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
			group->m_inventoryNode.InsertBefore( *gNode );
		} else {
			group->m_inventoryNode.AddToEnd( m_groups );
		}

	}

	Quit:
	return group;
}

///	Converts a slot into an index. (used for saving)
/**	Given 'slot', it returns an index which may be saved.
 *	IndexToSlot() can then be used to restore the index into a pointer.
 *	@param slot The slot to which an index is desired.
 *	@return The index of 'slot'.
 *	@see IndexToSlot()
 */
unsigned int CtdmInventory::SlotToIndex( const CtdmInventorySlot* slot ) const {
	if ( slot == NULL ) {
		return 0;
	}

	int index = 1;

	idLinkList<CtdmInventorySlot>* sNode = slot->m_inventoryNode.PrevNode();
	while ( sNode != NULL ) {
		index++;
		sNode = sNode->PrevNode();
	}

	return index;
}

///	Converts an index into a slot. (used for restoring)
/**	Given an 'index', it returns the slot to which the index refers.
 *	SlotToIndex() can be used to obtain a savable index.
 *	@param index A slot's index.
 *	@return A pointer to the slot refered to by 'index'.
 *	@see SlotToIndex()
 */
CtdmInventorySlot* CtdmInventory::IndexToSlot( unsigned int index ) const {
	if ( index == 0 ) {
		return NULL;
	}

	idLinkList<CtdmInventorySlot>* sNode;
	sNode = m_slots.NextNode();
	while ( --index ) {
		sNode = sNode->NextNode();
	}

	return sNode->Owner();
}

  ///////////////////////
 // CtdmInventoryItem //
///////////////////////

CLASS_DECLARATION( idClass, CtdmInventoryItem )
	EVENT( EV_PostRestore,	CtdmInventoryItem::Event_PostRestore )
END_CLASS

CtdmInventoryItem::CtdmInventoryItem() {
	// Add us to the list of things that are saved/restored.
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_inventory		= NULL;
	m_slot			= NULL;
}

CtdmInventoryItem::~CtdmInventoryItem() {
	EnterInventory( NULL );
}

void CtdmInventoryItem::Save( idSaveGame *savefile ) const {
	m_owner.Save( savefile );
	savefile->WriteObject( m_inventory );
	savefile->WriteInt( m_inventory->SlotToIndex( m_slot ) );
}

void CtdmInventoryItem::Restore( idRestoreGame *savefile ) {
	m_owner.Restore( savefile );
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_inventory ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_slotIndex ) );
	PostEventMS( &EV_PostRestore, 0 );
}

void CtdmInventoryItem::Event_PostRestore() {
	m_slot = m_inventory->IndexToSlot( m_slotIndex );
}

///	Puts the item in a specific location in an inventory.
/**	This is used to add/remove an item to/from an inventory, or move it
 *	around within an inventory. When called, EnterInventory() moves this
 *	item into the 'groupName' group of 'inventory'. If 'inventory' is
 *	NULL, the rest of the arguments are ignored, and this item is moved
 *	outside of any inventory.
 *	
 *	If 'position' isn't NULL, this item is placed immediately before
 *	(or after, if 'after' is true) it. If 'position' is NULL, this item
 *	is placed at the beginning (or end, if 'after is true) of the group
 *	specified by 'groupName'.
 *
 *	The average usage of enterInventory is as follows:
 *		item.enterInventory( inventory, group );
 *	That will place 'item' at the end of 'group' in 'inventory'.
 *	
 *	It is an error for 'position' to be non-NULL and not in 'inventory'.
 *	It is an error for 'position' to be non-NULL not be in 'groupName'.
 *	
 *	@param inventory the inventory to move this item to
 *	@param groupName the group to move this item to
 *	@param after whether or not to place this item after 'position'
 *	@param position the position to place this item
 *	@see Inventory()
 */
void CtdmInventoryItem::EnterInventory(	CtdmInventory* inventory,
										const char* groupName,
										bool after,
										CtdmInventoryItem* position ) {

	CtdmInventoryGroup* group = NULL;
	CtdmInventorySlot* slot = NULL;

	// Make sure we were given reasonable input.
	if ( !groupName ) {
		gameLocal.Warning("Null group specified.");
		goto Quit;
	}
	if ( position ) {
		if ( !inventory ) {
			gameLocal.Warning("Position was specified for null inventory.");
			goto Quit;
		} else if ( position->m_inventory != inventory ) {
			gameLocal.Warning("Position item is in the wrong inventory.");
			goto Quit;
		} else if ( position->m_slot->m_group->m_name.Cmp( groupName ) != 0 ) {
			gameLocal.Warning("Position item is in the wrong group.");
			goto Quit;
		}
	}

	// If we're moving to another (or the same) inventory, prepare the
	// destination slot.
	if ( inventory != NULL ) {

		// Obtain the inventory group we'll be put in.
		group = inventory->ObtainGroup( groupName );
		if ( !group ) {
			// I'm assuming obtainGroup would have already made an error
			// message, so this doesn't need to.
			//gameLocal.Error("Unable to allocate memory for inventory group.");
			goto Quit;
		}

		slot = new CtdmInventorySlot( group, this );
		if ( !slot ) {
			gameLocal.Error("Unable to allocate memory for inventory slot.");
			goto Quit;
		}

		// Put the slot in the correct place in the group and inventory.
		if ( position ) {
			// We need to place the slot before/after a specific position.
			if ( after ) {
				slot->m_inventoryNode.InsertAfter( position->m_slot->m_inventoryNode );
				slot->m_groupNode.InsertAfter( position->m_slot->m_groupNode );
			} else {
				slot->m_inventoryNode.InsertBefore( position->m_slot->m_inventoryNode );
				slot->m_groupNode.InsertBefore( position->m_slot->m_groupNode );
			}
		} else {
			// We need to place the slot at the beginning/end of the group.
			// Our group may be empty (but other groups cannot be) so we unfortunately
			// must place our slot's inventory node relative to slots from other groups,
			// which unfortunately complicates this code a little bit.
			if ( after ) {
				if ( group->m_inventoryNode.Next() ) {
					// There's a group after our current one - place our slot before its first item.
					slot->m_inventoryNode.InsertBefore( group->m_inventoryNode.Next()->m_slots.Next()->m_inventoryNode );
				} else {
					// Our group is the last one - place our slot at the end of the inventory.
					slot->m_inventoryNode.AddToEnd( inventory->m_slots );
				}
				slot->m_groupNode.AddToEnd( group->m_slots );
			} else {
				if ( group->m_inventoryNode.Prev() ) {
					// There's a group before our current one - place our slot after its last item.
					slot->m_inventoryNode.InsertAfter( group->m_inventoryNode.Prev()->m_slots.Prev()->m_inventoryNode );
				} else {
					// Our group is the first one - place our slot at the front of the inventory.
					slot->m_inventoryNode.AddToFront( inventory->m_slots );
				}
				slot->m_groupNode.AddToFront( group->m_slots );
			}
		}

		// At this point, we may potentially have two slots in a group pointing to
		// us, but that's ok... there'll only be one slot when the next if statement
		// is executed.
	}

	if ( m_slot != NULL ) {
		// Remove ourself from our current inventory.
		m_slot->SetItem( NULL );
	}

	m_inventory		= inventory;
	m_slot			= slot;

	Quit:
	// If we were able to allocate a new group, but not a new slot,
	// clean up our memory leak by deleting the group.
	if ( group && !group->m_slots.Next() ) {
		delete group;
	}
}

///	Replaces one item with another.
/**	Replaces 'item' with this item. 'item' will be moved out of its
 *	inventory, and this item will be put in its exact position. All
 *	cursors that were pointing to 'item' will now point to this item.
 *	
 *	It is important to note that
 *		newItem.ReplaceItem( oldItem );
 *	is different from
 *		newItem.EnterInventory( oldItem.Inventory(), oldItem.Group(), true, oldItem );
 *		oldItem.EnterInventory( NULL );
 *	With the latter code, any cursors that were pointing to 'oldItem'
 *	will not point to 'newItem'. The former code doesn't suffer from
 *	that problem.
 *	
 *	@param item The item to replace.
 */
void CtdmInventoryItem::ReplaceItem( CtdmInventoryItem* item ) {

	// If we're in an inventory, exit it.
	if ( m_inventory ) {
		EnterInventory( NULL );
	}

	// Make sure there's actually work to be done.
	if ( !item || !item->m_inventory ) {
		goto Quit;
	}

	// Replace the given item.
	m_inventory			= item->m_inventory;
	m_slot				= item->m_slot;

	m_slot->m_item		= this;

	item->m_inventory	= NULL;
	item->m_slot		= NULL;

	Quit:
	return;
}

///	Returns the inventory this item is contained by.
/**	If this item isn't in an inventory, NULL is returned.
 *	@return A pointer to the inventory this item resides in, or NULL.
 *	@see EnterInventory()
 */
CtdmInventory* CtdmInventoryItem::Inventory() const {
	return m_inventory;
}

///	Gets the item's group.
/**	If this item isn't in an inventory, NULL is returned.
 *	@return The name of the group this item is in, or NULL.
 */
const char* CtdmInventoryItem::Group() const {
	return m_slot ? m_slot->m_group->m_name.c_str() : NULL;
}


  /////////////////////////
 // CtdmInventoryCursor //
/////////////////////////

CLASS_DECLARATION( idClass, CtdmInventoryCursor )
	EVENT( EV_PostRestore,	CtdmInventoryCursor::Event_PostRestore )
END_CLASS

CtdmInventoryCursor::CtdmInventoryCursor() {
	// Add us to the list of things that are saved/restored.
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_inventoryNode.SetOwner( this );
	m_inventory		= NULL;
	m_slot			= NULL;
}

CtdmInventoryCursor::~CtdmInventoryCursor() {
	SetInventory( NULL );
}

CtdmInventoryCursor::CtdmInventoryCursor( const CtdmInventoryCursor& source ) {
	// Add us to the list of things that are saved/restored.
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );

	m_inventoryNode.SetOwner( this );
	m_inventory		= NULL;
	m_slot			= NULL;

	*this = source;
}

CtdmInventoryCursor& CtdmInventoryCursor::operator = ( const CtdmInventoryCursor& source ) {
	if ( this == &source ) {
		goto Quit;
	}

	// Copy everything except their histories.
	CopyActiveCursor( source, true );

	// Copy over their histories.
	CtdmInventoryGroupHistory* groupHistory;
	idLinkList<CtdmInventoryGroupHistory>* ghNode = source.m_groupHistories.NextNode();
	while ( ghNode != NULL ) {
		groupHistory = new CtdmInventoryGroupHistory( &m_groupHistories, ghNode->Owner()->m_slot );
		if ( groupHistory == NULL ) {
			gameLocal.Error("Unable to allocate memory for group history; group histories only partially copied.");
			goto Quit;
		}

		ghNode = ghNode->NextNode();
	}

	Quit:
	return *this;
}

void CtdmInventoryCursor::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( m_inventory );
	if ( m_inventory ) {
		savefile->WriteInt( m_inventory->SlotToIndex( m_slot ) );

		// Write out our grouped histories.
		savefile->WriteInt( m_groupHistories.Num() );
		idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistories.NextNode();
		while ( ghNode != NULL ) {
			savefile->WriteInt( m_inventory->SlotToIndex( ghNode->Owner()->m_slot ) );

			ghNode = ghNode->NextNode();
		}
	}
}

void CtdmInventoryCursor::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_inventory ) );
	if ( m_inventory ) {
		m_inventoryNode.AddToEnd( m_inventory->m_cursors );

		savefile->ReadInt( reinterpret_cast<int &>( m_slotNum ) );

		int numGroupHistories;
		CtdmInventoryGroupHistory* groupHistory;

		// Read in our group histories.
		savefile->ReadInt( numGroupHistories );
		while ( numGroupHistories-- ) {

			groupHistory = new CtdmInventoryGroupHistory( &m_groupHistories );
			if ( groupHistory == NULL ) {
				gameLocal.Error("Unable to allocate memory for group history.");
				goto Quit;
			}

			savefile->ReadInt( reinterpret_cast<int &>( groupHistory->m_slotNum ) );
		}

		PostEventMS( &EV_PostRestore, 0 );
	}

	Quit:
	return;
}

void CtdmInventoryCursor::Event_PostRestore() {
	m_slot = m_inventory->IndexToSlot( m_slotNum );
	if ( m_slot ) {
		m_slot->AddActiveCursor();
	}

	CtdmInventorySlot* slot;
	CtdmInventoryGroupHistory* groupHistory;
	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistories.NextNode();
	while ( ghNode ) {
		groupHistory = ghNode->Owner();
		slot = m_inventory->IndexToSlot( groupHistory->m_slotNum );
		groupHistory->m_slot = NULL;
		groupHistory->SetSlot( slot );

		ghNode = ghNode->NextNode();
	}
}

///	Copies only the active cursor position, not any cursor histories.
/**	This copies the active cursor position from 'source'. It is useful
 *	for cases where 'source' may be pointing to an empty slot. If 'source'
 *	points to an item and 'noHistory' is false, this cursor's group
 *	histories will be updated to reflect that the item was selected.
 *	(this function is faster if 'noHistory' is true)
 *	@param source The cursor to copy the active position from.
 *	@param noHistory Whether or not to suppress updating the group histories.
 */
void CtdmInventoryCursor::CopyActiveCursor(	const CtdmInventoryCursor& source,
											bool noHistory ) {
	if ( this == &source ) {
		goto Quit;
	}

	// Switch to the same inventory.
	SetInventory( source.m_inventory );
	// Copy over their active cursor.
	if ( m_inventory != NULL ) {
		Select( source.m_slot, noHistory );
	}

	assert( m_inventory == source.m_inventory );
	assert( m_slot == source.m_slot );

	Quit:
	return;
}

///	Sets which inventory this cursor points to.
/**	When called, it sets this cursor to point to 'inventory'.
 *	If 'inventory' isn't NULL, this cursor will point to it, initially
 *  pointing to its null slot.
 *	If inventory is NULL, this cursor will not point to any inventory.
 *	When SetInventory() is used to change the inventory this cursor points
 *	to, this cursor will lose any group history information it had.
 *	@param inventory The inventory to point to.
 *	@see Inventory()
 */
void CtdmInventoryCursor::SetInventory( CtdmInventory* inventory ) {
	// Make sure there's actually work to do.
	if ( inventory == m_inventory )
		goto Quit;

	if ( m_inventory ) {
		// Remove ourself from our current inventory.

		// Remove our active cursor from the inventory.
		Select( NULL );
		m_inventoryNode.Remove();

		// Remove our group histories.
		while ( m_groupHistories.Next() ) {
			delete m_groupHistories.Next();
		}

		m_inventory = NULL;
	}

	if ( inventory != NULL ) {
		// Add ourself to the new inventory.
		m_inventoryNode.AddToEnd( inventory->m_cursors );
	}

	m_inventory	= inventory;

	Quit:
	return;
}

///	Returns the inventory this cursor points to.
/** @return The inventory this cursor points to.
 *	@see SetInventory()
 */
CtdmInventory* CtdmInventoryCursor::Inventory() const {
	return m_inventory;
}

///	Returns the name of the group currently pointed to.
/**	If this cursor doesn't point to an inventory or it's pointing to
 *	the inventory's null slot, "" (or NULL if 'nullOk' is true) will
 *	be returned.
 *	@param nullOk Whether it's ok to return NULL, or if "" should be returned instead.
 *	@return The group this cursor points to.
 */
const char* CtdmInventoryCursor::Group( bool nullOk ) const {
	if ( m_slot ) {
		return m_slot->m_group->m_name.c_str();
	} else {
		return nullOk ? NULL : "";
	}
}

///	Selects a specific item.
/**	Selects 'item', or the null slot if 'item is NULL.
 *	If 'nohistory' is false, it updates this cursor's group histories to
 *	reflect that 'item' has been selected.
 *	(this function is faster if 'noHistory' is true)
 *	
 *	It is an error if 'item' isn't in the inventory pointed to by
 *	this cursor.
 *	
 *	@param item The item to select.
 *	@param noHistory Whether or not to update this cursor's group histories.
 *	@see Item()
 */
void CtdmInventoryCursor::SelectItem( CtdmInventoryItem* item, bool noHistory ) {
	if ( !m_inventory ) {
		gameLocal.Warning("selectItem() called on an independant tdmInventoryCursor.");
		goto Quit;
	}

	if ( !item ) {
		Select( NULL, noHistory );
		goto Quit;
	}

	if ( item->m_inventory != m_inventory ) {
		gameLocal.Warning("Attempted to move cursor to item outside current inventory.");
		goto Quit;
	}

	Select( item->m_slot, noHistory );

	Quit:
	return;
}

///	Returns the item this cursor points to.
/**	@return The item this cursor points to, or NULL if it doesn't point to an item.
 *	@see SelectItem()
 */
CtdmInventoryItem* CtdmInventoryCursor::Item() const {
	return m_slot ? m_slot->m_item : NULL;
}

// Iterates through the items of the inventory.
void CtdmInventoryCursor::IterateItem( bool backwards, bool noHistory ) {
	if ( !m_inventory ) {
		gameLocal.Warning("iterateItem() called on an independant tdmInventoryCursor.");
		goto Quit;
	}

	// Find a slot with an item.
	idLinkList<CtdmInventorySlot>* sNode = m_slot ? &m_slot->m_inventoryNode : &m_inventory->m_slots;
	do {
		sNode = backwards ? sNode->PrevNode() : sNode->NextNode();
	} while ( sNode && !sNode->Owner()->m_item );

	if ( sNode ) {
		Select( sNode->Owner(), noHistory );
	} else {
		Select( NULL, noHistory ); // the second argument isn't really necessary
	}

	Quit:
	return;
}

// Jumps to the next/previous group in the inventory.
void CtdmInventoryCursor::IterateGroup( bool backwards, bool noHistory ) {
	if ( !m_inventory ) {
		gameLocal.Warning("iterateGroup() called on an independant tdmInventoryCursor.");
		goto Quit;
	}

	// Find a group with an item.
	idLinkList<CtdmInventoryGroup>* gNode = m_slot ? &m_slot->m_group->m_inventoryNode : &m_inventory->m_groups;
	do {
		gNode = backwards ? gNode->PrevNode() : gNode->NextNode();
	} while ( gNode && !gNode->Owner()->m_numItems );

	if ( gNode ) {

		// If we have a group history and noHistory is false, then select it.
		// Else, select the first item.
		CtdmInventorySlot* slot;
		CtdmInventoryGroupHistory* groupHistory = noHistory ? NULL : GetGroupHistory( gNode->Owner() );
		if ( groupHistory )
			slot = groupHistory->m_slot;
		else {
			// Find the first item in the group. (we know there's one or we wouldn't have selected the group)
			idLinkList<CtdmInventorySlot>* sNode = gNode->Owner()->m_slots.NextNode();
			while ( !sNode->Owner()->m_item ) {
				sNode = sNode->NextNode();
			}
			slot = sNode->Owner();
		}

		Select( slot, noHistory );

	} else {
		Select( NULL, noHistory ); // the second argument isn't really necessary
	}

	Quit:
	return;
}

/// Selects a slot, updating reference counts.
void CtdmInventoryCursor::Select( CtdmInventorySlot* slot, bool noHistory ) {

	// Since this function is a central cog of much of the cursor and hence
	// inventory code, I figured it'd be a good place to put many assertions
	// that I'd like to ensure.
	assert( m_inventory );

	if ( slot ) {
		slot->AddActiveCursor();
	}
	if ( m_slot )  {
		m_slot->RemoveActiveCursor();
	}
	m_slot = slot;

	if ( !noHistory && m_slot ) {

		// Save our group history cursor.

		CtdmInventoryGroupHistory* groupHistory = GetGroupHistory( m_slot->m_group );

		// Ensure that a group history exists.
		if ( groupHistory == NULL ) {
			// There isn't yet a group history. Create one.
			groupHistory = new CtdmInventoryGroupHistory( &m_groupHistories, m_slot );
			if ( groupHistory == NULL ) {
				gameLocal.Error("Unable to allocate memory for group history.");
				goto Quit;
			}
		}

		groupHistory->SetSlot( slot );
	}

	Quit:
	return;
}

/// Find a group history, if it exists. May return NULL.
CtdmInventoryGroupHistory* CtdmInventoryCursor::GetGroupHistory( CtdmInventoryGroup* group ) const {
	// Find the requested node. Maybe return NULL.
	idLinkList<CtdmInventoryGroupHistory>* ghNode = m_groupHistories.NextNode();
	while ( ghNode != NULL && ghNode->Owner()->m_slot->m_group != group ) {
		ghNode = ghNode->NextNode();
	}
	return (ghNode != NULL) ? ghNode->Owner() : NULL;
}


  ////////////////////////
 // CtdmInventoryGroup //
////////////////////////

#ifdef TDMINVENTORY_DEBUG
int CtdmInventoryGroup::numGroups = 0;
#endif

CtdmInventoryGroup::CtdmInventoryGroup( const char* name ) {
	m_inventoryNode.SetOwner( this );
	m_name = name;
	m_numItems = 0;
	m_numActiveCursors = 0;

#ifdef TDMINVENTORY_DEBUG
	numGroups++;
#endif
}

CtdmInventoryGroup::~CtdmInventoryGroup() {
	// A group should never be removed if it has items or cursors.
	// If it is, it's caused by a bug.
	assert( !m_numItems );
	assert( !m_numActiveCursors );
	assert( !m_historyCursors.Num() );

#ifdef TDMINVENTORY_DEBUG
	numGroups--;
#endif
}

/// Check if this group should delete itself.
void CtdmInventoryGroup::Check() {
	// Should we remove ourself?
	if ( !m_numItems && !m_numActiveCursors ) {

		// Delete all group histories pointing to this group.
		// (doing so should also delete any slots in our group,
		// since we contain no items or active cursors)
		while ( m_historyCursors.Next() ) {
			assert( m_historyCursors.Next()->m_slot->m_group == this );
			delete m_historyCursors.Next();
		}

		delete this;
	}
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

CtdmInventorySlot::CtdmInventorySlot( CtdmInventoryGroup* group, CtdmInventoryItem* item ) {

	assert( group );

	m_inventoryNode.SetOwner( this );
	m_group = group;
	m_groupNode.SetOwner( this );
	m_numCursors = 0;
	m_item = NULL;
	SetItem( item );

#ifdef TDMINVENTORY_DEBUG
	numSlots++;
#endif
}

CtdmInventorySlot::~CtdmInventorySlot() {
	// A slot should never be removed if it has items or cursors.
	// If it is, it's caused by a bug.
	assert( !m_item );
	assert( !m_numCursors );

#ifdef TDMINVENTORY_DEBUG
	numSlots--;
#endif
}

/// Sets the item of this slot.
void CtdmInventorySlot::SetItem( CtdmInventoryItem* item ) {
	bool itemAdded   = !m_item && item;
	bool itemRemoved = m_item && !item;

	m_item = item;

	if ( itemAdded ) {
		m_group->m_numItems++;
	} else if ( itemRemoved ) {
		m_group->m_numItems--;

		CtdmInventoryGroup* group = m_group;
		// If we have neither an item nor cursors, then we should delete ourself.
		// We must be careful not to access 'this' or our member variables afterwards.
		if ( !m_item && !m_numCursors ) {
			delete this;
		}

		// Tell our group to check if it should remove itself.
		// (necessary because an item was removed from it)
		group->Check();
	}
}

/// Tells this slot that an active cursor is looking at it.
void CtdmInventorySlot::AddActiveCursor() {
	m_numCursors++;
	m_group->m_numActiveCursors++;
}

/// Tells this slot that an active cursor is not looking at it.
void CtdmInventorySlot::RemoveActiveCursor() {
	m_numCursors--;
	m_group->m_numActiveCursors--;

	CtdmInventoryGroup* group = m_group;
	// If we have neither an item nor cursors, then we should delete ourself.
	// We must be careful not to access 'this' or our member variables afterwards.
	if ( !m_item && !m_numCursors ) {
		delete this;
	}

	// Tell our group to check if it should remove itself.
	// (necessary because an active cursor was removed from it)
	group->Check();
}

/// Tells this slot that a history cursor is looking at it.
void CtdmInventorySlot::AddHistoryCursor() {
	m_numCursors++;
}

/// Tells this slot that a history cursor is not looking at it.
void CtdmInventorySlot::RemoveHistoryCursor() {
	m_numCursors--;

	// If we have neither an item nor cursors, then we should delete ourself.
	// We must be careful not to access 'this' or our member variables afterwards.
	if ( !m_item && !m_numCursors ) {
		delete this;
	}

	// There's no way our group would need to remove itself if we only lost a
	// history cursor.
}

#ifdef TDMINVENTORY_DEBUG
int CtdmInventorySlot::slots() {
	return numSlots;
}
#endif

  ///////////////////////////////
 // CtdmInventoryGroupHistory //
///////////////////////////////

CtdmInventoryGroupHistory::CtdmInventoryGroupHistory( idLinkList<CtdmInventoryGroupHistory>* histories, CtdmInventorySlot* slot ) {
	m_cursorNode.SetOwner( this );
	m_cursorNode.AddToEnd( *histories );
	m_groupNode.SetOwner( this );
	m_slot = slot;

	if ( m_slot ) {
		m_groupNode.AddToEnd( m_slot->m_group->m_historyCursors );
		m_slot->AddHistoryCursor();
	}
}

CtdmInventoryGroupHistory::~CtdmInventoryGroupHistory() {
	if ( m_slot ) {
		m_groupNode.Remove();
		m_slot->RemoveHistoryCursor();
	}
}

void CtdmInventoryGroupHistory::SetSlot( CtdmInventorySlot* slot ) {
	// Currently there should be no reason for a history cursor to get set to a null slot.
	assert( slot ); 

	m_groupNode.AddToEnd( slot->m_group->m_historyCursors );
	slot->AddHistoryCursor();

	if ( m_slot ) {
		m_slot->RemoveHistoryCursor();
	}

	m_slot = slot;
}
