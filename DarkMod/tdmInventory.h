/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.5  2006/08/11 12:32:44  gildoran
 * Added some code so I can start work on the inventory GUI.
 *
 * Revision 1.4  2006/07/25 01:40:28  gildoran
 * Completely revamped inventory code.
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

#ifndef __DARKMOD_TDMINVENTORY_H__
#define __DARKMOD_TDMINVENTORY_H__

#define TDMINVENTORY_DEBUG

// Classes that should be used by external files.
class CtdmInventory;
class CtdmInventoryItem;
class CtdmInventoryCursor;

// Classes that are used internally by inventories.
class CtdmInventoryGroup;
class CtdmInventorySlot;
class CtdmInventoryGroupHistory;

// Note: Modify below comment.
// contents flags, NOTE: make sure to keep the defines in doom_defs.script up to date with these!
// Inventory iteration flags: see iterate() in CtdmInventoryCursor
typedef enum {
	TDMINV_UNGROUPED,
	TDMINV_HYBRID,
	TDMINV_GROUP,
	TDMINV_ITEM
} EtdmInventoryIterationMethod;

/// Adds all inventory-related objects to the savefile's object list.
void tdmInventorySaveObjectList( idSaveGame *savefile );

/// An inventory that can hold items.
class CtdmInventory : public idClass {
	CLASS_PROTOTYPE( CtdmInventory );
  public:

	CtdmInventory();
	~CtdmInventory();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/// The inventory's owner.
	idEntityPtr<idEntity>	m_owner;

  private:

	friend class CtdmInventoryItem;
	friend class CtdmInventoryCursor;
	CtdmInventory(				const CtdmInventory& source ) { /* do nothing */}
	CtdmInventory& operator = (	const CtdmInventory& source ) { /* do nothing */}

	/// Returns a pointer to the group with the given name. Creates the group if it must.
	CtdmInventoryGroup*	obtainGroup( const char* groupName );
	/// Checks if an ungrouped slot should be deallocated, and does so if necessary.
	void				check( CtdmInventorySlot* slot );
	/// Checks if a group and slot should be deallocated, and does so if necessary.
	void				check( CtdmInventoryGroup* group, CtdmInventorySlot* slot = NULL );

	/// Converts a slot into an index. (used for saving)
	unsigned int		SlotToIndex( const CtdmInventorySlot* slot ) const;
	/// Converts an index into a slot. (used for restoring)
	CtdmInventorySlot*	IndexToSlot( unsigned int index, const CtdmInventoryGroup* group = NULL ) const;
	/// Converts a group into an index. (used for saving)
	unsigned int		GroupToIndex( const CtdmInventoryGroup* group ) const;
	/// Converts an index into a group. (used for restoring)
	CtdmInventoryGroup*	IndexToGroup( unsigned int index ) const;

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>					m_inventoryObjListNode;
	/// List of slots for ungrouped inventory.
	idLinkList<CtdmInventorySlot>		m_itemList;
	/// List of groups.
	idLinkList<CtdmInventoryGroup>		m_groupList;
	/// List of cursors pointing to this inventory.
	idLinkList<CtdmInventoryCursor>		m_cursors;

};

/// An item that can be put in an inventory.
class CtdmInventoryItem : public idClass {
	CLASS_PROTOTYPE( CtdmInventoryItem );
  public:

	CtdmInventoryItem();
	~CtdmInventoryItem();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	void				Event_PostRestore();

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
	void				setInventory(	CtdmInventory* inventory,
										const char* groupName = NULL,
										bool afterUngrouped = false,
										CtdmInventoryItem* ungroupedItem = NULL,
										bool afterGrouped = false,
										CtdmInventoryItem* groupedItem = NULL );

	/// Replaces one item with another.
	void				replaceItem( CtdmInventoryItem* item );

	/// Returns the inventory this item is contained by.
	CtdmInventory*		inventory() const;
	/// Gets the item's group.
	const char*			group() const;

	/// The inventory item's owner.
	idEntityPtr<idEntity>	m_owner;

  private:

	friend class CtdmInventory;
	friend class CtdmInventoryCursor;
	CtdmInventoryItem(				const CtdmInventoryItem& source ) { /* do nothing */}
	CtdmInventoryItem& operator = (	const CtdmInventoryItem& source ) { /* do nothing */}

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>		m_inventoryObjListNode;
	/// The name of this item's group.
	idStr					m_groupName;
	/// The inventory this item belongs to.
	CtdmInventory*			m_inventory;
	union {
		/// The group this item belongs to.
		CtdmInventoryGroup*	m_group;
		/// For temporary restoration purposes only.
		unsigned int		m_groupNum;
	};
	union {
		/// The grouped item slot we belong to.
		CtdmInventorySlot*	m_groupedSlot;
		/// For temporary restoration purposes only.
		unsigned int		m_groupedSlotNum;
	};
	union {
		/// The ungrouped item slot we belong to.
		CtdmInventorySlot*	m_ungroupedSlot;
		/// For temporary restoration purposes only.
		unsigned int		m_ungroupedSlotNum;
	};

};

/// A full-blown group-remembering cursor that can handle T1-style iteration and grouped iteration simultaneously.
class CtdmInventoryCursor : public idClass {
	CLASS_PROTOTYPE( CtdmInventoryCursor );
  public:

	CtdmInventoryCursor();
	~CtdmInventoryCursor();

	CtdmInventoryCursor(				const CtdmInventoryCursor& source );
	CtdmInventoryCursor& operator = (	const CtdmInventoryCursor& source );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					Event_PostRestore();

	/// Copies only the active cursor position, not any cursor histories.
	void					copyActiveCursor(	const CtdmInventoryCursor& source,
												bool noHistory = false );

	/// Sets the inventory that this cursor is pointing to.
	void					setInventory( CtdmInventory* inventory );
	/// Returns the inventory this cursor points to.
	CtdmInventory*			inventory() const;

	/// Returns the name of the current inventory group. 'nullOk' causes it to return NULL instead of "" when outside any group.
	const char*				group( bool nullOk = false ) const;

	/// Selects a specific item. (note: cursor must be pointing to the correct inventory)
	void					selectItem( CtdmInventoryItem* item, bool noHistory = false );
	/// Returns the item this cursor is pointing to.
	CtdmInventoryItem*		item() const;

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
	void					iterate(	EtdmInventoryIterationMethod type,
										bool backwards = false, bool noHistory = false,
										bool (*filter)( CtdmInventoryItem* ) = NULL );

	/// The inventory cursor's owner.
	idEntityPtr<idEntity>	m_owner;

  private:

	friend class CtdmInventory;
	friend class CtdmInventoryItem;

	/// Selects the given group and slots, updating reference counts.
	void						select(	CtdmInventoryGroup* group,
										CtdmInventorySlot* groupedSlot,
										CtdmInventorySlot* ungroupedSlot,
										bool noHistory = false );
	/// Find a group history, if it exists. May return NULL.
	CtdmInventoryGroupHistory*	getGroupHistory( CtdmInventoryGroup* group ) const;

	/// Iterates without regard to groups.
	void						iterateUngrouped( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) );
	/// Iterates with items grouped together.
	void						iterateHybrid( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) );
	/// Iterates through the list of groups.
	void						iterateGroup( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) );
	/// Iterates through the items in a group.
	void						iterateItem( bool backwards, bool noHistory, bool (*filter)( CtdmInventoryItem* ) );

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>						m_inventoryObjListNode;
	/// A node so the cursor can be kept in a list by inventories.
	idLinkList<CtdmInventoryCursor>			m_node;
	/// The inventory we're pointing to.
	CtdmInventory*							m_inventory;
	union {
		/// The group our item belongs to.
		CtdmInventoryGroup*					m_group;
		/// For temporary restoration purposes only.
		unsigned int						m_groupNum;
	};
	union {
		/// The grouped item slot we're pointing to. (note: can point to an empty slot while the ungrouped cursor points to a non-empty slot)
		CtdmInventorySlot*					m_groupedSlot;
		/// For temporary restoration purposes only.
		unsigned int						m_groupedSlotNum;
	};
	union {
		/// The ungrouped item slot we're pointing to.
		CtdmInventorySlot*					m_ungroupedSlot;
		/// For temporary restoration purposes only.
		unsigned int						m_ungroupedSlotNum;
	};
	/// A list of cursors histories for various groups.
	idLinkList<CtdmInventoryGroupHistory>	m_groupHistory;
};

/// Used by tdmInventory
class CtdmInventoryGroup {
	friend class CtdmInventory;
	friend class CtdmInventoryItem;
	friend class CtdmInventoryCursor;

	CtdmInventoryGroup( const char* name = NULL );
	~CtdmInventoryGroup();

	/// A node so the group can be kept in a list, by inventores.
	idLinkList<CtdmInventoryGroup>			m_node;
	/// The name of this group.
	idStr									m_name;
	/// A list of contained item slots.
	idLinkList<CtdmInventorySlot>			m_itemList;
	/// The number of items in this group.
	unsigned long							m_numItems;
	/// The number of active cursors pointing to this group.
	unsigned long							m_numCursors;

#ifdef TDMINVENTORY_DEBUG
	static int numGroups;
public:
	static int groups();
#endif
};

/// Used by tdmInventory and tdmInventoryGroup
class CtdmInventorySlot {
	friend class CtdmInventory;
	friend class CtdmInventoryItem;
	friend class CtdmInventoryCursor;
	friend class CtdmInventoryGroup;

	CtdmInventorySlot( CtdmInventoryItem* item = NULL );

	/// A node so this slot can be kept in a list, by inventories and groups.
	idLinkList<CtdmInventorySlot>			m_node;
	/// The item contained in this slot. (can be NULL representing an empty slot with cursors)
	CtdmInventoryItem*						m_item;
	/// The number of active cursors pointing to this slot.
	unsigned int							m_numCursors;
	/// The list of history cursors pointing to this slot.
	idLinkList<CtdmInventoryGroupHistory>	m_historyCursors;

#ifdef TDMINVENTORY_DEBUG
	~CtdmInventorySlot();
	static int numSlots;
	public:
	static int slots();
#endif
};

/// Used by tdmInventoryCursor
class CtdmInventoryGroupHistory {
	friend class CtdmInventory;
	friend class CtdmInventoryItem;
	friend class CtdmInventoryCursor;
	friend class CtdmInventoryGroup;

	CtdmInventoryGroupHistory( CtdmInventoryGroup* group = NULL );

	/// A node so this group history can be kept in a list, by cursors.
	idLinkList<CtdmInventoryGroupHistory>	m_node;
	/// A node so this group history can be kept in a list, by grouped slots.
	idLinkList<CtdmInventoryGroupHistory>	m_slotNode;
	union {
		/// The group this history refers to.
		CtdmInventoryGroup*					m_group;
		/// For temporary restoration purposes only.
		unsigned int						m_groupNum;
	};
	union {
		/// The last slot we used in this group.
		CtdmInventorySlot*					m_slot;
		/// For temporary restoration purposes only.
		unsigned int						m_slotNum;
	};
};

#endif /* __DARKMOD_TDMINVENTORY_H__ */
