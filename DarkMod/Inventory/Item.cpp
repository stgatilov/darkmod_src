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

static bool init_version = FileVersionList("$Id: Item.cpp 987 2007-05-12 13:36:09Z greebo $", init_version);

#include "Item.h"

CInventoryItem::CInventoryItem(idEntity *owner)
{
	m_Owner = owner;
	m_Item = NULL;
	m_Inventory = NULL;
	m_Category = NULL;
	m_Type = IT_ITEM;
	m_LootType = LT_NONE;
	m_Value = 0;
	m_Stackable = false;
	m_Count = 0;
	m_Droppable = false;
	m_Overlay = OVERLAYS_INVALID_HANDLE;
	m_Hud = false;
	m_Orientated = false;
}

CInventoryItem::CInventoryItem(idEntity* itemEntity, idEntity* owner) {
	// Don't allow NULL pointers
	assert(owner && itemEntity);

	m_Inventory = NULL;
	m_Category = NULL;
	m_Overlay = OVERLAYS_INVALID_HANDLE;
	m_Hud = false;

	m_Owner = owner;
	m_Item = itemEntity;
	
	// Determine and set the loot type
	m_LootType = getLootTypeFromSpawnargs(itemEntity->spawnArgs);

	// Read the spawnargs into the member variables
	m_Name = itemEntity->spawnArgs.GetString("inv_name", "");
	m_Value	= itemEntity->spawnArgs.GetInt("inv_loot_value", "-1");
	m_Stackable	= itemEntity->spawnArgs.GetBool("inv_stackable", "0");

	// Only stackable items have a non-zero count
	m_Count = (m_Stackable) ? itemEntity->spawnArgs.GetInt("inv_count", "1") : 0;

	m_Droppable = itemEntity->spawnArgs.GetBool("inv_droppable", "0");
	m_ItemId = itemEntity->spawnArgs.GetString("inv_item_id", "");

	m_Icon = itemEntity->spawnArgs.GetString("inv_icon", "");
	if (m_Icon.IsEmpty() && m_LootType == LT_NONE) {
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Warning: non-loot item %s has no icon.\r", itemEntity->name.c_str());
	}

	if (m_LootType != LT_NONE && m_Value <= 0) {
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Warning: Value for loot item missing on entity %s\r", itemEntity->name.c_str());
	}

	// Set the item type according to the loot property
	m_Type = (m_LootType != LT_NONE) ? IT_LOOT : IT_ITEM;

	m_BindMaster = itemEntity->GetBindMaster();
	m_Orientated = itemEntity->fl.bindOrientated;
}

CInventoryItem::~CInventoryItem()
{
	idEntity *e = m_Item.GetEntity();

	if(e != NULL)
		e->SetInventoryItem(NULL);
}

void CInventoryItem::Save( idSaveGame *savefile ) const
{
	// TODO
}

void CInventoryItem::Restore( idRestoreGame *savefile )
{
	// TODO
}

void CInventoryItem::SetLootType(CInventoryItem::LootType t)
{
	// Only positive values are allowed
	if(t >= CInventoryItem::LT_NONE && t <= CInventoryItem::LT_COUNT)
		m_LootType = t;
	else
		m_LootType = CInventoryItem::LT_NONE;
}

void CInventoryItem::SetValue(int n)
{
	// Only positive values are allowed
	if(n >= 0)
		m_Value = n;
}

void CInventoryItem::SetCount(int n)
{
	// Only positive values are allowed if stackable is true
	if(n >= 0 && m_Stackable == true)
		m_Count = n;
	else
		m_Count = 0;
}

void CInventoryItem::SetStackable(bool stack)
{
	m_Stackable = stack;
}

void CInventoryItem::SetHUD(const idStr &HudName, int layer)
{
	if(m_Overlay == OVERLAYS_INVALID_HANDLE || m_HudName != HudName)
	{
		idEntity *it;
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = owner->CreateOverlay(HudName, layer);
		if((it = m_Item.GetEntity()) != NULL)
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
	}
}

void CInventoryItem::SetOverlay(const idStr &HudName, int Overlay)
{
	if(Overlay != OVERLAYS_INVALID_HANDLE)
	{
		idEntity *it;
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = Overlay;
		if((it = m_Item.GetEntity()) != NULL)
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
	}
	else
		m_Hud = false;
}

CInventoryItem::LootType CInventoryItem::getLootTypeFromSpawnargs(const idDict& spawnargs) {
	// Determine the loot type
	int lootTypeInt;
	LootType returnValue = CInventoryItem::LT_NONE;

	if (spawnargs.GetInt("inv_loot_type", "", lootTypeInt) != false) 	{
		if (lootTypeInt >= LT_NONE && lootTypeInt < LT_COUNT)
			returnValue = static_cast<LootType>(lootTypeInt);
		else
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid loot type: %d\r", lootTypeInt);
	}

	return returnValue;
}
