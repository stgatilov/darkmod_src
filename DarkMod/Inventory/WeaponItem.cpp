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

CInventoryWeaponItem::CInventoryWeaponItem(const idDict& weaponDef, const idStr& weaponDefName, idEntity* owner) :
	CInventoryItem(owner),
	_weaponDef(weaponDef),
	_weaponDefName(weaponDefName)
{
	_maxAmmo = getMaxAmmo();
	_ammo = _maxAmmo;
}

int CInventoryWeaponItem::getMaxAmmo() const {
	// Sanity check
	if (m_Owner.GetEntity() == NULL) {
		return -1;
	}

	// Construct the weapon name to retrieve the "max_ammo_mossarrow" string, for instance
	idStr weaponName = _weaponDefName;
	weaponName.Strip("weapon_");

	idStr key = WEAPON_MAX_AMMO_PREFIX + weaponName;
	return m_Owner.GetEntity()->spawnArgs.GetInt(key, "0");
}

int CInventoryWeaponItem::getAmmo() const {
	return _ammo;
}

void CInventoryWeaponItem::setAmmo(int newAmount) {
	_ammo = (newAmount > _maxAmmo) ? _maxAmmo : newAmount;

	if (_ammo < 0) {
		_ammo = 0;
	}
}
