/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 915 $
 * $Date: 2007-04-19 22:10:27 +0200 (Do, 19 Apr 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef __DARKMOD_INVENTORYCURSOR_H__
#define __DARKMOD_INVENTORYCURSOR_H__

#include "Category.h"

class CInventoryCursor
{
	friend class CInventory;

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
	bool					SetCurrentItem(const idStr& name);

	/**
	 * Get the next/prev item in the inventory. Which item is actually returned, 
	 * depends on the settings of CategoryLock and WrapAround.
	 */
	CInventoryItem			*GetNextItem();
	CInventoryItem			*GetPrevItem();

	CInventoryCategory		*GetNextCategory();
	CInventoryCategory		*GetPrevCategory();
	
	/**
	 * Set the current group index.
	 */
	void				SetCurrentCategory(int Index);

	/**
	 * greebo: Returns the category of the focused item
	 *         or NULL if no inventory is attached.
	 */
	CInventoryCategory*      GetCurrentCategory();

	/**
	 * Set the current item index.
	 * Validation of the index is done when doing Nex/Prev Category
	 * so we don't really care wether this is a valid index or not.
	 */
	inline void				SetCurrentItem(int Index) { m_CurrentItem = Index; }

	/**
	 * Returns the current index within the category of the item pointed at.
	 */
	int							GetCurrentItemIndex() { return m_CurrentItem; }

	inline void				SetCategoryLock(bool bLock) { m_CategoryLock = bLock; }
	inline void				SetWrapAround(bool bWrap) { m_WrapAround = bWrap; }

	void					RemoveCategoryIgnored(const CInventoryCategory *);
	void					RemoveCategoryIgnored(const idStr& categoryName);

	void					SetCategoryIgnored(const CInventoryCategory *);
	void					SetCategoryIgnored(const idStr& categoryName);

protected:
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

#endif /* __DARKMOD_INVENTORYCURSOR_H__ */
