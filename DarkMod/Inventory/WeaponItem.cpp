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
#define WEAPON_TOGGLEABLE_FORMAT "weapon%d_toggle"

CInventoryWeaponItem::CInventoryWeaponItem() :
	CInventoryItem(NULL),
	_weaponDefName(""),
	_maxAmmo(0),
	_ammo(0),
	_weaponIndex(-1),
	_toggleable(false),
	_allowedEmpty(false)
{
	SetType(IT_WEAPON);
}

CInventoryWeaponItem::CInventoryWeaponItem(const idStr& weaponDefName, idEntity* owner) :
	CInventoryItem(owner),
	_weaponDefName(weaponDefName),
	_weaponIndex(-1),
	_toggleable(false),
	_allowedEmpty(false)
{
	SetType(IT_WEAPON);

	_maxAmmo = getMaxAmmo();
	_ammo = getStartAmmo();

	const idDict* weaponDict = gameLocal.FindEntityDefDict(weaponDefName.c_str());
	m_Name = weaponDict->GetString("inv_name", "Unknown weapon");
	m_Persistent = weaponDict->GetBool("inv_persistent", "0");
	m_LightgemModifier = weaponDict->GetInt("inv_lgmodifier", "0");
}

void CInventoryWeaponItem::Save( idSaveGame *savefile ) const
{
	// Pass the call to the base class first
	CInventoryItem::Save(savefile);

	savefile->WriteString(_weaponDefName.c_str());
	savefile->WriteInt(_maxAmmo);
	savefile->WriteInt(_ammo);
	savefile->WriteInt(_weaponIndex);
	savefile->WriteBool(_toggleable);
	savefile->WriteBool(_allowedEmpty);
}

void CInventoryWeaponItem::Restore( idRestoreGame *savefile )
{
	// Pass the call to the base class first
	CInventoryItem::Restore(savefile);

	savefile->ReadString(_weaponDefName);
	savefile->ReadInt(_maxAmmo);
	savefile->ReadInt(_ammo);
	savefile->ReadInt(_weaponIndex);
	savefile->ReadBool(_toggleable);
	savefile->ReadBool(_allowedEmpty);
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
	return _allowedEmpty;
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

	// Now that the weapon index is known, cache a few values from the owner spawnargs

	// Sanity check
	if (m_Owner.GetEntity() != NULL) {
		idStr toggleKey(va(WEAPON_TOGGLEABLE_FORMAT, _weaponIndex));
		_toggleable = m_Owner.GetEntity()->spawnArgs.GetBool(toggleKey, "0");

		idStr allowKey(va(WEAPON_ALLOWEMPTY_FORMAT, _weaponIndex));
		_allowedEmpty = m_Owner.GetEntity()->spawnArgs.GetBool(allowKey, "0");
	}
}

int CInventoryWeaponItem::getWeaponIndex() const {
	return _weaponIndex;
}

bool CInventoryWeaponItem::isToggleable() const {
	return _toggleable;
}

idStr CInventoryWeaponItem::getWeaponName() {
	idStr weaponName = _weaponDefName;
	weaponName.Strip(WEAPON_PREFIX);
	return weaponName;
}
