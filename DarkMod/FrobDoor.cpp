/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.3  2004/11/21 01:02:03  sparhawk
 * Doors can now be properly opened and have sound.
 *
 * Revision 1.2  2004/11/16 23:56:03  sparhawk
 * Frobcode has been generalized now and resides for all entities in the base classe.
 *
 * Revision 1.1  2004/11/14 20:19:11  sparhawk
 * Initial Release
 *
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "../game/Game_local.h"
#include "DarkModGlobals.h"
#include "FrobDoor.h"

//===============================================================================
//CFrobDoor
//===============================================================================

const idEventDef EV_TDM_Door_Open( "Open", NULL );
const idEventDef EV_TDM_Door_Close( "Close", NULL );
const idEventDef EV_TDM_Door_ToggleOpen( "ToggleOpen", NULL );
const idEventDef EV_TDM_Door_Lock( "Lock", NULL );
const idEventDef EV_TDM_Door_Unlock( "Unlock", NULL );
const idEventDef EV_TDM_Door_ToggleLock( "ToggleLock", NULL );

CLASS_DECLARATION( idMover, CFrobDoor )
	EVENT( EV_TDM_Door_Open,				CFrobDoor::Open)
	EVENT( EV_TDM_Door_Close,				CFrobDoor::Close)
	EVENT( EV_TDM_Door_ToggleOpen,			CFrobDoor::ToggleOpen)
	EVENT( EV_TDM_Door_Lock,				CFrobDoor::Lock)
	EVENT( EV_TDM_Door_Unlock,				CFrobDoor::Unlock)
	EVENT( EV_TDM_Door_ToggleLock,			CFrobDoor::ToggleLock)
END_CLASS

CFrobDoor::CFrobDoor(void)
{
	DM_LOG(LC_FUNCTION, LT_DEBUG).LogString("this: %08lX [%s]\r", this, __FUNCTION__);
	m_FrobActionScript = "frob_door";
	m_Open = false;
	m_Locked = false;
	m_Pickable = true;
}

void CFrobDoor::Save(idSaveGame *savefile) const
{
}

void CFrobDoor::Restore( idRestoreGame *savefile )
{
}

void CFrobDoor::WriteToSnapshot( idBitMsgDelta &msg ) const
{
}

void CFrobDoor::ReadFromSnapshot( const idBitMsgDelta &msg )
{
}

void CFrobDoor::Spawn( void )
{
	idStr str;
	idMover::Spawn();

	LoadTDMSettings();

	if(spawnArgs.GetString("master_open", "", str))
	{
		if(AddToMasterList(m_OpenList, str) == true)
			m_MasterOpen = str;
	}

	if(spawnArgs.GetString("master_lock", "", str))
	{
		if(AddToMasterList(m_LockList, str) == true)
			m_MasterLock = str;
	}

	if(spawnArgs.GetString("master_lock", "", str))
	{
		if(AddToMasterList(m_LockList, str) == true)
			m_MasterLock = str;
	}

	m_Rotate = spawnArgs.GetAngles("rotate", "0 90 0");

	m_Open = spawnArgs.GetBool("open");
	DM_LOG(LC_FROBBING, LT_INFO).LogString("[%s] open (%u)\r", name.c_str(), m_Open);

	m_Locked = spawnArgs.GetBool("locked");
	DM_LOG(LC_FROBBING, LT_INFO).LogString("[%s] locked (%u)\r", name.c_str(), m_Locked);

	m_Pickable = spawnArgs.GetBool("pickable");
	DM_LOG(LC_FROBBING, LT_INFO).LogString("[%s] pickable (%u)\r", name.c_str(), m_Pickable);
}

void CFrobDoor::Lock(void)
{
	// Numerical locks can always be locked by changing the number.
	// All other locks need to have an entity which represents the key. And 
	// this key must be equipped in the inventory to be used. Lockpicking
	// will not lock a door once it is open.
}

void CFrobDoor::Unlock(void)
{
	// Unlock needs to be an entity passed in which is the key. We can do this 
	// either by specifing the entity directly, or we can look which inventory
	// item currently is equipped. The question of course is, how are numerical 
	// locks treated? Of course the lockpicking can also be used.
}

void CFrobDoor::ToggleLock(void)
{
	if(m_Locked == true)
		Unlock();
	else
		Lock();
}

void CFrobDoor::Open(void)
{
	// If the door is already open, we don't have anything to do. :)
	if(m_Open == true)
		return;

	if(m_Locked == true)
		StartSound( "snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
	else
	{
		StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );
		Event_RotateOnce(m_Rotate);
		m_Open = true;
	}
}

void CFrobDoor::Close(void)
{
	// If the door is already closed, we don't have anything to do. :)
	if(m_Open == false)
		return;

	idAngles angle = m_Rotate * (-1);
	StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );
	Event_RotateOnce(angle);
	m_Open = false;
}

void CFrobDoor::ToggleOpen(void)
{
	if(m_Open == true)
		Close();
	else
		Open();
}

