/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
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

#ifndef __DARKMOD_TDMINVENTORY_H__
#define __DARKMOD_TDMINVENTORY_H__

class tdmInventory;
class tdmInventoryItem;
class tdmInventoryCursor;

struct tdmInventoryGroup;
struct tdmInventorySlot;
struct tdmInventoryGroupHistory;

/// Adds all inventory-related objects to the savefile's object list.
void tdmInventorySaveObjectList( idSaveGame *savefile );

/// An inventory that can hold items.
class tdmInventory : public idClass {
	CLASS_PROTOTYPE( tdmInventory );
  public:

	tdmInventory();
	~tdmInventory();

	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile );

	int		debugNumSlots() const;

	/// The inventory item's owner.
	idEntityPtr<idEntity>	m_owner;

  private:

	friend class tdmInventoryItem;
	friend class tdmInventoryCursor;
	tdmInventory(				const tdmInventory& source ) { /* do nothing */}
	tdmInventory& operator = (	const tdmInventory& source ) { /* do nothing */}

	/// Returns a pointer to the group with the given name. Creates the group if it must.
	tdmInventoryGroup*	obtainGroup( const char* groupName );
	/// Checks if a group should be deallocated, and does so if necessary.
	void				checkGroup( tdmInventoryGroup* group );
	/// Checks if a slot should be deallocated, and does so if necessary.
	void				checkSlot( tdmInventorySlot* slot );

	/// Converts a slot into an index. (used for saving)
	unsigned int		SlotToIndex( const tdmInventorySlot* slot ) const;
	/// Converts an index into a slot. (used for restoring)
	tdmInventorySlot*	IndexToSlot( unsigned int index, const tdmInventoryGroup* group = NULL ) const;
	/// Converts a group into an index. (used for saving)
	unsigned int		GroupToIndex( const tdmInventoryGroup* group ) const;
	/// Converts an index into a group. (used for restoring)
	tdmInventoryGroup*	IndexToGroup( unsigned int index ) const;

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>	m_inventoryObjListNode;
	/// Used for debugging pursposes.
	unsigned int						m_numSlots;
	/// List of slots for ungrouped inventory.
	idLinkList<tdmInventorySlot>		m_itemList;
	/// List of groups.
	idLinkList<tdmInventoryGroup>		m_groupList;
	/// List of cursors pointing to this inventory.
	idLinkList<tdmInventoryCursor>	m_cursors;
};

/// An item that can be put in an inventory.
class tdmInventoryItem : public idClass {
	CLASS_PROTOTYPE( tdmInventoryItem );
  public:

	tdmInventoryItem();
	~tdmInventoryItem();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				Event_PostRestore();

	/// Puts the item into an inventory or removes it.
	void				setInventory( tdmInventory* inventory );
	/// Returns the inventory this item is contained by.
	tdmInventory*	inventory() const;

	/// Sets the item's group. (any active cursors pointing to the item will remain pointing to it)
	void				setGroup( const char* groupName );
	/// Gets the item's group.
	const char*			group() const;

	/// The inventory item's owner.
	idEntityPtr<idEntity>	m_owner;

  private:

	friend class tdmInventory;
	friend class tdmInventoryCursor;
	tdmInventoryItem(				const tdmInventoryItem& source ) { /* do nothing */}
	tdmInventoryItem& operator = (	const tdmInventoryItem& source ) { /* do nothing */}

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>	m_inventoryObjListNode;
	/// The name of this item's group.
	idStr				m_groupName;
	/// The inventory this item belongs to.
	tdmInventory*	m_inventory;
	union {
		/// The group this item belongs to.
		tdmInventoryGroup*	m_group;
		/// For temporary restoration purposes only.
		unsigned int		m_groupNum;
	};
	union {
		/// The grouped item slot we belong to.
		tdmInventorySlot*	m_groupedSlot;
		/// For temporary restoration purposes only.
		unsigned int		m_groupedSlotNum;

	};
	union {
		/// The ungrouped item slot we belong to.
		tdmInventorySlot*	m_ungroupedSlot;
		/// For temporary restoration purposes only.
		unsigned int		m_ungroupedSlotNum;
	};
};

/// A full-blown group-remembering cursor that can handle T1-style iteration and grouped iteration simultaneously.
class tdmInventoryCursor : public idClass {
	CLASS_PROTOTYPE( tdmInventoryCursor );
  public:

	tdmInventoryCursor();
	~tdmInventoryCursor();

	tdmInventoryCursor(				const tdmInventoryCursor& source );
	tdmInventoryCursor& operator = (	const tdmInventoryCursor& source );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					Event_PostRestore();

	/// Copies only the active cursor position, not any cursor histories.
	void					copyActiveCursor( const tdmInventoryCursor& source );

	/// Sets the inventory that this cursor is pointing to.
	void					setInventory( tdmInventory* inventory );
	/// Returns the inventory this cursor points to.
	tdmInventory*		inventory() const;

	/// Returns the name of the current inventory group. 'nullOk' causes it to return NULL instead of "" when outside any group.
	const char*				group( bool nullOk = false ) const;

	/// Selects a specific item. (note: cursor must be pointing to the correct inventory)
	void					selectItem( tdmInventoryItem* item, bool noHistory = false );
	/// Returns the item this cursor is pointing to.
	tdmInventoryItem*	item() const;

	// Iterates through the inventory as though completely ungrouped.
	void					next( bool noHistory = false );
	void					prev( bool noHistory = false );

	// Iterates through the entire inventory using grouping.
	void					nextHybrid( bool noHistory = false );
	void					prevHybrid( bool noHistory = false );

	// Iterates through the inventory groups.
	void					nextGroup( bool noHistory = false );
	void					prevGroup( bool noHistory = false );
	// Iterates through the items in the current group.
	void					nextItem( bool noHistory = false );
	void					prevItem( bool noHistory = false );

  private:

	friend class tdmInventory;
	friend class tdmInventoryItem;

	/// Find a group history, if it exists. May return NULL.
	tdmInventoryGroupHistory*	getGroupHistory( tdmInventoryGroup* group ) const;
	/// Selects the grouped slot, updating history, etc.
	void						selectGroupedSlot( tdmInventoryGroup* group, tdmInventorySlot* slot, bool noHistory = false );

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>	m_inventoryObjListNode;
	/// A node so the cursor can be kept in a list by inventories.
	idLinkList<tdmInventoryCursor>		m_node;
	/// The inventory we're pointing to.
	tdmInventory*						m_inventory;
	union {
		/// The group our item belongs to.
		tdmInventoryGroup*					m_group;
		/// For temporary restoration purposes only.
		unsigned int						m_groupNum;
	};
	union {
		/// The grouped item slot we're pointing to. (note: can point to an empty slot while the ungrouped cursor points to a non-empty slot)
		tdmInventorySlot*					m_groupedSlot;
		/// For temporary restoration purposes only.
		unsigned int						m_groupedSlotNum;
	};
	union {
		/// The ungrouped item slot we're pointing to.
		tdmInventorySlot*					m_ungroupedSlot;
		/// For temporary restoration purposes only.
		unsigned int						m_ungroupedSlotNum;
	};
	/// A list of cursors histories for various groups.
	idLinkList<tdmInventoryGroupHistory>	m_groupHistory;
};

/// Used by tdmInventory
struct tdmInventoryGroup {
	/// A node so the group can be kept in a list, by inventores.
	idLinkList<tdmInventoryGroup>	m_node;
	/// The name of this group.
	idStr							m_name;
	/// A list of contained item slots.
	idLinkList<tdmInventorySlot>	m_itemList;
	/// The number of items in this group.
	unsigned long					m_numItems;
};

/// Used by tdmInventory and tdmInventoryGroup
struct tdmInventorySlot {
	/// A node so this slot can be kept in a list, by inventories and groups.
	idLinkList<tdmInventorySlot>	m_node;
	/// The item contained in this slot. (can be NULL representing an empty slot with cursors)
	tdmInventoryItem*			m_item;
	/// The number of cursors and histories pointing to this slot.
	unsigned int					m_numCursors;
};

/// Used by tdmInventoryCursor
struct tdmInventoryGroupHistory {
	/// A node so this group history can be kept in a list, by cursors.
	idLinkList<tdmInventoryGroupHistory>	m_node;
	union {
		/// The group this history refers to.
		tdmInventoryGroup*					m_group;
		/// For temporary restoration purposes only.
		unsigned int						m_groupNum;

	};
	union {
		/// The last slot we used in this group.
		tdmInventorySlot*					m_slot;
		/// For temporary restoration purposes only.
		unsigned int						m_slotNum;
	};
};

#endif /* __DARKMOD_TDMINVENTORY_H__ */
