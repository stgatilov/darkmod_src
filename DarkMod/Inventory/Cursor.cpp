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

CInventoryCursor::CInventoryCursor(CInventory* inventory, int id)
{
	m_Inventory = inventory;
	m_CategoryLock = false;							// Default behaviour ...
	m_WrapAround = true;							// ... is like standard Thief inventory.
	m_CurrentCategory = 0;
	m_CurrentItem = 0;
	m_CursorId = id;
}

CInventoryCursor::~CInventoryCursor()
{
}

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

CInventoryItem *CInventoryCursor::GetCurrentItem()
{
	CInventoryItem *rc = NULL;

	if(m_Inventory->GetNumCategories() > 0)
		rc = m_Inventory->GetCategory(m_CurrentCategory)->GetItem(m_CurrentItem);

	return rc;
}

void CInventoryCursor::ClearItem()
{
	// greebo: Invalidate the item index, this should be enough
	m_CurrentItem = -1;
}

bool CInventoryCursor::SetCurrentItem(CInventoryItem *Item)
{
	bool rc = false;
	int group, item;

	if(Item == NULL)
	{
		if((Item = m_Inventory->GetItem(TDM_DUMMY_ITEM)) == NULL)
			goto Quit;
	}

	if((group = m_Inventory->GetCategoryItemIndex(Item, &item)) == -1)
		goto Quit;

	// Only change the group and item indices, if they are valid.
	// Otherwise we might have an invalid index (-1).
	m_CurrentCategory = group;
	m_CurrentItem = item;

	rc = true;

Quit:
	return rc;
}

bool CInventoryCursor::SetCurrentItem(const idStr& Item)
{
	bool rc = false;
	int group, item;

	if (Item.IsEmpty())
		goto Quit;

	if((group = m_Inventory->GetCategoryItemIndex(Item, &item)) == -1)
		goto Quit;

	// Only change the group and item indizies, if they are valid.
	// Otherwise we might have an invalid index (-1).
	m_CurrentCategory = group;
	m_CurrentItem = item;

	rc = true;

Quit:
	return rc;
}

CInventoryItem *CInventoryCursor::GetNextItem()
{
	CInventoryItem *rc = NULL;

	CInventoryCategory* curCategory = m_Inventory->GetCategory(m_CurrentCategory);
	if (curCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Current Category doesn't exist anymore!\r", m_CurrentCategory);
		return NULL;
	}

	m_CurrentItem++;
	// Have we reached the end of the current category?
	if (m_CurrentItem >= curCategory->size())
	{
		curCategory = GetNextCategory();

		if (m_WrapAround == true) 
		{
			m_CurrentItem = 0;
		}
		else if (curCategory != NULL)
		{
			m_CurrentItem = curCategory->size() - 1;
			goto Quit;
		}
		else
		{
			ClearItem();
			goto Quit;			
		}
	}

	rc = curCategory->GetItem(m_CurrentItem);

Quit:
	return rc;
}

CInventoryItem *CInventoryCursor::GetPrevItem()
{
	CInventoryItem *rc = NULL;

	CInventoryCategory* curCategory = m_Inventory->GetCategory(m_CurrentCategory);
	if (curCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Current Category doesn't exist anymore!\r", m_CurrentCategory);
		return NULL;
	}

	m_CurrentItem--;
	if (m_CurrentItem < 0)
	{
		curCategory = GetPrevCategory();

		if (m_WrapAround == true)
			m_CurrentItem = curCategory->size() - 1;
		else 
		{
			m_CurrentItem = 0;
			goto Quit;
		}
	}

	rc = curCategory->GetItem(m_CurrentItem);

Quit:
	return rc;
}

CInventoryCategory *CInventoryCursor::GetNextCategory()
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

void CInventoryCursor::SetCurrentCategory(int Index)
{
	if(Index < 0)
		Index = 0;
	
	if(Index >= m_Inventory->GetNumCategories())
	{
		Index = m_Inventory->GetNumCategories();
		if(Index != 0)
			Index--;
	}
	m_CurrentCategory = Index;
}

void CInventoryCursor::SetCategoryIgnored(const CInventoryCategory *c)
{
	if(c != NULL)
	{
		m_CategoryIgnore.AddUnique(m_Inventory->GetCategoryIndex(c));
	}
}

void CInventoryCursor::SetCategoryIgnored(const idStr& categoryName)
{
	if (categoryName.IsEmpty())
		return;

	CInventoryCategory *c = m_Inventory->GetCategory(categoryName);
	SetCategoryIgnored(c);
}

void CInventoryCursor::RemoveCategoryIgnored(const CInventoryCategory *c)
{
	int i;

	for(i = 0; i < m_CategoryIgnore.Num(); i++)
	{
		if(m_CategoryIgnore[i] == m_Inventory->GetCategoryIndex(c))
		{
			m_CategoryIgnore.RemoveIndex(i);
			goto Quit;
		}
	}

Quit:
	return;
}

void CInventoryCursor::RemoveCategoryIgnored(const idStr& categoryName)
{
	if (categoryName.IsEmpty())
		return;

	CInventoryCategory *c = m_Inventory->GetCategory(categoryName);
	RemoveCategoryIgnored(c);
}

bool CInventoryCursor::IsCategoryIgnored(const CInventoryCategory *c) const
{
	bool rc = false;
	int i;

	for(i = 0; i < m_CategoryIgnore.Num(); i++)
	{
		if(m_CategoryIgnore[i] == m_Inventory->GetCategoryIndex(c))
		{
			rc = true;
			goto Quit;
		}
	}

Quit:
	return rc;
}

CInventoryCategory* CInventoryCursor::GetCurrentCategory()
{
	return (m_Inventory != NULL) ? m_Inventory->GetCategory(m_CurrentCategory) : NULL;
}
