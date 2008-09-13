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

#include "Category.h"
#include "WeaponItem.h"
#include "Inventory.h"

// Constructor
CInventoryCategory::CInventoryCategory(CInventory* inventory, const idStr& name) :
	m_Name(name)
{
	m_Owner = (inventory != NULL) ? inventory->GetOwner() : NULL;
}

// Destructor
CInventoryCategory::~CInventoryCategory() 
{
	m_Item.Clear();
}

bool CInventoryCategory::IsEmpty() const
{
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

		CInventoryItemPtr item;

		switch (itemType)
		{
		case CInventoryItem::IT_WEAPON:
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Restoring item #%d (weapon item).\r", i);
			item = CInventoryItemPtr(new CInventoryWeaponItem());
			break;
		default:
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Restoring item #%d (ordinary item).\r", i);
			item = CInventoryItemPtr(new CInventoryItem(NULL));
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
	m_Owner = owner; 

	for (int i = 0; i < m_Item.Num(); i++)
	{
		m_Item[i]->m_Owner = owner;
	}
}

void CInventoryCategory::PutItem(CInventoryItemPtr item)
{
	if (item == NULL) return;

	item->m_Owner = m_Owner;
	item->m_Category = this;
	item->m_Inventory = m_Inventory;

	m_Item.AddUnique(item);
}

CInventoryItemPtr CInventoryCategory::GetItem(int index)
{
	return (index >= 0 && index < m_Item.Num()) ? m_Item[index] : CInventoryItemPtr();
}

CInventoryItemPtr CInventoryCategory::GetItem(const idStr& itemName)
{
	if (itemName.IsEmpty()) return CInventoryItemPtr();

	for (int i = 0; i < m_Item.Num(); i++)
	{
		const CInventoryItemPtr& item = m_Item[i];

		if (itemName == item->m_Name)
		{
			return item;
		}
	}

	return CInventoryItemPtr();
}

CInventoryItemPtr CInventoryCategory::GetItemById(const idStr& id)
{
	if (id.IsEmpty()) return CInventoryItemPtr();

	for (int i = 0; i < m_Item.Num(); i++)
	{
		const CInventoryItemPtr& item = m_Item[i];

		if (id == item->m_ItemId)
		{
			return item;
		}
	}

	return CInventoryItemPtr();
}

int CInventoryCategory::GetItemIndex(const idStr& itemName)
{
	for (int i = 0; i < m_Item.Num(); i++)
	{
		if (itemName == m_Item[i]->m_Name)
		{
			return i;
		}
	}

	return -1;
}

int CInventoryCategory::GetItemIndex(const CInventoryItemPtr& item)
{
	return m_Item.FindIndex(item);
}

int CInventoryCategory::GetLoot(int& gold, int& jewelry, int& goods)
{
	for (int i = 0; i < m_Item.Num(); i++)
	{
		const CInventoryItemPtr& item = m_Item[i];

		switch (item->GetLootType())
		{
			case CInventoryItem::LT_JEWELS:
				jewelry += item->GetValue();
			break;

			case CInventoryItem::LT_GOLD:
				gold += item->GetValue();
			break;

			case CInventoryItem::LT_GOODS:
				goods += item->GetValue();
			break;
			
			default: break;
		}
	}

	return gold + jewelry + goods;
}

void CInventoryCategory::RemoveItem(const CInventoryItemPtr& item)
{
	m_Item.Remove(item);
}

int CInventoryCategory::GetNumItems() const
{
	return m_Item.Num();
}
