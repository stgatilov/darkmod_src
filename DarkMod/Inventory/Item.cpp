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

#include "Item.h"
#include <algorithm>

CInventoryItem::CInventoryItem(idEntity *owner)
{
	m_Owner = owner;
	m_Item = NULL;
	m_Category = NULL;
	m_Type = IT_ITEM;
	m_LootType = LT_NONE;
	m_Value = 0;
	m_Stackable = false;
	m_Count = 1;
	m_Droppable = false;
	m_Overlay = OVERLAYS_INVALID_HANDLE;
	m_Hud = false;
	m_Orientated = false;
	m_Persistent = false;
	m_LightgemModifier = 0;
	m_MovementModifier = 1.0f;
	m_UseOnFrob = false;
	m_DropOrientation = mat3_identity;
}

CInventoryItem::CInventoryItem(idEntity* itemEntity, idEntity* owner) {
	// Don't allow NULL pointers
	assert(owner && itemEntity);

	// Parse a few common spawnargs
	ParseSpawnargs(itemEntity->spawnArgs);

	m_Category = NULL;
	m_Overlay = OVERLAYS_INVALID_HANDLE;
	m_Hud = false;

	m_Owner = owner;
	m_Item = itemEntity;
	
	// Determine and set the loot type
	m_LootType = GetLootTypeFromSpawnargs(itemEntity->spawnArgs);

	// Read the spawnargs into the member variables
	m_Name = itemEntity->spawnArgs.GetString("inv_name", "");
	m_Value	= itemEntity->spawnArgs.GetInt("inv_loot_value", "-1");
	m_Stackable	= itemEntity->spawnArgs.GetBool("inv_stackable", "0");
	m_UseOnFrob = itemEntity->spawnArgs.GetBool("inv_use_on_frob", "0");

	m_Count = (m_Stackable) ? itemEntity->spawnArgs.GetInt("inv_count", "1") : 1;

	m_Droppable = itemEntity->spawnArgs.GetBool("inv_droppable", "0");
	m_ItemId = itemEntity->spawnArgs.GetString("inv_item_id", "");

	if (m_Icon.IsEmpty() && m_LootType == LT_NONE)
	{
		DM_LOG(LC_INVENTORY, LT_INFO)LOGSTRING("Information: non-loot item %s has no icon.\r", itemEntity->name.c_str());
	}

	if (m_LootType != LT_NONE && m_Value <= 0)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Warning: Value for loot item missing on entity %s\r", itemEntity->name.c_str());
	}

	// Set the item type according to the loot property
	m_Type = (m_LootType != LT_NONE) ? IT_LOOT : IT_ITEM;

	m_BindMaster = itemEntity->GetBindMaster();
	m_Orientated = itemEntity->fl.bindOrientated;

	idStr hudName;
	// Item could be added to the inventory, check for custom HUD
	if (itemEntity->spawnArgs.GetString("inv_hud", "", hudName) != false)
	{
		int hudLayer;
		itemEntity->spawnArgs.GetInt("inv_hud_layer", "0", hudLayer);
		SetHUD(hudName, hudLayer);
	}

	// Check for a preferred drop orientation, if not, use current orientation
	if( itemEntity->spawnArgs.FindKey("drop_angles") )
	{
		idAngles DropAngles;
		DropAngles = itemEntity->spawnArgs.GetAngles("drop_angles");
		m_DropOrientation = DropAngles.ToMat3();
	}
	else
	{
		idVec3 dummy;
		idMat3 playerView;
		gameLocal.GetLocalPlayer()->GetViewPos(dummy, playerView);
		// drop orientation is relative to the player view yaw only
		idAngles viewYaw = playerView.ToAngles();
		// ignore pitch and roll
		viewYaw[0] = 0;
		viewYaw[2] = 0;
		idMat3 playerViewYaw = viewYaw.ToMat3();

		m_DropOrientation = itemEntity->GetPhysics()->GetAxis() * playerViewYaw.Transpose();
	}
}

void CInventoryItem::Save( idSaveGame *savefile ) const
{
	m_Owner.Save(savefile);
	m_Item.Save(savefile);
	m_BindMaster.Save(savefile);

	savefile->WriteString(m_Name);
	savefile->WriteString(m_HudName);
	savefile->WriteString(m_ItemId);

	savefile->WriteInt(static_cast<int>(m_Type));
	savefile->WriteInt(static_cast<int>(m_LootType));

	savefile->WriteInt(m_Value);
	savefile->WriteInt(m_Overlay);
	savefile->WriteInt(m_Count);

	savefile->WriteBool(m_Stackable);
	savefile->WriteBool(m_Droppable);
	savefile->WriteBool(m_Hud);

	savefile->WriteString(m_Icon);

	savefile->WriteBool(m_Orientated);
	savefile->WriteBool(m_Persistent);
	
	savefile->WriteInt(m_LightgemModifier);
	savefile->WriteFloat(m_MovementModifier);
	savefile->WriteBool(m_UseOnFrob);
	savefile->WriteMat3(m_DropOrientation);
}

void CInventoryItem::Restore( idRestoreGame *savefile )
{
	int tempInt;

	m_Owner.Restore(savefile);
	m_Item.Restore(savefile);
	m_BindMaster.Restore(savefile);

	savefile->ReadString(m_Name);
	savefile->ReadString(m_HudName);
	savefile->ReadString(m_ItemId);

	savefile->ReadInt(tempInt);
	m_Type = static_cast<ItemType>(tempInt);

	savefile->ReadInt(tempInt);
	m_LootType = static_cast<LootType>(tempInt);

	savefile->ReadInt(m_Value);
	savefile->ReadInt(m_Overlay);
	savefile->ReadInt(m_Count);

	savefile->ReadBool(m_Stackable);
	savefile->ReadBool(m_Droppable);
	savefile->ReadBool(m_Hud);

	savefile->ReadString(m_Icon);

	savefile->ReadBool(m_Orientated);
	savefile->ReadBool(m_Persistent);

	savefile->ReadInt(m_LightgemModifier);
	savefile->ReadFloat(m_MovementModifier);
	savefile->ReadBool(m_UseOnFrob);
	savefile->ReadMat3(m_DropOrientation);
}

void CInventoryItem::ParseSpawnargs(const idDict& spawnArgs)
{
	m_Persistent = spawnArgs.GetBool("inv_persistent", "0");
	m_LightgemModifier = spawnArgs.GetInt("inv_lgmodifier", "0");
	m_MovementModifier = spawnArgs.GetFloat("inv_movement_modifier", "1");
	m_Icon = spawnArgs.GetString("inv_icon", "");
}

void CInventoryItem::SetLootType(CInventoryItem::LootType t)
{
	// Only positive values are allowed
	if (t >= CInventoryItem::LT_NONE && t <= CInventoryItem::LT_COUNT)
	{
		m_LootType = t;
	}
	else
	{
		m_LootType = CInventoryItem::LT_NONE;
	}

	NotifyItemChanged();
}

void CInventoryItem::SetValue(int n)
{
	// Only positive values are allowed
	if (n >= 0)
	{
		m_Value = n;

		NotifyItemChanged();
	}
}

void CInventoryItem::SetCount(int n)
{
	// Only positive values are allowed
	m_Count = (n >= 0) ? n : 0;

	NotifyItemChanged();
}

void CInventoryItem::SetStackable(bool stack)
{
	m_Stackable = stack;
}

void CInventoryItem::SetHUD(const idStr &HudName, int layer)
{
	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Setting hud %s on layer %d\r", HudName.c_str(), layer); 
	if (m_Overlay == OVERLAYS_INVALID_HANDLE || m_HudName != HudName)
	{
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = owner->CreateOverlay(HudName, layer);
		idEntity* it = m_Item.GetEntity();
		if (it != NULL)
		{
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
		}
	}

	NotifyItemChanged();
}

void CInventoryItem::SetOverlay(const idStr &HudName, int overlay)
{
	if (overlay != OVERLAYS_INVALID_HANDLE)
	{
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = overlay;
		idEntity* it = m_Item.GetEntity();
		if (it != NULL)
		{
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
		}
	}
	else
	{
		m_Hud = false;
	}
}

CInventoryItem::LootType CInventoryItem::GetLootTypeFromSpawnargs(const idDict& spawnargs) {
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

int CInventoryItem::GetPersistentCount()
{
	if (m_Persistent)
	{
		return m_Count;
	}
	else 
	{
		return 0;
	}
}

void CInventoryItem::SetPersistent(bool newValue)
{
	m_Persistent = newValue;
}

void CInventoryItem::SetLightgemModifier(int newValue)
{
	// greebo: Clamp the value to [0..1]
	m_LightgemModifier = idMath::ClampInt(0, DARKMOD_LG_MAX, newValue);
}

void CInventoryItem::SetMovementModifier(float newValue)
{
	if (newValue > 0)
	{
		m_MovementModifier = newValue;
	}
}

void CInventoryItem::SetDropOrientation(const idMat3& newAxis)
{
	m_DropOrientation = newAxis;
}

void CInventoryItem::SetIcon(const idStr& newIcon)
{
	m_Icon = newIcon;

	NotifyItemChanged();
}

void CInventoryItem::NotifyItemChanged()
{
	if (m_Owner.GetEntity() == NULL) return;

	m_Owner.GetEntity()->OnInventoryItemChanged();
}
