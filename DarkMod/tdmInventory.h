/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $LastChangedRevision$
 * $Date$
 * $Author$
 * $Id$
 *
 ***************************************************************************/

#ifndef __DARKMOD_TDMINVENTORY_H__
#define __DARKMOD_TDMINVENTORY_H__

#define TDM_INVENTORY_DEFAULT_GROUP		"DEFAULT"
#define TDM_LOOT_INFO_ITEM				"loot_info"
#define TDM_LOOT_SCRIPTOBJECT			"loot_gui"
#define TDM_DUMMY_ITEM					"dummy"

// Classes that should be used by external files.
class CInventory;
class CInventoryCategory;
class CInventoryItem;

/**
 * Inventorygroup is just a container for items that are currently held by an entity.
 * We put the constructor and PutItem into the protected scope, so we can ensure
 * that groups are only created by using the inventory class.
 */
class CInventoryCategory
{
	friend CInventory;
	friend CInventoryCursor;

public:
	inline idStr		&GetName() { return m_Name; }
	inline void			SetInventory(CInventory *Inventory) { m_Inventory = Inventory; };

	inline idEntity			*GetOwner(void) { return m_Owner.GetEntity(); };

	CInventoryItem		*GetItemById(const char *Name);
	inline CInventoryItem *GetItemById(const idStr &Name) { return GetItemById(Name.c_str()); };

	CInventoryItem		*GetItem(const char *Name);
	inline CInventoryItem	*GetItem(const idStr &Name) { return GetItem(Name.c_str()); };
	CInventoryItem		*GetItem(int Index);
	int					GetItemIndex(const idStr &Name);
	int					GetItemIndex(CInventoryItem *);

	int					GetLoot(int &Gold, int &Jewelry, int &Goods);

protected:
	CInventoryCategory(idEntity *owner, const char* name = NULL);
	~CInventoryCategory();

	void				PutItem(CInventoryItem *Item);
	void				SetOwner(idEntity *Owner);
	void				Save(idSaveGame *savefile) const;
	void				Restore(idRestoreGame *savefile);

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
	friend CInventoryCursor;
	friend CInventoryCategory;

public:
	typedef enum {
		IT_ITEM,			// Normal item, which is associated to an entity
		IT_LOOT,			// this is a loot item
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
	
	inline idEntity			*GetOwner(void) { return m_Owner.GetEntity(); };

	inline void				SetItemEntity(idEntity *ent) { m_Item = ent; };
	inline idEntity			*GetItemEntity() { return m_Item.GetEntity(); }
	inline void				SetType(CInventoryItem::ItemType type) { m_Type = type; };
	inline					ItemType GetType(void) { return m_Type; };

	inline int				GetCount(void) { return m_Count; };
	void					SetCount(int Amount);

	inline bool				IsStackable(void) { return m_Stackable; };
	void					SetStackable(bool);

	inline void				SetDroppable(bool bDroppable) { m_Droppable = bDroppable; };
	inline bool				IsDroppable(void) { return m_Droppable; };

	void					SetLootType(CInventoryItem::LootType t);
	inline LootType			GetLootType(void) { return m_LootType; };

	void					SetValue(int n);
	inline int				GetValue(void) { return m_Value; };

	inline void				SetName(const idStr &n) { m_Name = n; };
	inline idStr			GetName(void) { return m_Name; };

	inline void				SetItem(idEntity *item) { m_Item = item; };
	inline idEntity			*GetItem(void) { return m_Item.GetEntity(); };

	inline int				GetOverlay(void) { return m_Overlay; };
	void					SetOverlay(const idStr &HudName, int Overlay);
	bool					HasHUD(void) { return m_Hud; };
	void					SetHUD(const idStr &HudName, int layer);
	inline idStr			GetHUD(void) { return m_HudName; };

	void					SetDeleteable(bool bDeleteable = true);
	inline bool				IsDeletable(void) { return m_Deleteable; };

	inline void				SetItemId(const idStr &id) { m_ItemId = id; };
	inline idStr			GetItemId(void) { return m_ItemId; };

protected:
	idEntityPtr<idEntity>	m_Owner;
	idEntityPtr<idEntity>	m_Item;
	idEntityPtr<idEntity>	m_BindMaster;
	idStr					m_Name;
	idStr					m_HudName;		// filename for the hud file if it has a custom hud
	idStr					m_ItemId;		// Arbitrary Id, that the mapper can use to idenitfy an item.
											// It is also needed to identify items which are stackable
											// to make them identifiable. This can be used, for example
											// to create a fake health potion which shows up in the inventory
											// as a regular health potion, but actually deals damage. Needs
											// custom scripting to actually do this though.
	CInventory				*m_Inventory;
	CInventoryCategory		*m_Category;
	ItemType				m_Type;
	LootType				m_LootType;
	int						m_Value;
	int						m_Overlay;
	int						m_Count;		// How many of that item are currently represented (i.e. Arrows)
	bool					m_Stackable;	// Counter can be used if true, otherwise it's a unique item
	bool					m_Droppable;		// If the item is not dropable it will be inaccessible after it 
											// is put into the inventory
	bool					m_Hud;
	bool					m_Orientated;	// Taken from the entity
	bool					m_Deleteable;	// Can the entity be deleted when it has been put into the inventory?

};

/**
 * An inventory is a container for groups of items. An inventory has one default group
 * which is always created. All other groups are up to the mapper to decide.
 *
 * If an item is put into an inventory, without specifying the group, it will be put
 * in the default group which is always at index 0 and has the name DEFAULT.
 */
class CInventoryCursor
{
	friend CInventory;

protected:
	CInventoryCursor(CInventory *);
	~CInventoryCursor();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

public:
	inline CInventory		*Inventory() { return m_Inventory; };
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

	CInventoryCategory		*GetNextCategory(void);
	CInventoryCategory		*GetPrevCategory(void);

	/**
	 * Set the current group index.
	 */
	void				SetCurrentCategory(int Index);

	/**
	 * Set the current item index.
	 * Validation of the index is done when doing Nex/Prev Category
	 * so we don't really care wether this is a valid index or not.
	 */
	inline void				SetCurrentItem(int Index) { m_CurrentItem = Index; }
	inline void				SetCategoryLock(bool bLock) { m_CategoryLock = bLock; }
	inline void				SetWrapAround(bool bWrap) { m_WrapAround = bWrap; }

	void					RemoveCategoryIgnored(const CInventoryCategory *);
	void					RemoveCategoryIgnored(const char *CategoryName);
	inline void				RemoveCategoryIgnored(const idStr &Name) { RemoveCategoryIgnored(Name.c_str()); };

	void					SetCategoryIgnored(const CInventoryCategory *);
	void					SetCategoryIgnored(const char *CategoryName);
	inline void				SetCategoryIgnored(const idStr &Name) { SetCategoryIgnored(Name.c_str()); };

	void					DropCurrentItem(void);

protected:
	void					ValidateCategory(void);
	bool					IsCategoryIgnored(const CInventoryCategory *) const;

protected:
	CInventory				*m_Inventory;

	/**
	 * List of categories that should be ignored for this cursor.
	 * The resulting behaviour is that the cursor will cycle through
	 * items and categories, as if categories, which listed here, don't exist.
	 */
	idList<const CInventoryCategory *> m_CategoryIgnore;

	/**
	 * If true it means that the scrolling with next/prev is locked
	 * to the current group when a warparound occurs. In this case 
	 * the player has to use the next/prev group keys to switch to
	 * a different group.
	 */
	bool					m_CategoryLock;

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

	int						m_CursorId;
};

class CInventory : public idClass
{
public:
	CLASS_PROTOTYPE(CInventory);

	friend CInventoryCursor;

	CInventory();
	~CInventory();

	CInventoryCursor		*CreateCursor(void);

	int						GetLoot(int &Gold, int &Jewelry, int &Goods);

	inline idEntity			*GetOwner(void) { return m_Owner.GetEntity(); };
	void					SetOwner(idEntity *Owner);

	/**
	 * CreateCategory creates the named group if it doesn't already exist.
	 */
	CInventoryCategory		*CreateCategory(const char *CategoryName, int *Index = NULL);
	inline CInventoryCategory	*CreateCategory(idStr const &CategoryName, int *Index = NULL) { return CreateCategory(CategoryName.c_str(), Index); }

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
	int						GetCategoryItemIndex(CInventoryItem *Item, int *ItemIndex = NULL);
	int						GetCategoryItemIndex(const char *ItemName, int *ItemIndex = NULL);
	inline int				GetCategoryItemIndex(const idStr &ItemName, int *ItemIndex = NULL) 
									{ return GetCategoryItemIndex(ItemName.c_str(), ItemIndex); };

	/**
	 * Remove entity from map will remove the entity from the map. If 
	 * bDelete is set, the entity will be deleted as well. This can only
	 * be done if droppable is set to 0 as well. Otherwise it may be
	 * possible for the player to drop the item, in which case the entity 
	 * must stay around.
	 */
	void					RemoveEntityFromMap(idEntity *ent, bool bDelete = false);

	/**
	 * Puts an item back into the map. This can only be done when the item is
	 * droppable, otherwise it is ignored. If the item was an objective, the 
	 * objective will to be toggled back again. Owner is needed to retrieve the
	 * position where the item will be released.
	 */
	void					PutEntityInMap(idEntity *ent, idEntity *owner, CInventoryItem *item);

	/**
	 * Put an item in the inventory. Use the default group if none is specified.
	 * The name, that is to be displayed on a GUI, must be set on the respective
	 * entity.
	 */
	CInventoryItem			*PutItem(idEntity *Item, idEntity *Owner);
	void					PutItem(CInventoryItem *Item, char const *Category = NULL);

	/**
	 * Retrieve an item from an inventory. If no group is specified, all of 
	 * them are searched, otherwise only the given group.
	 */
	CInventoryItem			*GetItem(const char *Name, char const *Category = NULL, bool bCreateCategory = false);
	inline CInventoryItem	*GetItem(const idStr &Name, char const *Category = NULL, bool bCreateCategory = false)
						{ return GetItem(Name.c_str(), Category, bCreateCategory); } ;

	CInventoryItem			*GetItemById(const char *Name, char const *Category = NULL, bool bCreateCategory = false);
	inline CInventoryItem	*GetItemById(const idStr &Name, char const *Category = NULL, bool bCreateCategory = false)
						{ return GetItemById(Name.c_str(), Category, bCreateCategory); } ;

protected:
	void				Save(idSaveGame *savefile) const;
	void				Restore(idRestoreGame *savefile);
	CInventoryItem		*ValidateLoot(idEntity *ent, CInventoryItem::LootType, int value);

protected:
	idEntityPtr<idEntity>				m_Owner;

	idList<CInventoryCursor *>			m_Cursor;

	/**
	 * List of groups in that inventory
	 */
	idList<CInventoryCategory *>		m_Category;

	/**
	 * Here we keep the lootcount for the items, that don't need to actually 
	 * be stored in the inventory, because they can't get displayed anyway.
	 * LootItemCount will only count the items, that are stored here. Items
	 * that are visible, will not be counted here. This is only for stats.
	 */
	int						m_LootItemCount;
	int						m_Gold;
	int						m_Jewelry;
	int						m_Goods;
};


#endif /* __DARKMOD_TDMINVENTORY_H__ */
