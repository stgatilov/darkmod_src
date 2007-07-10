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
	m_Deletable = false;
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

void CInventoryItem::SetDeletable(bool bDeletable)
{
	if(m_Droppable == true)
		m_Deletable = false;
	else
		m_Deletable = bDeletable;
}
