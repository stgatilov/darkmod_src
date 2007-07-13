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

#include "WeaponItem.h"

#define WEAPON_MAX_AMMO_PREFIX "max_ammo_"
#define WEAPON_START_AMMO_PREFIX "ammo_"
#define WEAPON_PREFIX "weapon_"
#define WEAPON_ALLOWEMPTY_FORMAT "weapon%d_allowempty"

CInventoryWeaponItem::CInventoryWeaponItem(const idDict& weaponDef, const idStr& weaponDefName, idEntity* owner) :
	CInventoryItem(owner),
	_weaponDef(weaponDef),
	_weaponDefName(weaponDefName),
	_weaponIndex(-1)
{
	_maxAmmo = getMaxAmmo();
	_ammo = getStartAmmo();

	m_Name = weaponDef.GetString("inv_name", "Unknown weapon");
}

int CInventoryWeaponItem::getMaxAmmo() {
	// Sanity check
	if (m_Owner.GetEntity() == NULL) {
		return -1;
	}

	// Construct the weapon name to retrieve the "max_ammo_mossarrow" string, for instance
	idStr weaponName = _weaponDefName;
	weaponName.Strip(WEAPON_PREFIX);

	idStr key = WEAPON_MAX_AMMO_PREFIX + weaponName;
	return m_Owner.GetEntity()->spawnArgs.GetInt(key, "0");
}

bool CInventoryWeaponItem::allowedEmpty() {
	// Sanity check
	if (m_Owner.GetEntity() == NULL) {
		return false;
	}

	// Query the key "weaponNN_allowempty"
	idStr allowKey(va(WEAPON_ALLOWEMPTY_FORMAT, _weaponIndex));
	return m_Owner.GetEntity()->spawnArgs.GetBool(allowKey, "0");
}

int CInventoryWeaponItem::getStartAmmo() {
	// Sanity check
	if (m_Owner.GetEntity() == NULL) {
		return -1;
	}

	// Construct the weapon name to retrieve the "max_ammo_mossarrow" string, for instance
	idStr weaponName = _weaponDefName;
	weaponName.Strip(WEAPON_PREFIX);

	idStr key = WEAPON_START_AMMO_PREFIX + weaponName;
	return m_Owner.GetEntity()->spawnArgs.GetInt(key, "0");
}

int CInventoryWeaponItem::getAmmo() const {
	return _ammo;
}

void CInventoryWeaponItem::setAmmo(int newAmount) {
	if (allowedEmpty()) {
		// Don't set ammo of weapons that don't need any
		return;
	}

	_ammo = (newAmount > _maxAmmo) ? _maxAmmo : newAmount;

	if (_ammo < 0) {
		_ammo = 0;
	}
}

int CInventoryWeaponItem::hasAmmo() {
	if (allowedEmpty()) {
		// Always return 1 for non-ammo weapons
		return 1;
	}
	return _ammo;
}

void CInventoryWeaponItem::useAmmo(int amount) {
	setAmmo(_ammo - amount);
}

void CInventoryWeaponItem::setWeaponIndex(int index) {
	_weaponIndex = index;
}

int CInventoryWeaponItem::getWeaponIndex() const {
	return _weaponIndex;
}
