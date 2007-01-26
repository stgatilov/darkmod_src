/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.8  2007/01/26 12:52:50  sparhawk
 * New inventory concept.
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

/**
 * An inventory is a container for groups of items. An inventory has one default group
 * which is always created. All other groups are up to the mapper to decide.
 *
 * If an item is put into an inventory, without specifying the group, it will be put
 * in the default group which is always at index 0 and has the name DEFAULT.
 */
class CtdmInventory : public idClass
{
	CLASS_PROTOTYPE(CtdmInventory);

public:
	CtdmInventory();
	~CtdmInventory();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	 * GetGroup returns the pointer to the given group and it's index, 
	 * if the pointer is not NULL.
	 */
	CtdmInventoryGroup	*GetGroup(const char *GroupName, int *Index = NULL);
	CtdmInventoryGroup	*GetGroup(idStr const &GroupName, int *Index = NULL) { return GetGroup(GroupName.c_str(), Index); }

	/**
	 * GetGroupIndex returns the index to the given group or -1 if not found.
	 */
	int					GetGroupIndex(const char *GroupName);
	int					GetGroupIndex(idStr const &GroupName) { return GetGroupIndex(GroupName.c_str()); }

	/**
	 * CreateGroup creates the named group if it doesn't already exist.
	 */
	CtdmInventoryGroup	*CreateGroup(const char *GroupName, int *Index = NULL);
	CtdmInventoryGroup	*CreateGroup(idStr const &GroupName, int *Index = NULL) { return CreateGroup(GroupName.c_str(), Index); }

	idEntity			*GetOwner(void) { return m_Owner.GetEntity(); }
	void				SetOwner(idEntity *Owner);

	/**
	 * Put an item in the inventory. Use the default group if none is specified.
	 * The name, that is to be displayed on a GUI, must be set on the respective
	 * entity.
	 */
	CtdmInventoryItem	*PutItem(idEntity *Item, const idStr &Name, char const *Group = NULL);

	/**
	 * Retrieve an item from an inventory. If no group is specified, all of 
	 * them are searched, otherwise only the given group.
	 */
	idEntity			*GetItem(const idStr &Name, char const *Group = NULL);

	/**
	 * Get the next/prev item in the inventory. Which item is actually returned, 
	 * depends on the settings of GroupLock and WrapAround.
	 */
	idEntity			*GetNextItem(void);
	idEntity			*GetPrevItem(void);

	CtdmInventoryGroup	*GetNextGroup(void);
	CtdmInventoryGroup	*GetPrevGroup(void);

	/**
	 * Set the current group index.
	 * Validation of the index is done when doing Nex/Prev Group
	 * so we don't really care wether this is a valid index or not.
	 */
	void				SetCurrentGroup(int Index) { m_CurrentGroup = Index; }

	/**
	 * Set the current item index.
	 * Validation of the index is done when doing Nex/Prev Group
	 * so we don't really care wether this is a valid index or not.
	 */
	void				SetCurrentItem(int Index) { m_CurrentItem = Index; }

	void				SetGroupLock(bool bLock) { m_GroupLock = bLock; }

	void				SetWrapAround(bool bWrap) { m_WrapAround = bWrap; }

protected:
	void				ValidateGroup(void);

protected:
	idEntityPtr<idEntity>			m_Owner;

	/**
	 * List of groups in that inventory
	 */
	idList<CtdmInventoryGroup *>	m_Group;

	/**
	 * If true it means that the scrolling with next/prev is locked
	 * to the current group when a warparound occurs. In this case 
	 * the player has to use the next/prev group keys to switch to
	 * a different group.
	 */
	bool							m_GroupLock;

	/**
	 * If set to true the inventory will start from the first/last item
	 * if it is scrolled beyond the last/first item. Depending on the
	 * GroupLock setting, this may mean that, when the last item is reached
	 * and GroupLock is false, NextItem will yield the first item of
	 * the first group, and vice versa in the other direction. This would be
	 * the behaviour like the original Thief games inventory.
	 * If GroupLock is true and WrapAround is true it will also go to the first
	 * item, but stays in the same group.
	 * If WrapAround is false and the last/first item is reached, Next/PrevItem
	 * will always return the same item on each call;
	 */
	bool							m_WrapAround;

	/**
	 * CurrentGroup is the index of the current group for using Next/PrevItem
	 */
	int								m_CurrentGroup;

	/**
	 * CurrentItem is the index of the current item in the current group for
	 * using Next/PrevItem.
	 */
	int								m_CurrentItem;
};

/**
 * Inventorygroup is just a container for items that are currently held by an entity.
 */
class CtdmInventoryGroup : public idClass
{
	CLASS_PROTOTYPE(CtdmInventoryGroup);
	friend CtdmInventory;

public:
	CtdmInventoryGroup(const char* name = NULL);
	~CtdmInventoryGroup();

	void		Save(idSaveGame *savefile) const;
	void		Restore(idRestoreGame *savefile);

	idStr		&GetName() { return m_Name; }
	void		SetInventory(CtdmInventory *Inventory) { m_Inventory = Inventory; }

	idEntity			*GetOwner(void) { return m_Owner.GetEntity(); }

	CtdmInventoryItem	*PutItem(idEntity *Item, const idStr &Name);
	idEntity			*GetItem(const idStr &Name);

protected:
	void				SetOwner(idEntity *Owner);

protected:
	CtdmInventory					*m_Inventory;			// The inventory this group belongs to.
	idEntityPtr<idEntity>			m_Owner;

	// The name of this group.
	idStr							m_Name;

	// A list of contained item.
	idList<CtdmInventoryItem *>		m_Item;
};

/**
 * InventoryItem is a item that belongs to a group.
 */
class CtdmInventoryItem : public idClass
{
	CLASS_PROTOTYPE( CtdmInventoryItem );
	friend CtdmInventory;
	friend CtdmInventoryGroup;

public:
	CtdmInventoryItem();
	~CtdmInventoryItem();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	CtdmInventory		*Inventory() const { return m_Inventory; }
	CtdmInventoryGroup	*Group() const { return m_Group; }
	idEntity			*GetOwner(void) { return m_Owner.GetEntity(); }

protected:
	idEntityPtr<idEntity>	m_Owner;
	idEntityPtr<idEntity>	m_Item;
	idStr					m_Name;
	CtdmInventory			*m_Inventory;
	CtdmInventoryGroup		*m_Group;
};

// A full-blown group-remembering cursor that can handle T1-style iteration and grouped iteration simultaneously.
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
