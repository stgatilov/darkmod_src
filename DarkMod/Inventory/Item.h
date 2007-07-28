/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 915 $
 * $Date: 2007-04-19 22:10:27 +0200 (Do, 19 Apr 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef __DARKMOD_INVENTORYITEM_H__
#define __DARKMOD_INVENTORYITEM_H__

/* FORWARD DECLS */
class CInventory;
class CInventoryCategory;

/**
 * InventoryItem is an item that belongs to a group.
 */
class CInventoryItem
{
	friend class CInventory;
	friend class CInventoryCursor;
	friend class CInventoryCategory;

public:
	typedef enum {
		IT_ITEM,			// Normal item, which is associated to an entity
		IT_LOOT,			// this is a loot item
		IT_WEAPON,			// a weapon item
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

	// Create an inventoryitem out of the given entity and the given owner
	CInventoryItem(idEntity* itemEntity, idEntity* owner);

	virtual ~CInventoryItem();

	virtual void			Save( idSaveGame *savefile ) const;
	virtual void			Restore( idRestoreGame *savefile );

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

	inline idStr&			GetIcon() { return m_Icon; };

	inline void				SetItemId(const idStr &id) { m_ItemId = id; };
	inline idStr			GetItemId(void) { return m_ItemId; };

	static LootType			getLootTypeFromSpawnargs(const idDict& spawnargs);

	/**
	 * greebo: This returns the number of persistent items contained in this InventoryItem.
	 *         For ordinary persistent items, this is always 1, for non-persistent items this is 0.
	 *         Stackable persistent items will return the current stack count.
	 */
	int						GetPersistentCount();
	void					SetPersistent(bool newValue);

	/**
	 * greebo: When this item is active, the lightgem can be modified by this value.
	 *         The integer value should be between 0 and DARKMOD_LG_MAX and is added to the
	 *         the result of the lightgem renderpasses. Corresponds to the spawnarg "inv_lgmodifier"
	 */
	inline int				GetLightgemModifier() { return m_LightgemModifier; }
	void					SetLightgemModifier(int newValue);

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
	idStr					m_Icon;			// The inventory icon string
	bool					m_Orientated;	// Taken from the entity

	bool					m_Persistent;	// Can be taken to the next map (default is FALSE)

	int						m_LightgemModifier; // Is added to the lightgem when this item is active
};

#endif /* __DARKMOD_INVENTORYITEM_H__ */
