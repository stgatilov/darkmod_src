/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.13  2007/02/07 22:06:25  sparhawk
 * Items can now be frobbed and added to the inventory
 *
 * Revision 1.12  2007/02/03 21:56:21  sparhawk
 * Removed old inventories and fixed a bug in the new one.
 *
 * Revision 1.11  2007/02/03 18:07:39  sparhawk
 * Loot items implemented and various improvements to the interface.
 *
 * Revision 1.10  2007/02/01 19:47:35  sparhawk
 * Callback for inventory added.
 *
 * Revision 1.9  2007/01/31 23:41:49  sparhawk
 * Inventory updated
 *
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

#define TDM_INVENTORY_DEFAULT_GROUP		"DEFAULT"

// Classes that should be used by external files.
class CInventory;
class CInventoryCategory;
class CInventoryItem;

/**
 * An inventory is a container for groups of items. An inventory has one default group
 * which is always created. All other groups are up to the mapper to decide.
 *
 * If an item is put into an inventory, without specifying the group, it will be put
 * in the default group which is always at index 0 and has the name DEFAULT.
 */
class CInventory : public idClass
{
	CLASS_PROTOTYPE(CInventory);

public:
	CInventory();
	~CInventory();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	 * GetCategory returns the pointer to the given group and it's index, 
	 * if the pointer is not NULL.
	 */
	CInventoryCategory	*GetCategory(const char *CategoryName = NULL, int *Index = NULL);
	inline CInventoryCategory	*GetCategory(idStr const &CategoryName, int *Index = NULL) { return GetCategory(CategoryName.c_str(), Index); }

	/**
	 * GetCategoryIndex returns the index to the given group or -1 if not found.
	 */
	int					GetCategoryIndex(const char *CategoryName);
	inline int			GetCategoryIndex(idStr const &CategoryName) { return GetCategoryIndex(CategoryName.c_str()); }

	/**
	 * Return the groupindex of the item or -1 if it doesn't exist. Optionally
	 * the itemindex within that group can also be obtained. Both are set to -1 
	 * if the item can not be found. The ItemIndex pointer only when it is not NULL of course.
	 */
	int GetCategoryItemIndex(CInventoryItem *Item, int *ItemIndex = NULL);
	int GetCategoryItemIndex(const char *ItemName, int *ItemIndex = NULL);
	inline int GetCategoryItemIndex(const idStr &ItemName, int *ItemIndex = NULL) { return GetCategoryItemIndex(ItemName.c_str(), ItemIndex); };

	/**
	 * CreateCategory creates the named group if it doesn't already exist.
	 */
	CInventoryCategory	*CreateCategory(const char *CategoryName, int *Index = NULL);
	inline CInventoryCategory	*CreateCategory(idStr const &CategoryName, int *Index = NULL) { return CreateCategory(CategoryName.c_str(), Index); }

	idEntity			*GetOwner(void) { return m_Owner.GetEntity(); }
	void				SetOwner(idEntity *Owner);

	/**
	 * Put an item in the inventory. Use the default group if none is specified.
	 * The name, that is to be displayed on a GUI, must be set on the respective
	 * entity.
	 */
	CInventoryItem	*PutItem(idEntity *Item, const idStr &Name, char const *Category = NULL);
	void				PutItem(CInventoryItem *Item, char const *Category = NULL);

	/**
	 * Retrieve an item from an inventory. If no group is specified, all of 
	 * them are searched, otherwise only the given group.
	 */
	CInventoryItem			*GetItem(const char *Name, char const *Category = NULL);
	inline CInventoryItem	*GetItem(const idStr &Name, char const *Category = NULL) { return GetItem(Name.c_str(), Category); } ;

	/**
	 * Retrieve the currently selected item.
	 */
	CInventoryItem			*GetCurrentItem();
	bool					SetCurrentItem(CInventoryItem *Item);
	bool					SetCurrentItem(const char *name);
	inline bool				SetCurrentItem(const idStr &Name) { return SetCurrentItem(Name.c_str()); };

	/**
	 * Get the next/prev item in the inventory. Which item is actually returned, 
	 * depends on the settings of CategoryLock and WrapAround.
	 */
	CInventoryItem			*GetNextItem(void);
	CInventoryItem			*GetPrevItem(void);

	CInventoryCategory			*GetNextCategory(void);
	CInventoryCategory			*GetPrevCategory(void);

	/**
	 * Set the current group index.
	 * Validation of the index is done when doing Nex/Prev Category
	 * so we don't really care wether this is a valid index or not.
	 */
	inline void				SetCurrentCategory(int Index) { m_CurrentCategory = Index; }

	/**
	 * Set the current item index.
	 * Validation of the index is done when doing Nex/Prev Category
	 * so we don't really care wether this is a valid index or not.
	 */
	inline void				SetCurrentItem(int Index) { m_CurrentItem = Index; }
	inline void				SetCategoryLock(bool bLock) { m_CategoryLock = bLock; }
	inline void				SetWrapAround(bool bWrap) { m_WrapAround = bWrap; }

	int						GetLoot(int &Gold, int &Jewelry, int &Goods);

protected:
	void					ValidateCategory(void);

protected:
	idEntityPtr<idEntity>	m_Owner;

	/**
	 * List of groups in that inventory
	 */
	idList<CInventoryCategory *>		m_Category;

	/**
	 * If true it means that the scrolling with next/prev is locked
	 * to the current group when a warparound occurs. In this case 
	 * the player has to use the next/prev group keys to switch to
	 * a different group.
	 */
	bool							m_CategoryLock;

	/**
	 * If set to true the inventory will start from the first/last item
	 * if it is scrolled beyond the last/first item. Depending on the
	 * CategoryLock setting, this may mean that, when the last item is reached
	 * and CategoryLock is false, NextItem will yield the first item of
	 * the first group, and vice versa in the other direction. This would be
	 * the behaviour like the original Thief games inventory.
	 * If CategoryLock is true and WrapAround is true it will also go to the first
	 * item, but stays in the same group.
	 * If WrapAround is false and the last/first item is reached, Next/PrevItem
	 * will always return the same item on each call;
	 */
	bool					m_WrapAround;

	/**
	 * CurrentCategory is the index of the current group for using Next/PrevItem
	 */
	int						m_CurrentCategory;

	/**
	 * CurrentItem is the index of the current item in the current group for
	 * using Next/PrevItem.
	 */
	int						m_CurrentItem;
};

/**
 * Inventorygroup is just a container for items that are currently held by an entity.
 */
class CInventoryCategory
{
	friend CInventory;

public:
	CInventoryCategory(const char* name = NULL);
	~CInventoryCategory();

	void				Save(idSaveGame *savefile) const;
	void				Restore(idRestoreGame *savefile);

	inline idStr		&GetName() { return m_Name; }
	inline void			SetInventory(CInventory *Inventory) { m_Inventory = Inventory; }

	idEntity			*GetOwner(void) { return m_Owner.GetEntity(); }

	CInventoryItem		*PutItem(idEntity *Item, const idStr &Name);
	void				PutItem(CInventoryItem *Item);

	CInventoryItem		*GetItem(const idStr &Name);
	CInventoryItem		*GetItem(int Index);
	int					GetItemIndex(const idStr &Name);
	int					GetItemIndex(CInventoryItem *);

	int					GetLoot(int &Gold, int &Jewelry, int &Goods);

protected:
	void				SetOwner(idEntity *Owner);

protected:
	CInventory					*m_Inventory;			// The inventory this group belongs to.
	idEntityPtr<idEntity>		m_Owner;

	// The name of this group.
	idStr						m_Name;

	// A list of contained item.
	idList<CInventoryItem *>	m_Item;
};

/**
 * InventoryItem is a item that belongs to a group.
 */
class CInventoryItem
{
	friend CInventory;
	friend CInventoryCategory;

public:
	typedef enum {
		IT_ITEM,			// Normal item, which is associated to an entity
		IT_LOOT,			// this is a loot item
		IT_LOOT_INFO,		// Special loot info item, which doesn't have an entity
		IT_DUMMY,			// This also doesn't have an entity, but provides a dummy so 
							// we can have an empty space in the inventory.
		IT_COUNT
	} ItemType;

	typedef enum {
		LT_NONE,			// No lootobject
		LT_JEWELS,
		LT_GOLD,
		LT_GOODS,
		LT_COUNT		// dummy
	} LootType;

public:
	CInventoryItem(idEntity *m_Owner);
	~CInventoryItem();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	inline CInventory		*Inventory() const { return m_Inventory; }
	inline CInventoryCategory	*Category() const { return m_Category; }
	inline idEntity			*GetOwner(void) { return m_Owner.GetEntity(); }
	inline idEntity			*GetEntity() { return m_Item.GetEntity(); }
	inline void				SetType(CInventoryItem::ItemType type) { m_Type = type; };
	inline					ItemType GetType(void) { return m_Type; };

	inline int				GetCount(void) { return m_Count; };
	void					SetCount(int Amount);

	inline bool				IsStackable(void) { return m_Stackable; };
	void					SetStackable(bool);

	void					SetLootType(CInventoryItem::LootType t);
	inline LootType			GetLootType(void) { return m_LootType; };

	void					SetValue(int n);
	inline int				GetValue(void) { return m_Value; };

	inline void				SetName(const idStr &n) { m_Name = n; };
	inline idStr			GetName(void) { return m_Name; };

	inline void				SetItem(idEntity *item) { m_Item = item; };
	inline idEntity			*GetItem(void) { return m_Item.GetEntity(); };

protected:
	idEntityPtr<idEntity>	m_Owner;
	idEntityPtr<idEntity>	m_Item;
	idStr					m_Name;
	CInventory				*m_Inventory;
	CInventoryCategory		*m_Category;
	ItemType				m_Type;
	LootType				m_LootType;
	int						m_Value;
	int						m_Count;		// How many of that item are currently represented (i.e. Arrows)
	bool					m_Stackable;	// Counter can be used if true, otherwise it's a unique item
	bool					m_Droppable;	// If the item is not droppable it will be inaccessible after it 
											// is put into the inventory
};

#endif /* __DARKMOD_TDMINVENTORY_H__ */
