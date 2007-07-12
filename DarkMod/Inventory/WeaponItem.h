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
 * WeaponInventoryItem is an item that belongs to a group.
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

public:
	CInventoryWeaponItem(const idDict& weaponDef, const idStr& weaponDefName, idEntity* owner);

	// Retrieves the maximum amount of ammo this weapon can hold
	int getMaxAmmo();
	// Retrives the amount of ammo at player spawn time
	int getStartAmmo();

	// Returns the currently available ammonition
	int getAmmo() const;
	// Sets the new ammonition value (is automatically clamped to [0,maxAmmo])
	void setAmmo(int newAmount);

};

#endif /* __DARKMOD_INVENTORYWEAPONITEM_H__ */
