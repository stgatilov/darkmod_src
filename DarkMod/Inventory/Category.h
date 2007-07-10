/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 915 $
 * $Date: 2007-04-19 22:10:27 +0200 (Do, 19 Apr 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef __DARKMOD_INVENTORYCATEGORY_H__
#define __DARKMOD_INVENTORYCATEGORY_H__

#include "Item.h"

/**
 * InventoryCategory is just a container for items that are currently held by an entity.
 * We put the constructor and PutItem into the protected scope, so we can ensure
 * that groups are only created by using the inventory class.
 */
class CInventoryCategory
{
	friend class CInventory;
	friend class CInventoryCursor;

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
	CInventoryCategory(CInventory* inventory, const idStr& name = "");
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

#endif /* __DARKMOD_INVENTORYCATEGORY_H__ */
