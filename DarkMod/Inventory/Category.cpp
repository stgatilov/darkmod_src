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

static bool init_version = FileVersionList("$Id: Category.cpp 987 2007-05-12 13:36:09Z greebo $", init_version);

#include "Category.h"
#include "WeaponItem.h"
#include "Inventory.h"

// Constructor
CInventoryCategory::CInventoryCategory(CInventory* inventory, const idStr& name)
{
	m_Owner = (inventory != NULL) ? inventory->GetOwner() : NULL;
	m_Name = name;
}

// Destructor
CInventoryCategory::~CInventoryCategory() 
{
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
		delete m_Item[i];
}

bool CInventoryCategory::isEmpty() const {
	return (m_Item.Num() == 0);
}

void CInventoryCategory::Save(idSaveGame *savefile) const
{
	m_Owner.Save(savefile);
	savefile->WriteString(m_Name.c_str());

	savefile->WriteInt(m_Item.Num());
	for (int i = 0; i < m_Item.Num(); i++)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Item type: %d.\r", static_cast<int>(m_Item[i]->GetType()));
		savefile->WriteInt(static_cast<int>(m_Item[i]->GetType()));
		m_Item[i]->Save(savefile);
	}
}

void CInventoryCategory::Restore(idRestoreGame *savefile)
{
	m_Owner.Restore(savefile);
	savefile->ReadString(m_Name);

	int num;
	savefile->ReadInt(num);
	for (int i = 0; i < num; i++)
	{
		int itemTypeInt;
		savefile->ReadInt(itemTypeInt);
		CInventoryItem::ItemType itemType = static_cast<CInventoryItem::ItemType>(itemTypeInt);

		CInventoryItem* item = NULL;

		switch (itemType)
		{
		case CInventoryItem::IT_WEAPON:
			item = new CInventoryWeaponItem();
			break;
		default:
			item = new CInventoryItem(NULL);
			break;
		};

		if (item != NULL)
		{
			item->Restore(savefile);
			m_Item.Append(item);

			// Set the pointers of the item class directly
			item->m_Category = this;
			item->m_Inventory = m_Inventory;
		}
	}
}

void CInventoryCategory::SetOwner(idEntity *owner)
{
	int i, n;

	m_Owner = owner; 
	n = m_Item.Num();
	for(i = 0; i < n; i++)
		m_Item[i]->m_Owner = owner;
}

void CInventoryCategory::PutItem(CInventoryItem *item)
{
	if(item == NULL)
		goto Quit;

	item->m_Owner = m_Owner;
	item->m_Category = this;
	item->m_Inventory = m_Inventory;

	m_Item.AddUnique(item);

Quit:
	return;
}

CInventoryItem *CInventoryCategory::GetItem(int i)
{
	CInventoryItem *rc = NULL;

	if(i >= 0 && i < m_Item.Num())
		rc = m_Item[i];

	return rc;
}

CInventoryItem *CInventoryCategory::GetItem(const idStr& Name)
{
	CInventoryItem *rc = NULL;
	CInventoryItem *e;
	int i, n;

	if (Name.IsEmpty())
		goto Quit;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		e = m_Item[i];
		if(Name == e->m_Name)
		{
			rc = e;
			goto Quit;
		}
	}

Quit:
	return rc;
}

CInventoryItem *CInventoryCategory::GetItemById(const idStr& id)
{
	CInventoryItem *rc = NULL;
	CInventoryItem *e;
	int i, n;

	if(id.IsEmpty())
		goto Quit;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		e = m_Item[i];
		if(id == e->m_ItemId)
		{
			rc = e;
			goto Quit;
		}
	}

Quit:
	return rc;
}

int CInventoryCategory::GetItemIndex(const idStr &Name)
{
	int rc = -1;
	CInventoryItem *e;
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		e = m_Item[i];
		if(Name == e->m_Name)
		{
			rc = i;
			goto Quit;
		}
	}

Quit:
	return rc;
}

int CInventoryCategory::GetItemIndex(CInventoryItem *it)
{
	int rc = -1;
	int i, n;

	n = m_Item.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Item[i] == it)
		{
			rc = i;
			goto Quit;
		}
	}

Quit:
	return rc;
}

int CInventoryCategory::GetLoot(int &Gold, int &Jewelry, int &Goods)
{
	int i;
	CInventoryItem *it;

	for(i = 0; i < m_Item.Num(); i++)
	{
		it = m_Item[i];

		switch(it->GetLootType())
		{
			case CInventoryItem::LT_JEWELS:
				Jewelry += it->GetValue();
			break;

			case CInventoryItem::LT_GOLD:
				Gold += it->GetValue();
			break;

			case CInventoryItem::LT_GOODS:
				Goods += it->GetValue();
			break;
		}
	}

	return Gold + Jewelry + Goods;
}

void CInventoryCategory::removeItem(CInventoryItem* item) {
	for (int i = 0; i < m_Item.Num(); i++) {
		if (m_Item[i] == item) {
			m_Item.RemoveIndex(i);
			delete item;
			break;
		}
	}
}

int CInventoryCategory::size() const {
	return m_Item.Num();
}
