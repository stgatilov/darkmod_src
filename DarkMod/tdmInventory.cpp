/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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

  /////////////////////
 // tdmInventoryObj //
/////////////////////

CLASS_DECLARATION( idClass, tdmInventoryObj )
END_CLASS

tdmInventoryObj::tdmInventoryObj() {
	m_numSlots = 0;
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );
}

tdmInventoryObj::~tdmInventoryObj() {
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

void tdmInventoryObj::Save( idSaveGame *savefile ) const {

	m_owner.Save( savefile );

	// Write out our ungrouped slots.
	savefile->WriteInt( m_itemList.Num() );
	idLinkList<tdmInventorySlot>* sNode = m_itemList.NextNode();
	while ( sNode != NULL ) {
		savefile->WriteObject( sNode->Owner()->m_item );
		sNode = sNode->NextNode();
	}

	// Write out our groups and grouped slots.
	savefile->WriteInt( m_groupList.Num() );
	idLinkList<tdmInventoryGroup>* gNode = m_groupList.NextNode();
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

void tdmInventoryObj::Restore( idRestoreGame *savefile ) {
	unsigned int numSlots;
	unsigned int numGroups;
	tdmInventorySlot* slot;

	m_owner.Restore( savefile );

	// Read in our ungrouped slots.
	savefile->ReadInt( reinterpret_cast<int &>( numSlots ) );
	while ( numSlots-- ) {

		slot = new tdmInventorySlot();
		if ( slot == NULL ) {
			gameLocal.Error("Unable to allocate memory for ungrouped slot.");
			goto Quit;
		}

		// Setup the new slot.
		slot->m_node.SetOwner( slot );
		slot->m_numCursors = 0;
		m_numSlots++;
		// Load the slot's item.
		savefile->ReadObject( reinterpret_cast<idClass *&>( slot->m_item ) );

		slot->m_node.AddToEnd( m_itemList );
	}

	tdmInventoryGroup* group;

	// Read in our groups and grouped slots.
	savefile->ReadInt( reinterpret_cast<int &>( numGroups ) );
	while ( numGroups-- ) {

		group = new tdmInventoryGroup();
		if ( group == NULL ) {
			gameLocal.Error("Unable to allocate memory for group.");
			goto Quit;
		}

		// Setup the new group.
		group->m_node.SetOwner( group );
		savefile->ReadString( group->m_name );
		group->m_numItems = 0;

		group->m_node.AddToEnd( m_groupList );

		// Read in the group's slots.
		savefile->ReadInt( reinterpret_cast<int &>( numSlots ) );
		while ( numSlots-- ) {

			slot = new tdmInventorySlot();
			if ( slot == NULL ) {
				gameLocal.Error("Unable to allocate memory for ungrouped slot.");
				goto Quit;
			}

			// Setup the new slot.
			slot->m_node.SetOwner( slot );
			slot->m_numCursors = 0;
			m_numSlots++;
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

int tdmInventoryObj::debugNumSlots() const {
	return m_numSlots;
}

/// Return the group with the given name. Create it if neccessary.
tdmInventoryGroup* tdmInventoryObj::obtainGroup( const char* groupName )
{
	// Try to find the requested group.
	// This loop will either set gNode to the requested group,
	// or the group node we would need to add a new group before.
	idLinkList<tdmInventoryGroup>* gNode = m_groupList.NextNodeCircular();
	while ( gNode != &m_groupList && gNode->Owner()->m_name.Cmp( groupName ) < 0 ) {
		gNode = gNode->NextNodeCircular();
	}

	tdmInventoryGroup* group;

	// Did we find our correct group?
	if ( gNode != &m_groupList && gNode->Owner()->m_name.Cmp( groupName ) == 0 ) {

		// We found the group. Let's return it.
		group = gNode->Owner();

	} else {

		// No, we need to create a new group of the correct type, and add it.

		// Alloc a new group.
		group = new tdmInventoryGroup();
		if ( group != NULL ) {

			group->m_node.SetOwner( group );
			group->m_name = groupName;
			group->m_numItems = 0;

			// Insert the group into the inventory.
			group->m_node.InsertBefore( *gNode );

		} else {
			gameLocal.Error("Unable to allocate memory for inventory group.");
		}

	}

	return group;
}

void tdmInventoryObj::checkGroup( tdmInventoryGroup* group ) {
	// If this group contains no items, but has cursors on it,
	// we may be able to consolidate all the cursors into a
	// single slot, to reduce memory usage, or better yet remove
	// this group entirely.
	if ( group->m_numItems == 0 && !group->m_itemList.IsListEmpty() ) {
		// The number of active cursors encountered.
		unsigned int activeCursors = 0;
		// The first slot in the group... We'll be moving cursors here a
		// lot, so I'd like to keep a direct reference to it handy.
		tdmInventorySlot* firstSlot = group->m_itemList.NextNodeCircular()->Owner();
		// The current cursor.
		tdmInventoryCursorObj* cursor;

		// Consolidate active cursors to the first slot,
		// and count how many there were.
		idLinkList<tdmInventoryCursorObj>* cNode = m_cursors.NextNodeCircular();
		while ( cNode != &m_cursors ) {

			cursor = cNode->Owner();
			// Is there an active cursor pointing to this group?
			if ( cursor->m_group == group ) {
				activeCursors++;

				// Move the active cursor to the first slot.
				cursor->m_groupedSlot->m_numCursors--;
				firstSlot->m_numCursors++;
				cursor->m_groupedSlot = firstSlot;
			}

			cNode = cNode->NextNodeCircular();
		}

		tdmInventoryGroupHistory* groupHistory;

		// Consolidate history cursors to the first slot,
		// or if there were no active cursors, then delete them entirely,
		// since we'll be deleting this group.
		cNode = m_cursors.NextNodeCircular();
		while ( cNode != &m_cursors ) {

			cursor = cNode->Owner();

			groupHistory = cursor->getGroupHistory( group );
			if ( groupHistory != NULL ) {
				if ( activeCursors != 0 ) {
					// We're merely moving the group history to the first slot.
					groupHistory->m_slot->m_numCursors--;
					firstSlot->m_numCursors++;
					groupHistory->m_slot = firstSlot;
				} else {
					// We're going to be deleting this group, so let's delete its group history.
					groupHistory->m_slot->m_numCursors--;
					delete groupHistory;
				}
			}

			cNode = cNode->NextNodeCircular();
		}

		// Delete all but the first slot.
		// ( We've consolidate all cursors to the first slot,
		// and we know they don't have items, so it's ok to delete
		// directly, instead of calling checkItem(). )
		idLinkList<tdmInventorySlot>* sNode = firstSlot->m_node.NextNodeCircular();
		tdmInventorySlot* tempSlot;
		while ( sNode != &group->m_itemList ) {
			tempSlot = sNode->Owner();
			sNode = sNode->NextNodeCircular();
			delete tempSlot;
			m_numSlots--;
		}

		// Possibly delete the first slot.
		checkSlot( firstSlot );
	}

	// If the group has no slots, (and thus nothing pointing to it)
	// delete it.
	if ( group->m_itemList.IsListEmpty() ) {
		delete group;
	}
}

// Consider inlining?
void tdmInventoryObj::checkSlot( tdmInventorySlot* slot ) {
	// If the slot is empty and no cursors depend on it, delete it.
	if ( slot->m_item == NULL && slot->m_numCursors == 0 ) {
		delete slot;
		m_numSlots--;
	}
}

/// Converts a slot into an index.
unsigned int tdmInventoryObj::SlotToIndex( const tdmInventorySlot* slot ) const {
	if ( slot == NULL ) {
		return 0;
	}

	int index = 1;

	idLinkList<tdmInventorySlot>* sNode = slot->m_node.PrevNode();
	while ( sNode != NULL ) {
		index++;
		sNode = sNode->PrevNode();
	}

	return index;
}

/// Converts an index into a slot.
tdmInventorySlot* tdmInventoryObj::IndexToSlot( unsigned int index, const tdmInventoryGroup* group ) const {
	if ( index == 0 ) {
		return NULL;
	}

	idLinkList<tdmInventorySlot>* sNode;
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
unsigned int tdmInventoryObj::GroupToIndex( const tdmInventoryGroup* group ) const {
	if ( group == NULL ) {
		return 0;
	}

	int index = 1;

	idLinkList<tdmInventoryGroup>* gNode = group->m_node.PrevNode();
	while ( gNode != NULL ) {
		index++;
		gNode = gNode->PrevNode();
	}

	return index;
}

/// Converts an index into a group.
tdmInventoryGroup* tdmInventoryObj::IndexToGroup( unsigned int index ) const {
	if ( index == 0 ) {
		return NULL;
	}

	idLinkList<tdmInventoryGroup>* gNode = m_groupList.NextNode();
	while ( --index ) {
		gNode = gNode->NextNode();
	}

	return gNode->Owner();
}


  /////////////////////////
 // tdmInventoryItemObj //
/////////////////////////

CLASS_DECLARATION( idClass, tdmInventoryItemObj )
	EVENT( EV_PostRestore,	tdmInventoryItemObj::Event_PostRestore )
END_CLASS

tdmInventoryItemObj::tdmInventoryItemObj() {
	m_inventory		= NULL;
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );
}

tdmInventoryItemObj::~tdmInventoryItemObj() {
	setInventory( NULL );
}

void tdmInventoryItemObj::Save( idSaveGame *savefile ) const {
	m_owner.Save( savefile );
	savefile->WriteString( m_groupName );
	savefile->WriteObject( m_inventory );
	savefile->WriteInt( m_inventory->GroupToIndex( m_group ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_groupedSlot ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_ungroupedSlot ) );
}

void tdmInventoryItemObj::Restore( idRestoreGame *savefile ) {
	m_owner.Restore( savefile );
	savefile->ReadString( m_groupName );
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_inventory ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_groupNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_groupedSlotNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_ungroupedSlotNum ) );
	PostEventMS( &EV_PostRestore, 0 );
}

void tdmInventoryItemObj::Event_PostRestore() {
	m_group = m_inventory->IndexToGroup( m_groupNum );
	m_groupedSlot = m_inventory->IndexToSlot( m_groupedSlotNum, m_group );
	m_ungroupedSlot = m_inventory->IndexToSlot( m_ungroupedSlotNum );
}

void tdmInventoryItemObj::setInventory( tdmInventoryObj *inventory ) {
	// Set to true if allocated memory should be freed upon quitting.
	bool cancelMemory = true;

	// Make sure we're actually changing inventories.
	if ( inventory == m_inventory ) {
		goto Quit;
	}

	if ( m_inventory != NULL ) {
		// Remove ourself from our current inventory.

		m_groupedSlot->m_item = NULL;
		m_ungroupedSlot->m_item = NULL;
		m_group->m_numItems--;
		m_inventory->checkSlot( m_groupedSlot );
		m_inventory->checkSlot( m_ungroupedSlot );
		m_inventory->checkGroup( m_group );

		m_inventory		= NULL;
		m_group			= NULL;
		m_groupedSlot	= NULL;
		m_ungroupedSlot	= NULL;
	}

	if ( inventory != NULL ) {

		// Allocate a new group slot for us.
		m_groupedSlot = new tdmInventorySlot();
		if ( m_groupedSlot == NULL ) {
			gameLocal.Error("Unable to allocate memory for inventory slot.");
			goto Quit;
		}
		// Allocate a new inventory slot for us.
		m_ungroupedSlot = new tdmInventorySlot();
		if ( m_ungroupedSlot == NULL ) {
			gameLocal.Error("Unable to allocate memory for inventory slot.");
			goto Quit;
		}
		// Obtain the inventory group we belong in.
		m_group = inventory->obtainGroup( m_groupName );
		if ( m_group == NULL ) {
			// I'm assuming obtainGroup would have already made an error
			// message, so this doesn't need to.
			//gameLocal.Error("Unable to allocate memory for inventory group.");
			goto Quit;
		}

		// Setup the slots.
		m_groupedSlot->m_node.SetOwner( m_groupedSlot );
		m_groupedSlot->m_item = this;
		m_groupedSlot->m_numCursors = 0;
		m_ungroupedSlot->m_node.SetOwner( m_ungroupedSlot );
		m_ungroupedSlot->m_item = this;
		m_ungroupedSlot->m_numCursors = 0;

		// Add the slots to the inventory/group.
		m_groupedSlot->m_node.AddToEnd( m_group->m_itemList );
		m_ungroupedSlot->m_node.AddToEnd( inventory->m_itemList );
		m_group->m_numItems++;
		inventory->m_numSlots += 2;

		m_inventory = inventory;
	}

	cancelMemory = false;

	Quit:
	if ( cancelMemory ) {
		if ( m_groupedSlot != NULL ) {
			delete m_groupedSlot;
			m_groupedSlot = NULL;
		}
		if ( m_ungroupedSlot != NULL ) {
			delete m_ungroupedSlot;
			m_ungroupedSlot = NULL;
		}
	}
	return;
}

// Consider inlining?
tdmInventoryObj* tdmInventoryItemObj::inventory() const {
	return m_inventory;
}

void tdmInventoryItemObj::setGroup( const char* name ) {
	if ( m_groupName == name ) {
		goto Quit;
	}

	// If we're in an inventory, we'll need to change inventory groups.
	if ( m_inventory != NULL ) {

		tdmInventoryObj* inventory = m_inventory;
		tdmInventorySlot* prevSlot = m_groupedSlot;

		// Let the other code properly handle putting us in the correct group.
		setInventory( NULL );
		m_groupName = name;
		setInventory( inventory );

		if ( prevSlot != NULL ) {

			// If there were any active cursors that were pointing to us,
			// keep them pointing to us.
			idLinkList<tdmInventoryCursorObj>* cNode = inventory->m_cursors.NextNode();
			while ( cNode != NULL ) {
				if ( cNode->Owner()->m_groupedSlot == prevSlot ) {
					cNode->Owner()->selectItem( this );
				}
				cNode = cNode->NextNode();
			}

		}

	} else {
		m_groupName = name;
	}

	Quit:
	return;
}

// Consider inlining?
const char* tdmInventoryItemObj::group( void ) const {
	return m_groupName.c_str();
}

  ///////////////////////////
 // tdmInventoryCursorObj //
///////////////////////////

CLASS_DECLARATION( idClass, tdmInventoryCursorObj )
	EVENT( EV_PostRestore,	tdmInventoryCursorObj::Event_PostRestore )
END_CLASS

tdmInventoryCursorObj::tdmInventoryCursorObj() {
	m_node.SetOwner( this );
	m_inventory		= NULL;
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;
	m_inventoryObjListNode.SetOwner( this );
	m_inventoryObjListNode.AddToEnd( tdmInventoryObjList );
}

tdmInventoryCursorObj::~tdmInventoryCursorObj() {
	setInventory( NULL );
}

tdmInventoryCursorObj::tdmInventoryCursorObj( const tdmInventoryCursorObj& source ) {
	m_node.SetOwner( this );
	m_inventory		= NULL;
	// The following three lines probably aren't strictly necessary.
	m_group			= NULL;
	m_groupedSlot	= NULL;
	m_ungroupedSlot	= NULL;

	*this = source;
}

tdmInventoryCursorObj& tdmInventoryCursorObj::operator = ( const tdmInventoryCursorObj& source ) {
	if ( this == &source ) {
		goto Quit;
	}

	// Copy everything except their histories.
	copyActiveCursor( source );

	// Copy over their histories.
	tdmInventoryGroupHistory* groupHistory;

	idLinkList<tdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL ) {
		groupHistory = new tdmInventoryGroupHistory();
		if ( groupHistory == NULL ) {
			gameLocal.Error("Unable to allocate memory for group history; group histories only partially copied.");
			goto Quit;
		}
		// Copy over the group history information.
		groupHistory->m_node.SetOwner( groupHistory );
		groupHistory->m_group = ghNode->Owner()->m_group;
		groupHistory->m_slot = ghNode->Owner()->m_slot;
		groupHistory->m_slot->m_numCursors++;
		groupHistory->m_node.AddToEnd( m_groupHistory );

		ghNode = ghNode->NextNode();
	}

	Quit:
	return *this;
}

void tdmInventoryCursorObj::Save( idSaveGame *savefile ) const {
	savefile->WriteObject( m_inventory );
	savefile->WriteInt( m_inventory->GroupToIndex( m_group ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_groupedSlot ) );
	savefile->WriteInt( m_inventory->SlotToIndex( m_ungroupedSlot ) );

	// Write out our grouped histories.
	savefile->WriteInt( this->m_groupHistory.Num() );
	idLinkList<tdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL ) {
		savefile->WriteInt( m_inventory->GroupToIndex( ghNode->Owner()->m_group ) );
		savefile->WriteInt( m_inventory->SlotToIndex( ghNode->Owner()->m_slot ) );

		ghNode = ghNode->NextNode();
	}
}

void tdmInventoryCursorObj::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( m_inventory ) );
	if ( m_inventory != NULL ) {
		m_node.AddToEnd( m_inventory->m_cursors );
	}
	savefile->ReadInt( reinterpret_cast<int &>( m_groupNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_groupedSlotNum ) );
	savefile->ReadInt( reinterpret_cast<int &>( m_ungroupedSlotNum ) );

	unsigned int numGroupHistories;
	tdmInventoryGroupHistory* groupHistory;

	// Read in our group histories.
	savefile->ReadInt( reinterpret_cast<int &>( numGroupHistories ) );
	while ( numGroupHistories-- ) {

		groupHistory = new tdmInventoryGroupHistory();
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

void tdmInventoryCursorObj::Event_PostRestore() {
	m_group = m_inventory->IndexToGroup( m_groupNum );
	m_groupedSlot = m_inventory->IndexToSlot( m_groupedSlotNum, m_group );
	if ( m_groupedSlot != NULL ) {
		m_groupedSlot->m_numCursors++;
	}
	m_ungroupedSlot = m_inventory->IndexToSlot( m_ungroupedSlotNum );
	if ( m_ungroupedSlot != NULL ) {
		m_ungroupedSlot->m_numCursors++;
	}

	tdmInventoryGroupHistory* groupHistory;
	idLinkList<tdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNode();
	while ( ghNode != NULL ) {

		groupHistory = ghNode->Owner();
		groupHistory->m_group = m_inventory->IndexToGroup( groupHistory->m_groupNum );
		groupHistory->m_slot = m_inventory->IndexToSlot( groupHistory->m_slotNum, groupHistory->m_group );
		if ( groupHistory->m_slot != NULL ) {
			groupHistory->m_slot->m_numCursors++;
		}

		ghNode = ghNode->NextNode();
	}
}

/// Copies only the active cursor position, not any cursor histories.
void tdmInventoryCursorObj::copyActiveCursor( const tdmInventoryCursorObj& source ) {
	if ( this == &source ) {
		goto Quit;
	}

	// Switch to the same inventory.
	setInventory( source.m_inventory );
	// Copy over their active cursors.
	m_group = source.m_group;
	m_groupedSlot = source.m_groupedSlot;
	if ( m_groupedSlot != NULL ) {
		m_groupedSlot->m_numCursors++;
	}
	m_ungroupedSlot = source.m_ungroupedSlot;
	if ( m_ungroupedSlot != NULL ) {
		m_ungroupedSlot->m_numCursors++;
	}

	Quit:
	return;
}

void tdmInventoryCursorObj::setInventory( tdmInventoryObj* inventory ) {

	if ( m_inventory != NULL ) {
		// Remove ourself from our current inventory.

		// Remove ourself from the cursor list of the inventory,
		// so checkGroup() doesn't try to fiddle with our data
		// while we're busy removing it.
		m_node.Remove();

		// Remove any direct cursor references.
		if ( m_groupedSlot != NULL ) {
			m_groupedSlot->m_numCursors--;
			m_inventory->checkSlot( m_groupedSlot );
		}
		if ( m_ungroupedSlot != NULL ) {
			m_ungroupedSlot->m_numCursors--;
			m_inventory->checkSlot( m_ungroupedSlot );
		}
		if ( m_group != NULL ) {
			m_inventory->checkGroup( m_group );
		}

		// Remove our histories.
		idLinkList<tdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNodeCircular();
		tdmInventoryGroupHistory* tempGroupHistory;
		while ( ghNode != &m_groupHistory ) {

			tempGroupHistory = ghNode->Owner();
			ghNode = ghNode->NextNodeCircular();

			tempGroupHistory->m_slot->m_numCursors--;
			m_inventory->checkSlot( tempGroupHistory->m_slot );
			m_inventory->checkGroup( tempGroupHistory->m_group );
			delete tempGroupHistory;
		}

		m_inventory		= NULL;
		m_group			= NULL;
		m_groupedSlot	= NULL;
		m_ungroupedSlot	= NULL;
	}

	if ( inventory != NULL ) {
		m_node.AddToEnd( inventory->m_cursors );
		m_inventory	= inventory;
	}

}

// Consider inlining?
tdmInventoryObj* tdmInventoryCursorObj::inventory() const {
	return m_inventory;
}

const char* tdmInventoryCursorObj::group( bool nullOk ) const {
	if ( m_group == NULL ) {
		return nullOk ? NULL : "" ;
	} else {
		return m_group->m_name.c_str();
	}
}

void tdmInventoryCursorObj::selectItem( tdmInventoryItemObj* item, bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("selectItem() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}
	if ( item->m_inventory != m_inventory ) {
		gameLocal.Warning("Attempted to move cursor to item outside current inventory.");
		goto Quit;
	}

	// We'll need these to clean up after we've done the move.
	tdmInventorySlot* prevUngroupedSlot = m_ungroupedSlot;

	// Move our active cursors to the item's slots.
	m_ungroupedSlot = item->m_ungroupedSlot;
	m_ungroupedSlot->m_numCursors++;
	if ( prevUngroupedSlot != NULL ) {
		prevUngroupedSlot->m_numCursors--;
	}

	selectGroupedSlot( item->m_group, item->m_groupedSlot, noHistory );

	// Clean up the ungrouped slot our ungrouped cursor used to point at.
	if ( prevUngroupedSlot != NULL ) {
		m_inventory->checkSlot( prevUngroupedSlot );
	}

	Quit:
	return;
}

tdmInventoryItemObj* tdmInventoryCursorObj::item() const {
	// It's intentional that we're only paying attention to
	// m_groupedSlot, and not m_ungroupedSlot.
	if ( m_groupedSlot != NULL ) {
		return m_groupedSlot->m_item;
	} else {
		return NULL;
	}
}

void tdmInventoryCursorObj::next( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("next() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	idLinkList<tdmInventorySlot>* sNode;

	if ( m_ungroupedSlot == NULL ) {

		// Search for an ungrouped slot with an item in it.
		sNode = m_inventory->m_itemList.NextNodeCircular();
		while ( sNode != &m_inventory->m_itemList ) {
			if ( sNode->Owner()->m_item != NULL ) {
				break;
			}
			sNode = sNode->NextNodeCircular();
		}

	} else {

		// Search for the next ungrouped slot with an item in it.
		sNode = m_ungroupedSlot->m_node.NextNodeCircular();
		while ( sNode != &m_ungroupedSlot->m_node ) {
			if ( sNode->Owner() != NULL && sNode->Owner()->m_item != NULL ) {
				break;
			}
			sNode = sNode->NextNodeCircular();
		}

	}
	if ( sNode->Owner() == NULL || sNode->Owner()->m_item == NULL ) {
		// Apparently the inventory is empty. Let's just stay where we are.
		goto Quit;
	}

	selectItem( sNode->Owner()->m_item, noHistory );

	Quit:
	return;
}

void tdmInventoryCursorObj::prev( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("prev() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	idLinkList<tdmInventorySlot>* sNode;

	if ( m_ungroupedSlot == NULL ) {

		// Search for an ungrouped slot with an item in it.
		sNode = m_inventory->m_itemList.PrevNodeCircular();
		while ( sNode != &m_inventory->m_itemList ) {
			if ( sNode->Owner()->m_item != NULL ) {
				break;
			}
			sNode = sNode->PrevNodeCircular();
		}

	} else {

		// Search for the next ungrouped slot with an item in it.
		sNode = m_ungroupedSlot->m_node.PrevNodeCircular();
		while ( sNode != &m_ungroupedSlot->m_node ) {
			if ( sNode->Owner() != NULL && sNode->Owner()->m_item != NULL ) {
				break;
			}
			sNode = sNode->PrevNodeCircular();
		}

	}
	if ( sNode->Owner() == NULL || sNode->Owner()->m_item == NULL ) {
		// Apparently the inventory is empty. Let's just stay where we are.
		goto Quit;
	}

	selectItem( sNode->Owner()->m_item, noHistory );

	Quit:
	return;
}

void tdmInventoryCursorObj::nextHybrid( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("nextHybrid() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	tdmInventoryItemObj* item = NULL;
	idLinkList<tdmInventorySlot>* sNode;

	if ( m_groupedSlot != NULL ) {

		// If we have a group selected, search for the next item
		// in that group.
		sNode = m_groupedSlot->m_node.NextNodeCircular();
		while ( sNode != &m_group->m_itemList ) {
			if ( sNode->Owner() != NULL && sNode->Owner()->m_item != NULL ) {
				item = sNode->Owner()->m_item;
				break;
			}
			sNode = sNode->NextNodeCircular();
		}

	}

	if ( item == NULL ) {
		// If we haven't found an item, look for the next group
		// that has an item.

		idLinkList<tdmInventoryGroup>* gNode;

		// Find
		if ( m_group == NULL ) {

			// Search for the first group with an item in it.
			gNode = m_inventory->m_groupList.NextNodeCircular();
			while ( gNode != &m_inventory->m_groupList ) {
				if ( gNode->Owner()->m_numItems != 0 ) {
					break;
				}
				gNode = gNode->NextNodeCircular();
			}

		} else {

			// Search for the next group with an item in it.
			gNode = m_group->m_node.NextNodeCircular();
			while ( gNode != &m_group->m_node ) {
				if ( gNode->Owner() != NULL && gNode->Owner()->m_numItems != 0 ) {
					break;
				}
				gNode = gNode->NextNodeCircular();
			}

		}
		if ( gNode->Owner() == NULL || gNode->Owner()->m_numItems == 0 ) {
			// Apparently the inventory is empty. Let's just stay where we are.
			goto Quit;
		}

		// Search for the item in our group.
		sNode = gNode->Owner()->m_itemList.NextNodeCircular();
		while ( sNode != &gNode->Owner()->m_itemList ) {
			if ( sNode->Owner()->m_item != NULL ) {
				item = sNode->Owner()->m_item;
				break;
			}
			sNode = sNode->NextNodeCircular();
		}

		if ( item == NULL ) {
			// Shouldn't be possible :P
			gameLocal.Error("Corrupt inventory detected.");
			goto Quit;
		}
	}

	selectItem( item, noHistory );

	Quit:
	return;
}

void tdmInventoryCursorObj::prevHybrid( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("prevHybrid() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	tdmInventoryItemObj* item = NULL;
	idLinkList<tdmInventorySlot>* sNode;

	if ( m_groupedSlot != NULL ) {

		// If we have a group selected, search for the next item
		// in that group.
		sNode = m_groupedSlot->m_node.PrevNodeCircular();
		while ( sNode != &m_group->m_itemList ) {
			if ( sNode->Owner() != NULL && sNode->Owner()->m_item != NULL ) {
				item = sNode->Owner()->m_item;
				break;
			}
			sNode = sNode->PrevNodeCircular();
		}

	}

	if ( item == NULL ) {
		// If we haven't found an item, look for the next group
		// that has an item.

		idLinkList<tdmInventoryGroup>* gNode;

		// Find
		if ( m_group == NULL ) {

			// Search for the first group with an item in it.
			gNode = m_inventory->m_groupList.PrevNodeCircular();
			while ( gNode != &m_inventory->m_groupList ) {
				if ( gNode->Owner()->m_numItems != 0 ) {
					break;
				}
				gNode = gNode->PrevNodeCircular();
			}

		} else {

			// Search for the next group with an item in it.
			gNode = m_group->m_node.PrevNodeCircular();
			while ( gNode != &m_group->m_node ) {
				if ( gNode->Owner() != NULL && gNode->Owner()->m_numItems != 0 ) {
					break;
				}
				gNode = gNode->PrevNodeCircular();
			}

		}
		if ( gNode->Owner() == NULL || gNode->Owner()->m_numItems == 0 ) {
			// Apparently the inventory is empty. Let's just stay where we are.
			goto Quit;
		}

		// Search for the first item in the group.
		sNode = gNode->Owner()->m_itemList.PrevNodeCircular();
		while ( sNode != &gNode->Owner()->m_itemList ) {
			if ( sNode->Owner()->m_item != NULL ) {
				item = sNode->Owner()->m_item;
				break;
			}
			sNode = sNode->PrevNodeCircular();
		}

		if ( item == NULL ) {
			// Shouldn't be possible :P
			gameLocal.Error("Corrupt inventory detected.");
			goto Quit;
		}
	}

	selectItem( item, noHistory );

	Quit:
	return;
}

void tdmInventoryCursorObj::nextGroup( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("nextGroup() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	idLinkList<tdmInventoryGroup>* gNode;
	if ( m_group == NULL ) {

		// Search for the first group with an item in it.
		gNode = m_inventory->m_groupList.NextNodeCircular();
		while ( gNode != &m_inventory->m_groupList ) {
			if ( gNode->Owner()->m_numItems != 0 ) {
				break;
			}
			gNode = gNode->NextNodeCircular();
		}

	} else {

		// Search for the next group with an item in it.
		gNode = m_group->m_node.NextNodeCircular();
		while ( gNode != &m_group->m_node ) {
			if ( gNode->Owner() != NULL &&
				 gNode->Owner()->m_numItems != 0 ) {
				break;
			}
			gNode = gNode->NextNodeCircular();
		}

	}
	if ( gNode->Owner() == NULL || gNode->Owner()->m_numItems == 0 ) {
		// Apparently the inventory is empty. Let's just stay where we are.
		goto Quit;
	}

	tdmInventoryGroupHistory* groupHistory = getGroupHistory( gNode->Owner() );
	if ( groupHistory == NULL ) {

		tdmInventorySlot* slot;

		// Is there a blank slot at the start of the group?
		if ( gNode->Owner()->m_itemList.NextNode() == NULL ||
			 gNode->Owner()->m_itemList.NextNode()->Owner()->m_item != NULL ) {

			// No, add a new blank slot at the beginning of the group.
			slot = new tdmInventorySlot();
			if ( slot == NULL ) {
				gameLocal.Error("Unable to allocate memory for inventory slot.");
				goto Quit;
			}

			// Setup the new slot.
			slot->m_node.SetOwner( slot );
			slot->m_item = NULL;
			slot->m_numCursors = 0;
			// Add the new slot to the start of the group.
			slot->m_node.AddToFront( gNode->Owner()->m_itemList );
			m_inventory->m_numSlots++;

		} else {

			// Yes, move our grouped cursor to that blank slot.
			slot = gNode->Owner()->m_itemList.NextNode()->Owner();

		}
		selectGroupedSlot( gNode->Owner(), slot, noHistory );

	} else {

		// Set our grouped slot to its last value in this group.

		tdmInventorySlot* prevSlot = m_groupedSlot;
		tdmInventoryGroup* prevGroup = m_group;

		m_group = groupHistory->m_group;
		m_groupedSlot = groupHistory->m_slot;
		m_groupedSlot->m_numCursors++;
		if ( prevSlot != NULL ) {
			prevSlot->m_numCursors--;
			m_inventory->checkSlot( prevSlot );
			m_inventory->checkGroup( prevGroup );
		}
	}

	Quit:
	return;
}

void tdmInventoryCursorObj::prevGroup( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("prevGroup() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	idLinkList<tdmInventoryGroup>* gNode;
	if ( m_group == NULL ) {

		// Search for the first group with an item in it.
		gNode = m_inventory->m_groupList.PrevNodeCircular();
		while ( gNode != &m_inventory->m_groupList ) {
			if ( gNode->Owner()->m_numItems != 0 ) {
				break;
			}
			gNode = gNode->PrevNodeCircular();
		}

	} else {

		// Search for the next group with an item in it.
		gNode = m_group->m_node.PrevNodeCircular();
		while ( gNode != &m_group->m_node ) {
			if ( gNode->Owner() != NULL &&
				 gNode->Owner()->m_numItems != 0 ) {
				break;
			}
			gNode = gNode->PrevNodeCircular();
		}

	}
	if ( gNode->Owner() == NULL || gNode->Owner()->m_numItems == 0 ) {
		// Apparently the inventory is empty. Let's just stay where we are.
		goto Quit;
	}

	tdmInventoryGroupHistory* groupHistory = getGroupHistory( gNode->Owner() );
	if ( groupHistory == NULL ) {

		// Note: This part is intentionally assymetrical to nextGroup.
		// Regardless of whether you're moving forward or backwards through
		// groups, you should start out at the front of a group you've never
		// been to before.

		tdmInventorySlot* slot;

		// Is there a blank slot at the start of the group?
		if ( gNode->Owner()->m_itemList.NextNode() == NULL ||
			 gNode->Owner()->m_itemList.NextNode()->Owner()->m_item != NULL ) {

			// No, add a new blank slot at the beginning of the group.
			slot = new tdmInventorySlot();
			if ( slot == NULL ) {
				gameLocal.Error("Unable to allocate memory for inventory slot.");
				goto Quit;
			}

			// Setup the new slot.
			slot->m_node.SetOwner( slot );
			slot->m_item = NULL;
			slot->m_numCursors = 0;

			// Add the new slot to the start of the group.
			slot->m_node.AddToFront( gNode->Owner()->m_itemList );
			m_inventory->m_numSlots++;

		} else {

			// Yes, move our grouped cursor to that blank slot.
			slot = gNode->Owner()->m_itemList.NextNode()->Owner();

		}
		selectGroupedSlot( gNode->Owner(), slot, noHistory );

	} else {

		// Set our grouped slot to its last value in this group.

		tdmInventorySlot* prevSlot = m_groupedSlot;
		tdmInventoryGroup* prevGroup = m_group;

		m_group = groupHistory->m_group;
		m_groupedSlot = groupHistory->m_slot;
		m_groupedSlot->m_numCursors++;
		if ( prevSlot != NULL ) {
			prevSlot->m_numCursors--;
			m_inventory->checkSlot( prevSlot );
			m_inventory->checkGroup( prevGroup );
		}
	}

	Quit:
	return;
}

void tdmInventoryCursorObj::nextItem( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("nextItem() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	if ( m_groupedSlot == NULL ) {
		// We're not in an inventory group so this call was meaningless.
		goto Quit;
	}

	// Search for the next item in our group.
	idLinkList<tdmInventorySlot>* sNode = m_groupedSlot->m_node.NextNodeCircular();
	while ( sNode != &m_groupedSlot->m_node ) {
		if ( sNode->Owner() != NULL && sNode->Owner()->m_item != NULL ) {
			break;
		}
		sNode = sNode->NextNodeCircular();
	}
	if ( sNode->Owner() == NULL || sNode->Owner()->m_item == NULL ) {
		// Apparently the group is empty. Let's just stay where we are.
		goto Quit;
	}

	selectItem( sNode->Owner()->m_item, noHistory );

	Quit:
	return;
}

void tdmInventoryCursorObj::prevItem( bool noHistory ) {
	if ( m_inventory == NULL ) {
		gameLocal.Warning("nextItem() called on an independant tdmInventoryCursorObj.");
		goto Quit;
	}

	if ( m_groupedSlot == NULL ) {
		// We're not in an inventory group so this call was meaningless.
		goto Quit;
	}

	// Search for the next item in our group.
	idLinkList<tdmInventorySlot>* sNode = m_groupedSlot->m_node.PrevNodeCircular();
	while ( sNode != &m_groupedSlot->m_node ) {
		if ( sNode->Owner() != NULL && sNode->Owner()->m_item != NULL ) {
			break;
		}
		sNode = sNode->PrevNodeCircular();
	}
	if ( sNode->Owner() == NULL || sNode->Owner()->m_item == NULL ) {
		// Apparently the group is empty. Let's just stay where we are.
		goto Quit;
	}

	selectItem( sNode->Owner()->m_item, noHistory );

	Quit:
	return;
}

tdmInventoryGroupHistory* tdmInventoryCursorObj::getGroupHistory( tdmInventoryGroup* group ) const {
	// Find the requested node. Maybe return NULL.
	idLinkList<tdmInventoryGroupHistory>* ghNode = m_groupHistory.NextNodeCircular();
	while ( ghNode != &m_groupHistory && ghNode->Owner()->m_group != group ) {
		ghNode = ghNode->NextNodeCircular();
	}
	return ghNode->Owner();
}

/// Selects the grouped slot, updating history, etc.
void tdmInventoryCursorObj::selectGroupedSlot( tdmInventoryGroup* group, tdmInventorySlot* slot, bool noHistory ) {
	// We'll need these to clean up after we've done the move.
	tdmInventorySlot* prevGroupedSlot = m_groupedSlot;
	tdmInventoryGroup* prevGroup = m_group;

	// Move our active grouped cursor to the slot.
	m_groupedSlot = slot;
	m_groupedSlot->m_numCursors++;
	if ( prevGroupedSlot != NULL ) {
		prevGroupedSlot->m_numCursors--;
	}
	m_group = group;

	// Setting these to null shouldn't be necessary.
	tdmInventoryGroupHistory* groupHistory = NULL;
	tdmInventorySlot* prevGroupHistorySlot = NULL;
	if ( noHistory == false ) {
		// Update our history.

		// Find or create the associated group history.
		groupHistory = getGroupHistory( group );
		if ( groupHistory == NULL ) {
			groupHistory = new tdmInventoryGroupHistory();
			if ( groupHistory == NULL ) {
				gameLocal.Error("Unable to allocate memory for group history.");
				goto Quit;
			}
			groupHistory->m_node.SetOwner( groupHistory );
			groupHistory->m_group = group;
			groupHistory->m_slot = NULL;

			groupHistory->m_node.AddToEnd( m_groupHistory );
		}

		prevGroupHistorySlot = groupHistory->m_slot;

		// Move the group history's cursor to the new slot.
		groupHistory->m_slot = m_groupedSlot;
		groupHistory->m_slot->m_numCursors++;
		if ( prevGroupHistorySlot != NULL ) {
			prevGroupHistorySlot->m_numCursors--;
		}
	}

	// Clean up the slots and groups our cursors used to point at.
	if ( prevGroupedSlot != NULL ) {
		m_inventory->checkSlot( prevGroupedSlot );
	}
	if ( noHistory == false &&
		 prevGroupHistorySlot != NULL ) {
		m_inventory->checkSlot( prevGroupHistorySlot );
	}
	if ( prevGroup != NULL ) {
		m_inventory->checkGroup( prevGroup );
	}
	if ( noHistory == false &&
		 groupHistory->m_group != NULL && groupHistory->m_group != prevGroup ) {
		m_inventory->checkGroup( groupHistory->m_group );
	}

	Quit:
	return;
}
