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
 * Revision 1.13  2007/02/03 21:56:21  sparhawk
 * Removed old inventories and fixed a bug in the new one.
 *
 * Revision 1.12  2006/08/07 06:52:55  ishtvan
 * added m_FrobTrace variable that gets set by idPlayer::FrobCheck
 *
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

CDarkModPlayer::CDarkModPlayer(void)
{
	m_FrobEntity = NULL;
	m_FrobJoint = INVALID_JOINT;
	m_FrobID = 0;
	m_FrobEntityPrevious = NULL;
	m_LightgemValue = 0;

	// TODO: Spawn grabber from a .def file (maybe?)
	this->grabber = new CGrabber();
}

CDarkModPlayer::~CDarkModPlayer(void)
{
	// remove grabber object	
	this->grabber->PostEventSec( &EV_Remove, 0 );
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
