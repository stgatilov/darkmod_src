/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 987 $
 * $Date: 2007-05-12 15:36:09 +0200 (Sa, 12 Mai 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#pragma warning(disable : 4533 4800)

static bool init_version = FileVersionList("$Id: Cursor.cpp 987 2007-05-12 13:36:09Z greebo $", init_version);

#include "Cursor.h"

#include "Inventory.h"

CInventoryCursor::CInventoryCursor(CInventory *inv)
{
	m_Inventory = inv;
	m_CategoryLock = false;							// Default behaviour ...
	m_WrapAround = true;							// ... is like standard Thief inventory.
	m_CurrentCategory = 0;
	m_CurrentItem = 0;
}

CInventoryCursor::~CInventoryCursor()
{
}

void CInventoryCursor::Save(idSaveGame *savefile) const
{
	// TODO: Has to call the groups and items as well.
}

void CInventoryCursor::Restore(idRestoreGame *savefile)
{
	// TODO: Has to call the groups and items as well.
}

CInventoryItem *CInventoryCursor::GetCurrentItem()
{
	CInventoryItem *rc = NULL;

	if(m_Inventory->m_Category.Num() > 0)
		rc = m_Inventory->m_Category[m_CurrentCategory]->GetItem(m_CurrentItem);

	return rc;
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

	// Only change the group and item indizies, if they are valid.
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

CInventoryItem *CInventoryCursor::GetNextItem(void)
{
	CInventoryItem *rc = NULL;
	int ni;

	ni = m_Inventory->m_Category[m_CurrentCategory]->m_Item.Num();

	m_CurrentItem++;
	if(m_CurrentItem >= ni)
	{
		GetNextCategory();

		if(m_WrapAround == true)
			m_CurrentItem = 0;
		else 
		{
			m_CurrentItem = m_Inventory->m_Category[m_CurrentCategory]->m_Item.Num()-1;
			goto Quit;
		}
	}

	rc = m_Inventory->m_Category[m_CurrentCategory]->m_Item[m_CurrentItem];

Quit:
	return rc;
}

CInventoryItem *CInventoryCursor::GetPrevItem(void)
{
	CInventoryItem *rc = NULL;

	m_CurrentItem--;
	if(m_CurrentItem < 0)
	{
		GetPrevCategory();

		if(m_WrapAround == true)
			m_CurrentItem = m_Inventory->m_Category[m_CurrentCategory]->m_Item.Num()-1;
		else 
		{
			m_CurrentItem = 0;
			goto Quit;
		}
	}

	rc = m_Inventory->m_Category[m_CurrentCategory]->m_Item[m_CurrentItem];

Quit:
	return rc;
}

CInventoryCategory *CInventoryCursor::GetNextCategory(void)
{
	CInventoryCategory *rc = NULL;

	// If category lock is switched on, we don't allow to switch 
	// to another category.
	if(m_CategoryLock == true)
		return NULL;

	int n = m_Inventory->m_Category.Num();
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

		rc = m_Inventory->m_Category[m_CurrentCategory];
		if(IsCategoryIgnored(rc) == false)
			break;
	}

	return rc;
}

CInventoryCategory *CInventoryCursor::GetPrevCategory(void)
{
	CInventoryCategory *rc = NULL;

	// If category lock is switched on, we don't allow to switch 
	// to another category.
	if(m_CategoryLock == true)
		return NULL;

	int n = m_Inventory->m_Category.Num();
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

		rc = m_Inventory->m_Category[m_CurrentCategory];
		if(IsCategoryIgnored(rc) == false)
			break;
	}

	return rc;
}

void CInventoryCursor::SetCurrentCategory(int Index)
{
	if(Index < 0)
		Index = 0;
	
	if(Index >= m_Inventory->m_Category.Num())
	{
		Index = m_Inventory->m_Category.Num();
		if(Index != 0)
			Index--;
	}
	m_CurrentCategory = Index;
}

void CInventoryCursor::SetCategoryIgnored(const CInventoryCategory *c)
{
	if(c != NULL)
		m_CategoryIgnore.AddUnique(c);
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
		if(m_CategoryIgnore[i] == c)
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
		if(m_CategoryIgnore[i] == c)
		{
			rc = true;
			goto Quit;
		}
	}

Quit:
	return rc;
}

CInventoryCategory* CInventoryCursor::GetCurrentCategory() {
	return (m_Inventory != NULL) ? m_Inventory->m_Category[m_CurrentCategory] : NULL;
}
