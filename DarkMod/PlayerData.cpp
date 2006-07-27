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
 * Revision 1.11  2006/07/27 09:01:07  ishtvan
 * added m_FrobEntityPrevious var to store the frob entity of the previous frame
 *
 * Revision 1.10  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.9  2005/12/12 05:21:42  ishtvan
 * inventory fix - item clipmodel removed when it's picked up so it doesn't get in the way of picking up other items
 *
 * Revision 1.8  2005/12/12 02:57:33  ishtvan
 * ammo items that are frobbed go into the D3 inventory
 *
 * added inventory clearing function
 *
 * Revision 1.7  2005/11/19 17:26:48  sparhawk
 * LogString with macro replaced
 *
 * Revision 1.6  2005/10/18 13:56:09  sparhawk
 * Lightgem updates
 *
 * Revision 1.5  2005/09/24 03:13:49  lloyd
 * Changed CGrabber grabber to CGrabber *grabber
 *
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

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

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
	m_FrobEntityPrevious = NULL;
	CInventoryItem inv_item;

	// The first entry in the inventory is always empty and selected by default.
	m_Selection = 0;
	m_Inventory.Append(inv_item);
	m_LightgemValue = 0;

	// TODO: Spawn grabber from a .def file (maybe?)
	this->grabber = new CGrabber();
}

CDarkModPlayer::~CDarkModPlayer(void)
{
	// remove grabber object	
	this->grabber->PostEventSec( &EV_Remove, 0 );
}

void CDarkModPlayer::AddEntity(idEntity *ent)
{
	int i, n;
	bool bFound = false;
	CInventoryItem new_item;

	// Ammo items get added to weapon ammo slots
	// These are handled by D3's old inventory, so we need to call this:
	if( ent->IsType(idItem::Type) && ent->spawnArgs.MatchPrefix("inv_ammo_", NULL) )
	{
		gameLocal.GetLocalPlayer()->GiveItem( static_cast<idItem *>(ent) );
		
		ent->Unbind();
		ent->GetPhysics()->PutToRest();
		ent->GetPhysics()->UnlinkClip();
		ent->Hide();

		// for now, keep it for 5 seconds giving the acquire sound some time to play
		ent->PostEventMS( &EV_Remove, 5000 );

		goto Quit;
	}

	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
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
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("[%s] added to inventory (%u)\r", ent->name.c_str(), m_Inventory.Num());
		
		ent->Unbind();
		ent->GetPhysics()->PutToRest();
// TODO: don't forget to re-link the clipmodel if we drop the item later
		ent->GetPhysics()->UnlinkClip();
		ent->Hide();
	}

Quit:
	return;
}

void CDarkModPlayer::SelectNext(void)
{
	if(m_Selection < m_Inventory.Num()-1)
		m_Selection++;
	else
		m_Selection = 0;
}



void CDarkModPlayer::SelectPrev(void)
{
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
	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("%u requested from %u Inventory items\r", i, m_Inventory.Num());
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
		DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("%08lX [%s] %lu added to LightList\r", light, light->name.c_str(), m_LightList.Num());
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
			DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("%08lX [%s] %lu removed from LightList\r", light, light->name.c_str(), m_LightList.Num());
		}
	}

	return m_LightList.Num();
}

void CDarkModPlayer::ClearInventory(void)
{
	CInventoryItem inv_item;
	m_FrobEntity = NULL;
	
	m_Inventory.Clear();

	// The first entry in the inventory is always empty and selected by default.
	m_Selection = 0;
	m_Inventory.Append(inv_item);
}


