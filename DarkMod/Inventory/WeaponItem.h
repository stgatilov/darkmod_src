/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 915 $
 * $Date: 2007-04-19 22:10:27 +0200 (Do, 19 Apr 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef __DARKMOD_INVENTORYWEAPONITEM_H__
#define __DARKMOD_INVENTORYWEAPONITEM_H__

#include "Item.h"

/**
 * WeaponInventoryItem is an item that belongs to a group. This item represents
 * a weapon entityDef and provides some methods to manage the weapon's ammo.
 */
class CInventoryWeaponItem :
	public CInventoryItem
{
protected:
	// The entityDef this weapon is associated to
	const idDict& _weaponDef;

	idStr _weaponDefName;

	// The maximum amount of ammo for this weapon
	int		_maxAmmo;

	// The current amount of ammonition (set to getStartAmmo() in constructor)
	int		_ammo;

	// The index of this weapon (between 0 and MAX_WEAPONS)
	int		_weaponIndex;

	// Is TRUE for weapons that can be toggled (like the lantern)
	bool	_toggleable;

	// TRUE, if this weapon doesn't need ammo (like shortsword, blackjack)
	bool	_allowedEmpty;

public:
	CInventoryWeaponItem(const idDict& weaponDef, const idStr& weaponDefName, idEntity* owner);

	// Retrieves the maximum amount of ammo this weapon can hold
	int getMaxAmmo();
	// Retrives the amount of ammo at player spawn time
	int getStartAmmo();

	// Returns TRUE if this weapon doesn't need ammo and therefore can be selected 
	bool allowedEmpty();

	// Returns the currently available ammonition
	int getAmmo() const;
	// Sets the new ammonition value (is automatically clamped to [0,maxAmmo])
	void setAmmo(int newAmount);

	/**
 	 * This is used to check whether a weapon can "fire". This is always "1" for 
	 * weapons without ammo (sword, blackjack). For all other weapons, the ammo amount
	 * is returned.
	 */
	int hasAmmo();

	/**
	 * "Uses" a certain <amount> of ammo. This decreases the current ammo counter
	 * by the given value. Only affects the ammo count of weapons that actually need ammo.
	 */
	void useAmmo(int amount);

	// Sets/Returns the weapon index (corresponds to the keyboard number keys used to access the weapons)
	void setWeaponIndex(int index);
	int  getWeaponIndex() const;

	/**
	 * greebo: Returns TRUE if this weapon is meant to be toggleable (like the player lantern).
	 */
	bool isToggleable() const;

	/**
	 * greebo: Returns the name of the weapon, as derived from the weaponDef name.
	 *         entityDef "weapon_broadhead" => weapon name: "broadhead"
	 */
	idStr getWeaponName();
};

#endif /* __DARKMOD_INVENTORYWEAPONITEM_H__ */
