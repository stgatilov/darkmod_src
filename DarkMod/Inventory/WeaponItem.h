/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
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
	idStr	m_WeaponDefName;

	// The maximum amount of ammo for this weapon
	int		m_MaxAmmo;

	// The current amount of ammonition (set to getStartAmmo() in constructor)
	int		m_Ammo;

	// The index of this weapon (between 0 and MAX_WEAPONS)
	int		m_WeaponIndex;

	// Is TRUE for weapons that can be toggled (like the lantern)
	bool	m_Toggleable;

	// TRUE, if this weapon doesn't need ammo (like shortsword, blackjack)
	bool	m_AllowedEmpty;

public:
	// Default constructor, should only be used during restoring from savegames
	CInventoryWeaponItem();

	CInventoryWeaponItem(const idStr& weaponDefName, idEntity* owner);

	virtual void	Save( idSaveGame *savefile ) const;
	virtual void	Restore(idRestoreGame *savefile);

	// Retrieves the maximum amount of ammo this weapon can hold
	int GetMaxAmmo();
	// Retrives the amount of ammo at player spawn time
	int GetStartAmmo();

	// Returns TRUE if this weapon doesn't need ammo and therefore can be selected 
	bool IsAllowedEmpty();

	// Returns the currently available ammonition
	int GetAmmo() const;
	// Sets the new ammonition value (is automatically clamped to [0,maxAmmo])
	void SetAmmo(int newAmount);

	/**
 	 * This is used to check whether a weapon can "fire". This is always "1" for 
	 * weapons without ammo (sword, blackjack). For all other weapons, the ammo amount
	 * is returned.
	 */
	int HasAmmo();

	/**
	 * "Uses" a certain <amount> of ammo. This decreases the current ammo counter
	 * by the given value. Only affects the ammo count of weapons that actually need ammo.
	 */
	void UseAmmo(int amount);

	// Sets/Returns the weapon index (corresponds to the keyboard number keys used to access the weapons)
	void SetWeaponIndex(int index);
	int  GetWeaponIndex() const;

	/**
	 * greebo: Returns TRUE if this weapon is meant to be toggleable (like the player lantern).
	 */
	bool IsToggleable() const;

	/**
	 * greebo: Returns the name of the weapon, as derived from the weaponDef name.
	 *         entityDef "weapon_broadhead" => weapon name: "broadhead"
	 */
	idStr GetWeaponName();
};
typedef boost::shared_ptr<CInventoryWeaponItem> CInventoryWeaponItemPtr;

#endif /* __DARKMOD_INVENTORYWEAPONITEM_H__ */
