/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#pragma warning(disable : 4533 4800)

static bool init_version = FileVersionList("$Id$", init_version);

#include "Cursor.h"

#include "Inventory.h"

CInventoryCursor::CInventoryCursor(CInventory* inventory, int id) :
	m_Inventory(inventory),
	m_CategoryLock(false),	// Default behaviour ...
	m_WrapAround(true),		// ... is like standard Thief inventory.
	m_CurrentCategory(0),
	m_CurrentItem(0),
	m_CursorId(id)
{}

int	CInventoryCursor::GetId()
{
	return m_CursorId;
}

void CInventoryCursor::Save(idSaveGame *savefile) const
{
	savefile->WriteBool(m_CategoryLock);
	savefile->WriteBool(m_WrapAround);
	savefile->WriteInt(m_CurrentCategory);
	savefile->WriteInt(m_CurrentItem);
	savefile->WriteInt(m_CursorId);

	savefile->WriteInt(m_CategoryIgnore.Num());
	for (int i = 0; i < m_CategoryIgnore.Num(); i++)
	{
		savefile->WriteInt(m_CategoryIgnore[i]);
	}
}

void CInventoryCursor::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool(m_CategoryLock);
	savefile->ReadBool(m_WrapAround);
	savefile->ReadInt(m_CurrentCategory);
	savefile->ReadInt(m_CurrentItem);
	savefile->ReadInt(m_CursorId);

	int num;
	savefile->ReadInt(num);
	for (int i = 0; i < num; i++)
	{
		int ignoreIndex;
		savefile->ReadInt(ignoreIndex);
		m_CategoryIgnore.AddUnique(ignoreIndex);
	}
}

CInventoryItem* CInventoryCursor::GetCurrentItem()
{
	// Return an item if the inventory has items
	return (m_Inventory->GetNumCategories() > 0) ? m_Inventory->GetCategory(m_CurrentCategory)->GetItem(m_CurrentItem) : NULL;
}

void CInventoryCursor::ClearItem()
{
	// greebo: Invalidate the item index, this should be enough
	m_CurrentItem = -1;
}

bool CInventoryCursor::SetCurrentItem(CInventoryItem* item)
{
	if (item == NULL)
	{
		// NULL item passed, which means "clear cursor": replace the pointer with a pointer to the dummy item
		item = m_Inventory->GetItem(TDM_DUMMY_ITEM);

		if (item == NULL) return false;
	}

	// retrieve the category and item index for the given inventory item
	int itemIdx = -1;
	int category = m_Inventory->GetCategoryItemIndex(item, &itemIdx);

	if (category == -1) return false; // category not found

	// Only change the group and item indices if they are valid.
	m_CurrentCategory = category;
	m_CurrentItem = itemIdx;

	return true;
}

bool CInventoryCursor::SetCurrentItem(const idStr& itemName)
{
	if (itemName.IsEmpty()) return false;

	int itemIdx = -1;
	int category = m_Inventory->GetCategoryItemIndex(itemName, &itemIdx);

	if (category == -1) return false; // category not found

	// Only change the group and item indices if they are valid.
	m_CurrentCategory = category;
	m_CurrentItem = itemIdx;

	return true;
}

CInventoryItem* CInventoryCursor::GetNextItem()
{
	CInventoryCategory* curCategory = m_Inventory->GetCategory(m_CurrentCategory);

	if (curCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Current Category doesn't exist anymore!\r", m_CurrentCategory);
		return NULL;
	}

	// Advance our cursor
	m_CurrentItem++;

	// Have we reached the end of the current category?
	if (m_CurrentItem >= curCategory->GetNumItems())
	{
		// Advance to the next allowed category
		curCategory = GetNextCategory();

		if (m_WrapAround) 
		{
			m_CurrentItem = 0;
		}
		else
		{
			m_CurrentItem = curCategory->GetNumItems() - 1;
		}
	}

	return curCategory->GetItem(m_CurrentItem);
}

CInventoryItem *CInventoryCursor::GetPrevItem()
{
	CInventoryCategory* curCategory = m_Inventory->GetCategory(m_CurrentCategory);

	if (curCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Current Category doesn't exist anymore!\r", m_CurrentCategory);
		return NULL;
	}

	// Move our cursor backwards
	m_CurrentItem--;

	if (m_CurrentItem < 0)
	{
		curCategory = GetPrevCategory();

		if (m_WrapAround)
		{
			m_CurrentItem = curCategory->GetNumItems() - 1;
		}
		else 
		{
			// Not allowed to wrap around.
			m_CurrentItem = 0;
			return NULL;
		}
	}

	return curCategory->GetItem(m_CurrentItem);
}

CInventoryCategory* CInventoryCursor::GetNextCategory()
{
	if (m_CategoryLock) 
	{
		// Category lock is switched on, we don't allow to switch to another category.
		return m_Inventory->GetCategory(m_CurrentCategory);
	}

	int cnt = 0;

	CInventoryCategory* rc = NULL;

	int n = m_Inventory->GetNumCategories() - 1;

	if (n < 0) n = 0;

	while (true)
	{
		m_CurrentCategory++;

		// Check if we already passed through all the available categories.
		// This means that either the inventory is quite empty, or there are
		// no categories that are allowed for this cursor.
		cnt++;
		if(cnt > n)
		{
			rc = NULL;
			m_CurrentCategory = 0;
			break;
		}

		if(m_CurrentCategory > n)
		{
			if(m_WrapAround == true)
				m_CurrentCategory = 0;
			else
				m_CurrentCategory = n;
		}

		rc = m_Inventory->GetCategory(m_CurrentCategory);
		if (!IsCategoryIgnored(rc))
			break; // We found a suitable category (not ignored)
	}

	return rc;
}

CInventoryCategory *CInventoryCursor::GetPrevCategory()
{
	CInventoryCategory *rc = NULL;

	// If category lock is switched on, we don't allow to switch 
	// to another category.
	if(m_CategoryLock == true)
		return NULL;

	int n = m_Inventory->GetNumCategories();
	int cnt = 0;

	n--;
	if(n < 0)
		n = 0;

	while(1)
	{
		m_CurrentCategory--;

		// Check if we already passed through all the available categories.
		// This means that either the inventory is quite empty, or there are
		// no categories that are allowed for this cursor.
		cnt++;
		if(cnt > n)
		{
			rc = NULL;
			m_CurrentCategory = 0;
			break;
		}

		if(m_CurrentCategory < 0)
		{
			if(m_WrapAround == true)
				m_CurrentCategory = n;
			else
				m_CurrentCategory = 0;
		}

		rc = m_Inventory->GetCategory(m_CurrentCategory);
		if (!IsCategoryIgnored(rc))
			break; // We found a suitable category (not ignored)
	}

	return rc;
}

void CInventoryCursor::SetCurrentCategory(int index)
{
	// Ensure the index is within bounds
	index = idMath::ClampInt(0, m_Inventory->GetNumCategories() - 1, index);

	m_CurrentCategory = index;
}

void CInventoryCursor::AddCategoryIgnored(const CInventoryCategory* category)
{
	if (category != NULL)
	{
		m_CategoryIgnore.AddUnique( m_Inventory->GetCategoryIndex(category) );
	}
}

void CInventoryCursor::AddCategoryIgnored(const idStr& categoryName)
{
	if (categoryName.IsEmpty()) return;

	// Resolve the name and pass the call to the overload
	AddCategoryIgnored( m_Inventory->GetCategory(categoryName) );
}

void CInventoryCursor::RemoveCategoryIgnored(const CInventoryCategory* category)
{
	int categoryIndex = m_Inventory->GetCategoryIndex(category);
	m_CategoryIgnore.Remove(categoryIndex);
}

void CInventoryCursor::RemoveCategoryIgnored(const idStr& categoryName)
{
	if (categoryName.IsEmpty()) return;

	// Resolve the name and pass the call to the overload
	RemoveCategoryIgnored( m_Inventory->GetCategory(categoryName) );
}

bool CInventoryCursor::IsCategoryIgnored(const CInventoryCategory* category) const
{
	int categoryIndex = m_Inventory->GetCategoryIndex(category);

	return (m_CategoryIgnore.FindIndex(categoryIndex) != -1);
}

CInventoryCategory* CInventoryCursor::GetCurrentCategory()
{
	return (m_Inventory != NULL) ? m_Inventory->GetCategory(m_CurrentCategory) : NULL;
}
