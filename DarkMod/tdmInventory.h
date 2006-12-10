/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.6  2006/12/10 04:53:18  gildoran
 * Completely revamped the inventory code again. I took out the other iteration methods leaving only hybrid (and grouped) iteration. This allowed me to slim down and simplify much of the code, hopefully making it easier to read. It still needs to be improved some, but it's much better than before.
 *
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
	CtdmInventoryGroup*	ObtainGroup( const char* groupName );

	/// Converts a slot into an index. (used for saving)
	unsigned int		SlotToIndex( const CtdmInventorySlot* slot ) const;
	/// Converts an index into a slot. (used for restoring)
	CtdmInventorySlot*	IndexToSlot( unsigned int index ) const;

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>					m_inventoryObjListNode;
	/// List of slots for ungrouped inventory.
	idLinkList<CtdmInventorySlot>		m_slots;
	/// List of groups.
	idLinkList<CtdmInventoryGroup>		m_groups;
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
	 *	When called, enterInventory() moves the item into 'inventory', in the
	 *	group of 'groupName'.
	 *	
	 *	If 'groupName' is NULL, it is assumed to be "".
	 *	
	 *	If 'position' isn't NULL, this item is placed immediately before
	 *	(or after, if 'after' is true) it.
	 *	
	 *	If 'position' is NULL, this item is placed at the beginning (or end,
	 *	if 'after' is true) of the group specified by 'groupName'.
	 *	
	 *	The average usage of enterInventory() is as follows:
	 *		item.enterInventory( inventory, group );
	 *	That will place 'item' in 'group' of 'inventory', at the end.
	 *
	 *	It is an error if 'position' is non-NULL, and not in 'inventory'.
	 */
	void				EnterInventory(	CtdmInventory* inventory,
										const char* groupName = "",
										bool after = true,
										CtdmInventoryItem* position = NULL );

	/// Replaces one item with another.
	void				ReplaceItem( CtdmInventoryItem* item );

	/// Returns the inventory this item is contained by.
	CtdmInventory*		Inventory() const;
	/// Gets the item's group.
	const char*			Group() const;

	/// The inventory item's owner.
	idEntityPtr<idEntity>	m_owner;

  private:

	friend class CtdmInventory;
	friend class CtdmInventoryCursor;
	CtdmInventoryItem(				const CtdmInventoryItem& source ) { /* do nothing */}
	CtdmInventoryItem& operator = (	const CtdmInventoryItem& source ) { /* do nothing */}

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>		m_inventoryObjListNode;
	/// The inventory this item belongs to.
	CtdmInventory*			m_inventory;
	union {
		/// The grouped item slot we belong to.
		CtdmInventorySlot*	m_slot;
		/// For temporary restoration purposes only.
		unsigned int		m_slotNum;
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
	void					CopyActiveCursor(	const CtdmInventoryCursor& source,
												bool noHistory = false );

	/// Sets the inventory that this cursor is pointing to.
	void					SetInventory( CtdmInventory* inventory );
	/// Returns the inventory this cursor points to.
	CtdmInventory*			Inventory() const;

	/// Returns the name of the current inventory group. 'nullOk' causes it to return NULL instead of "" when outside any group.
	const char*				Group( bool nullOk = false ) const;

	/// Selects a specific item. (note: cursor must be pointing to the correct inventory)
	void					SelectItem( CtdmInventoryItem* item, bool noHistory = false );
	/// Returns the item this cursor is pointing to.
	CtdmInventoryItem*		Item() const;

	/// Iterates through the items of the inventory.
	/**	IterateItem() iterates through the inventory, item by item.
	 *	
	 *	If 'backwards' is true, it iterates backwards.
	 *	
	 *	If 'noHistory' is true, it doesn't keep track of the last item
	 *	referenced in each group. This allows it to use less CPU. For the
	 *	player interface, group histories should be used, but for code that
	 *	just scans through an inventory, group histories are just a waste
	 *	of resources and should be turned off.
	 */
	void					IterateItem( bool backwards = false, bool noHistory = false );
	/// Iterates through the groups of the inventory.
	/**	IterateGroup() iterates through the inventory, jumping from group
	 *	to group. Only groups with items in them are selected.
	 *	
	 *	If 'backwards' is true, it iterates backwards.
	 *	
	 *	If 'noHistories' is  false, it will try to jump to the last item/slot
	 *	referenced in a group, or select the first item of the group if no
	 *	group history is available.
	 *	
	 *	If 'noHistories' is true, it will just choose the first item of each
	 *	group, and will not update the group histories to reflect that.
	 */
	void					IterateGroup( bool backwards = false, bool noHistory = false );

	/// The inventory cursor's owner.
	idEntityPtr<idEntity>	m_owner;

  private:

	friend class CtdmInventory;
	friend class CtdmInventoryItem;

	/// Selects the given group and slots, updating reference counts.
	void						Select(	CtdmInventorySlot* slot, bool noHistory = false );
	/// Find a group history, if it exists. May return NULL.
	CtdmInventoryGroupHistory*	GetGroupHistory( CtdmInventoryGroup* group ) const;

	/// Used to keep track of all inventory-related objects.
	idLinkList<idClass>						m_inventoryObjListNode;
	/// The inventory we're pointing to.
	CtdmInventory*							m_inventory;
	/// A node so the cursor can be kept in a list by inventories.
	idLinkList<CtdmInventoryCursor>			m_inventoryNode;
	union {
		/// The ungrouped item slot we're pointing to.
		CtdmInventorySlot*					m_slot;
		/// For temporary restoration purposes only.
		unsigned int						m_slotNum;
	};
	/// A list of cursor histories for various groups.
	idLinkList<CtdmInventoryGroupHistory>	m_groupHistories;
};

/// Used by tdmInventory
class CtdmInventoryGroup {
	friend class CtdmInventory;
	friend class CtdmInventoryItem;
	friend class CtdmInventoryCursor;
	friend class CtdmInventorySlot;
	friend class CtdmInventoryGroupHistory;

	CtdmInventoryGroup( const char* name = NULL );
	~CtdmInventoryGroup();

	// Tells this group to check if it needs to delete itself.
	void Check();

	/// A node so the group can be kept in a list, by inventores.
	idLinkList<CtdmInventoryGroup>			m_inventoryNode;
	/// The name of this group.
	idStr									m_name;
	/// The list of slots in this group.
	idLinkList<CtdmInventorySlot>			m_slots;
	/// The number of items in this group.
	unsigned long							m_numItems;
	/// The number of active cursors pointing to this group.
	unsigned long							m_numActiveCursors;
	/// The list of history cursors pointing to this group.
	idLinkList<CtdmInventoryGroupHistory>	m_historyCursors;

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
	friend class CtdmInventoryGroupHistory;

	CtdmInventorySlot( CtdmInventoryGroup* group, CtdmInventoryItem* item = NULL );
	~CtdmInventorySlot();

	/// Sets the item of this slot.
	void				SetItem( CtdmInventoryItem* item );

	/// Tells this slot that an active cursor is looking at it.
	void				AddActiveCursor();
	/// Tells this slot that an active cursor is not looking at it.
	void				RemoveActiveCursor();

	/// Tells this slot that a history cursor is looking at it.
	void				AddHistoryCursor();
	/// Tells this slot that a history cursor is not looking at it.
	void				RemoveHistoryCursor();

	/// A node so this slot can be kept in a list, by inventories.
	idLinkList<CtdmInventorySlot>			m_inventoryNode;
	/// The group this slot belongs to.
	CtdmInventoryGroup*						m_group;
	/// A node so this slot can be kept in a list, by groups.
	idLinkList<CtdmInventorySlot>			m_groupNode;
	/// The item contained in this slot. (can be NULL representing an empty slot with cursors)
	CtdmInventoryItem*						m_item;
	/// The number of cursors (both active and history) pointing to this slot.
	unsigned int							m_numCursors;

#ifdef TDMINVENTORY_DEBUG
	static int numSlots;
	public:
	static int slots();
#endif
};

/// Used by tdmInventoryCursor
class CtdmInventoryGroupHistory {
	friend class CtdmInventoryCursor;
	friend class CtdmInventoryGroup;

  public:

	CtdmInventoryGroupHistory( idLinkList<CtdmInventoryGroupHistory>* histories, CtdmInventorySlot* slot = NULL );
	~CtdmInventoryGroupHistory();

	void SetSlot( CtdmInventorySlot* slot );

  private:

	/// A node so this group history can be kept in a list, by cursors.
	idLinkList<CtdmInventoryGroupHistory>	m_cursorNode;
	/// A node so this group history can be kept in a list, by groups.
	idLinkList<CtdmInventoryGroupHistory>	m_groupNode;
	union {
		/// The last slot we used in this group.
		CtdmInventorySlot*					m_slot;
		/// For temporary restoration purposes only.
		unsigned int						m_slotNum;
	};
};

#endif /* __DARKMOD_TDMINVENTORY_H__ */
