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

#include "WeaponItem.h"

#define WEAPON_MAX_AMMO_PREFIX "max_ammo_"
#define WEAPON_START_AMMO_PREFIX "ammo_"
#define WEAPON_PREFIX "weapon_"
#define WEAPON_ALLOWEMPTY_FORMAT "weapon%d_allowempty"
#define WEAPON_TOGGLEABLE_FORMAT "weapon%d_toggle"

CInventoryWeaponItem::CInventoryWeaponItem() :
	CInventoryItem(NULL),
	m_WeaponDefName(""),
	m_MaxAmmo(0),
	m_Ammo(0),
	m_WeaponIndex(-1),
	m_Toggleable(false),
	m_AllowedEmpty(false)
{
	SetType(IT_WEAPON);
}

CInventoryWeaponItem::CInventoryWeaponItem(const idStr& weaponDefName, idEntity* owner) :
	CInventoryItem(owner),
	m_WeaponDefName(weaponDefName),
	m_WeaponIndex(-1),
	m_Toggleable(false),
	m_AllowedEmpty(false)
{
	SetType(IT_WEAPON);

	m_MaxAmmo = GetMaxAmmo();
	m_Ammo = GetStartAmmo();

	const idDict* weaponDict = gameLocal.FindEntityDefDict(weaponDefName);
	m_Name = weaponDict->GetString("inv_name", "Unknown weapon");

	// Parse the common spawnargs which apply to both this and the base class
	ParseSpawnargs(*weaponDict);
}

void CInventoryWeaponItem::Save( idSaveGame *savefile ) const
{
	// Pass the call to the base class first
	CInventoryItem::Save(savefile);

	savefile->WriteString(m_WeaponDefName);
	savefile->WriteInt(m_MaxAmmo);
	savefile->WriteInt(m_Ammo);
	savefile->WriteInt(m_WeaponIndex);
	savefile->WriteBool(m_Toggleable);
	savefile->WriteBool(m_AllowedEmpty);
}

void CInventoryWeaponItem::Restore( idRestoreGame *savefile )
{
	// Pass the call to the base class first
	CInventoryItem::Restore(savefile);

	savefile->ReadString(m_WeaponDefName);
	savefile->ReadInt(m_MaxAmmo);
	savefile->ReadInt(m_Ammo);
	savefile->ReadInt(m_WeaponIndex);
	savefile->ReadBool(m_Toggleable);
	savefile->ReadBool(m_AllowedEmpty);
}

int CInventoryWeaponItem::GetMaxAmmo()
{
	// Sanity check
	if (m_Owner.GetEntity() == NULL) {
		return -1;
	}

	// Construct the weapon name to retrieve the "max_ammo_mossarrow" string, for instance
	idStr weaponName = m_WeaponDefName;
	weaponName.Strip(WEAPON_PREFIX);

	idStr key = WEAPON_MAX_AMMO_PREFIX + weaponName;
	return m_Owner.GetEntity()->spawnArgs.GetInt(key, "0");
}

bool CInventoryWeaponItem::IsAllowedEmpty()
{
	return m_AllowedEmpty;
}

int CInventoryWeaponItem::GetStartAmmo()
{
	// Sanity check
	if (m_Owner.GetEntity() == NULL) {
		return -1;
	}

	// Construct the weapon name to retrieve the "max_ammo_mossarrow" string, for instance
	idStr weaponName = m_WeaponDefName;
	weaponName.Strip(WEAPON_PREFIX);

	idStr key = WEAPON_START_AMMO_PREFIX + weaponName;
	return m_Owner.GetEntity()->spawnArgs.GetInt(key, "0");
}

int CInventoryWeaponItem::GetAmmo() const
{
	return m_Ammo;
}

void CInventoryWeaponItem::SetAmmo(int newAmount)
{
	if (IsAllowedEmpty()) {
		// Don't set ammo of weapons that don't need any
		return;
	}

	m_Ammo = (newAmount > m_MaxAmmo) ? m_MaxAmmo : newAmount;

	if (m_Ammo < 0) {
		m_Ammo = 0;
	}
}

int CInventoryWeaponItem::HasAmmo()
{
	if (IsAllowedEmpty()) {
		// Always return 1 for non-ammo weapons
		return 1;
	}
	return m_Ammo;
}

void CInventoryWeaponItem::UseAmmo(int amount)
{
	SetAmmo(m_Ammo - amount);
}

void CInventoryWeaponItem::SetWeaponIndex(int index)
{
	m_WeaponIndex = index;

	// Now that the weapon index is known, cache a few values from the owner spawnargs

	// Sanity check
	if (m_Owner.GetEntity() != NULL) {
		idStr toggleKey(va(WEAPON_TOGGLEABLE_FORMAT, m_WeaponIndex));
		m_Toggleable = m_Owner.GetEntity()->spawnArgs.GetBool(toggleKey, "0");

		idStr allowKey(va(WEAPON_ALLOWEMPTY_FORMAT, m_WeaponIndex));
		m_AllowedEmpty = m_Owner.GetEntity()->spawnArgs.GetBool(allowKey, "0");
	}
}

int CInventoryWeaponItem::GetWeaponIndex() const
{
	return m_WeaponIndex;
}

bool CInventoryWeaponItem::IsToggleable() const
{
	return m_Toggleable;
}

idStr CInventoryWeaponItem::GetWeaponName()
{
	idStr weaponName = m_WeaponDefName;
	weaponName.Strip(WEAPON_PREFIX);
	return weaponName;
}
