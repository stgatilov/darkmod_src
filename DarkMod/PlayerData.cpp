/******************************************************************************/
/*                                                                            */
/*         DarkModGlobals (C) by Gerhard W. Gruber in Germany 2004            */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.4  2005/01/07 02:01:10  sparhawk
 * Lightgem updates
 *
 * Revision 1.3  2004/11/24 21:59:06  sparhawk
 * *) Multifrob implemented
 * *) Usage of items against other items implemented.
 * *) Basic Inventory system added.
 *
 * Revision 1.2  2004/11/03 21:47:17  sparhawk
 * Changed debug LogString for better performance and group settings
 *
 * Revision 1.1  2004/10/31 20:03:36  sparhawk
 * CDarkMod created to contain relevant player data seperate from id code.
 *
 *
 * DESCRIPTION: This file data which is relevant to the player. Examples
 * are: pointer to the inventory, currently highlighted entity adn others.
 *
 *****************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "../darkmod/darkmodglobals.h"
#include "../darkmod/playerdata.h"

// TODO: Items which can be put in the inventory should get a counter parameter.
// If they have it the item is not removed from gameworld until the counter reached
// zero.

CInventoryItem::CInventoryItem(void)
{
	m_Entity = NULL;
	m_Value = 0;
	m_Count = 0;
}

CDarkModPlayer::CDarkModPlayer(void)
{
	m_FrobEntity = NULL;
	CInventoryItem inv_item;

	// The first entry in the inventory is always empty and selected by default.
	m_Selection = 0;
	m_Inventory.Append(inv_item);
	m_LightgemValue = 0;
}

CDarkModPlayer::~CDarkModPlayer(void)
{
}

void CDarkModPlayer::AddEntity(idEntity *ent)
{
	int i, n;
	bool bFound = false;
	CInventoryItem new_item;

	DM_LOG(LC_INVENTORY, LT_DEBUG).LogString("this: %08lX [%s]\r", this, __FUNCTION__);
	n = m_Inventory.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Inventory[i].m_Entity == ent)
		{
			bFound = true;
			break;
		}
	}

	// Only add the item if we don't have it already and make it the
	// current selected one
	if(bFound == false)
	{
		new_item.m_Entity = ent;
		m_Inventory.Append(new_item);
		m_Selection = m_Inventory.Num()-1;
		DM_LOG(LC_INVENTORY, LT_DEBUG).LogString("[%s] added to inventory (%u)\r", ent->name.c_str(), m_Inventory.Num());
		ent->Hide();
	}
}

void CDarkModPlayer::SelectNext(void)
{
	// TODO: Inventory keys are abused for lightgem
	idPlayer *p = gameLocal.GetLocalPlayer();
	if(m_LightgemValue < LIGHTGEM_MAX)
		m_LightgemValue++;
	p->hud->SetStateInt("lightgem_val", m_LightgemValue);

	if(m_Selection < m_Inventory.Num()-1)
		m_Selection++;
	else
		m_Selection = 0;
}

void CDarkModPlayer::SelectPrev(void)
{
	// TODO: Inventory keys are abused for lightgem
	idPlayer *p = gameLocal.GetLocalPlayer();
	if(m_LightgemValue > LIGHTGEM_MIN)
		m_LightgemValue--;
	p->hud->SetStateInt("lightgem_val", m_LightgemValue);

	if(m_Selection > 0)
		m_Selection--;
	else
		m_Selection = m_Inventory.Num()-1;
}

long CDarkModPlayer::GetEntity(idEntity *ent)
{
	int i, n;

	n = m_Inventory.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Inventory[i].m_Entity == ent)
			return i;
	}

	return -1;
}

idEntity *CDarkModPlayer::GetEntity(long i)
{
	DM_LOG(LC_INVENTORY, LT_DEBUG).LogString("%u requested from %u Inventory items\r", i, m_Inventory.Num());
	if(i <= m_Inventory.Num())
		return m_Inventory[i].m_Entity;
	else
		return NULL;
}

unsigned long CDarkModPlayer::AddLight(idLight *light)
{
	if(light)
	{
		m_LightList.Append(light);
		DM_LOG(LC_FUNCTION, LT_DEBUG).LogString("%08lX [%s] %lu added to LightList\r", light, light->name.c_str(), m_LightList.Num());
	}

	return m_LightList.Num();
}

unsigned long CDarkModPlayer::RemoveLight(idLight *light)
{
	int n;

	if(light)
	{
		if((n = m_LightList.FindIndex(light)) != -1)
		{
			m_LightList.RemoveIndex(n);
			DM_LOG(LC_FUNCTION, LT_DEBUG).LogString("%08lX [%s] %lu removed from LightList\r", light, light->name.c_str(), m_LightList.Num());
		}
	}

	return m_LightList.Num();
}

