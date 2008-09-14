/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
#ifndef __DARKMOD_INVENTORYCURSOR_H__
#define __DARKMOD_INVENTORYCURSOR_H__

#include "Category.h"

class CInventoryCursor
{
public:
	/**
	 * greebo: Construct this cursor with a pointer to the parent inventory and its ID.
	 */
	CInventoryCursor(CInventory* inventory, int id);

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	inline CInventory*		Inventory() { return m_Inventory; };
	/**
	 * Retrieve the currently selected item.
	 */
	CInventoryItemPtr		GetCurrentItem();
	bool					SetCurrentItem(CInventoryItemPtr item);
	bool					SetCurrentItem(const idStr& itemName);

	/**
	 * greebo: Clears the cursor. After calling this, the cursor is pointing
	 *         at nothing. The category lock and other settings are not affected by this call.
	 */
	void					ClearItem();

	/**
	 * Get the next/prev item in the inventory. Which item is actually returned, 
	 * depends on the settings of CategoryLock and WrapAround.
	 */
	CInventoryItemPtr		GetNextItem();
	CInventoryItemPtr		GetPrevItem();

	CInventoryCategoryPtr	GetNextCategory();
	CInventoryCategoryPtr	GetPrevCategory();
	
	/**
	 * Set the current group index.
	 */
	void					SetCurrentCategory(int index);

	/**
	 * greebo: Returns the category of the focused item or NULL if no inventory is attached.
	 */
	CInventoryCategoryPtr	GetCurrentCategory();

	/**
	 * Set the current item index.
	 * Validation of the index is done when doing Nex/Prev Category
	 * so we don't really care whether this is a valid index or not.
	 */
	void						SetCurrentItem(int index) { m_CurrentItem = index; }

	/**
	 * Returns the current index within the category of the item pointed at.
	 */
	int							GetCurrentItemIndex() { return m_CurrentItem; }

	void						SetCategoryLock(bool bLock) { m_CategoryLock = bLock; }
	void						SetWrapAround(bool bWrap) { m_WrapAround = bWrap; }

	void						RemoveCategoryIgnored(const CInventoryCategoryPtr& category);
	void						RemoveCategoryIgnored(const idStr& categoryName);

	void						AddCategoryIgnored(const CInventoryCategoryPtr& category);
	void						AddCategoryIgnored(const idStr& categoryName);

	// Returns the unique ID of this cursor
	int							GetId();

protected:
	bool						IsCategoryIgnored(const CInventoryCategoryPtr& category) const;

protected:

	// The inventory we're associated with
	CInventory* m_Inventory;

	/**
	 * List of category indices that should be ignored for this cursor.
	 * The resulting behaviour is that the cursor will cycle through
	 * items and categories, as if the categories in this list didn't exist.
	 */
	idList<int> m_CategoryIgnore;

	/**
	 * If true it means that the scrolling with next/prev is locked
	 * to the current group when a warparound occurs. In this case 
	 * the player has to use the next/prev group keys to switch to
	 * a different group.
	 */
	bool m_CategoryLock;

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
	bool m_WrapAround;

	/**
	 * CurrentCategory is the index of the current group for using Next/PrevItem
	 */
	int m_CurrentCategory;

	/**
	 * CurrentItem is the index of the current item in the current group for
	 * using Next/PrevItem.
	 */
	int m_CurrentItem;

	// The unique ID of this cursor
	int m_CursorId;
};
typedef boost::shared_ptr<CInventoryCursor> CInventoryCursorPtr;

#endif /* __DARKMOD_INVENTORYCURSOR_H__ */
