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

public:
	CInventoryWeaponItem(const idDict& weaponDef, idEntity* owner);
};

#endif /* __DARKMOD_INVENTORYWEAPONITEM_H__ */
