/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.6  2005/09/27 08:05:43  ishtvan
 * *) Doors now activate/deactivate visportals that are touching them
 *
 * *) Fixed multifrob problem, implemented stopping of doors partway by frobbing
 *
 * *) Only returns the closed acoustical loss when completely closed
 *
 * Revision 1.5  2005/04/07 08:42:38  ishtvan
 * Added placeholder method GetSoundLoss, which is called by CsndProp
 *
 * Revision 1.4  2004/11/24 21:59:06  sparhawk
 * *) Multifrob implemented
 * *) Usage of items against other items implemented.
 * *) Basic Inventory system added.
 *
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

// TODO: A parameter must be added to translate doors. Currently they
// can be only rotated when they are opened.

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
	m_bInterrupted = false;
	m_bIntentOpen = false;
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
	idEntity *e;
	CFrobDoor *master;
	idAngles tempAngle;

	LoadTDMSettings();

	if(spawnArgs.GetString("master_open", "", str))
	{
		if((e = gameLocal.FindEntity(str.c_str())) != NULL)
		{
			if((master = dynamic_cast<CFrobDoor *>(e)) != NULL)
			{
				if(AddToMasterList(master->m_OpenList, str) == true)
					m_MasterOpen = str;
				DM_LOG(LC_SYSTEM, LT_INFO).LogString("master_open [%s] (%u)\r", m_MasterOpen.c_str(), master->m_OpenList.Num());
			}
			else
				DM_LOG(LC_SYSTEM, LT_ERROR).LogString("master_open [%s] is of wrong type\r", m_MasterOpen.c_str());
		}
		else
			DM_LOG(LC_SYSTEM, LT_ERROR).LogString("master_open [%s] not yet defined\r", m_MasterOpen.c_str());
	}

	if(spawnArgs.GetString("master_lock", "", str))
	{
		if((e = gameLocal.FindEntity(str.c_str())) != NULL)
		{
			if((master = dynamic_cast<CFrobDoor *>(e)) != NULL)
			{
				if(AddToMasterList(master->m_LockList, str) == true)
					m_MasterLock = str;
				DM_LOG(LC_SYSTEM, LT_INFO).LogString("master_open [%s] (%u)\r", m_MasterOpen.c_str(), master->m_LockList.Num());
			}
			else
				DM_LOG(LC_SYSTEM, LT_ERROR).LogString("master_open [%s] is of wrong type\r", m_MasterOpen.c_str());
		}
		else
			DM_LOG(LC_SYSTEM, LT_ERROR).LogString("master_open [%s] not yet defined\r", m_MasterOpen.c_str());
	}

	m_Rotate = spawnArgs.GetAngles("rotate", "0 90 0");

	m_Open = spawnArgs.GetBool("open");
	DM_LOG(LC_SYSTEM, LT_INFO).LogString("[%s] open (%u)\r", name.c_str(), m_Open);

	m_Locked = spawnArgs.GetBool("locked");
	DM_LOG(LC_SYSTEM, LT_INFO).LogString("[%s] locked (%u)\r", name.c_str(), m_Locked);

	m_Pickable = spawnArgs.GetBool("pickable");
	DM_LOG(LC_SYSTEM, LT_INFO).LogString("[%s] pickable (%u)\r", name.c_str(), m_Pickable);


	// Check for a visportal within this door (NOTE: areaPortal is a member from idMover)
	areaPortal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds() );
	if( areaPortal > 0 )
		DM_LOG(LC_SYSTEM, LT_DEBUG).LogString("Door [%s] found portal handle %d on spawn \r", name.c_str(), areaPortal);

	physicsObj.GetLocalAngles( tempAngle );

	if ( !m_Open ) 
	{
		// Door starts closed

		Event_ClosePortal();

		m_ClosedAngles = tempAngle;
		m_OpenAngles = m_Rotate;
	}
	else
	{
		m_ClosedAngles = -1*m_Rotate;
		m_OpenAngles = tempAngle;
	}

	// set the first intent according to the initial doorstate
	m_bIntentOpen = !m_Open;

	//TODO: Add portal/door pair to soundprop data here, 
	//	replacing the old way in sndPropLoader
}

void CFrobDoor::Lock(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;

	StartSound("snd_unlock", SND_CHANNEL_ANY, 0, false, NULL);
	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Lock(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_LockList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Calling linked entity [%s] for lock\r", m_LockList[i].c_str());
					ent->Lock(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		DM_LOG(LC_FROBBING, LT_DEBUG).LogString("[%s] Door is locked\r", name.c_str());
		m_Locked = true;
	}
}

void CFrobDoor::Unlock(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;

	StartSound("snd_unlock", SND_CHANNEL_ANY, 0, false, NULL);
	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Unlock(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_LockList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Calling linked entity [%s] for lock\r", m_LockList[i].c_str());
					ent->Unlock(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		DM_LOG(LC_FROBBING, LT_DEBUG).LogString("[%s] Door is unlocked\r", name.c_str());
		m_Locked = false;

		ToggleOpen();
	}
}

void CFrobDoor::ToggleLock(void)
{
	// A door can only be un/locked when it is closed.
	if(m_Open == true)
	{
		ToggleOpen();
		return;
	}

	if(m_Locked == true)
		Unlock(true);
	else
		Lock(true);
}

void CFrobDoor::Open(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;
	idAngles tempAng;

	// If the door is already open, we don't have anything to do. :)
	if(m_Open == true && !m_bInterrupted)
		return;

	DM_LOG(LC_FROBBING, LT_DEBUG).LogString("FrobDoor: Opening\r" );

	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Open(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_OpenList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Calling linked entity [%s] for lock\r", m_OpenList[i].c_str());
					ent->Open(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		if(m_Locked == true)
			StartSound( "snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
		else
		{
			// don't play the sound if the door was not closed all the way
			if( !m_bInterrupted )
				StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );

			physicsObj.GetLocalAngles( tempAng );
			Event_RotateOnce( (m_OpenAngles - tempAng).Normalize180() );
			m_Open = true;

			// Open visportal
			Event_OpenPortal();
		}
	}
}

void CFrobDoor::Close(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;
	idAngles tempAng;

	// If the door is already closed, we don't have anything to do. :)
	if(m_Open == false)
		return;

	DM_LOG(LC_FROBBING, LT_DEBUG).LogString("FrobDoor: Closing\r" );

	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Close(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_OpenList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG).LogString("Calling linked entity [%s] for lock\r", m_OpenList[i].c_str());
					ent->Close(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		physicsObj.GetLocalAngles( tempAng );
		Event_RotateOnce( (m_ClosedAngles - tempAng).Normalize180() );

	}
}

void CFrobDoor::ToggleOpen(void)
{

	// Check if the door is stopped.
	if( physicsObj.GetAngularExtrapolationType() == EXTRAPOLATION_NONE )
	{
		DM_LOG(LC_FROBBING, LT_DEBUG).LogString("FrobDoor: Was stationary on frobbing\r" );


		if(m_bIntentOpen == true)
		{
			Open(true);
		}
		else
		{
			Close(true);
		}

		m_bInterrupted = false;
		
		goto Quit;
	}

	DM_LOG(LC_FROBBING, LT_DEBUG).LogString("FrobDoor: Interrupted!  Stopping door\r" );

	// Otherwise, door is moving.  Stop it
	m_bInterrupted = true;
	Event_StopRotating();

	// reverse the intent
	m_bIntentOpen = !m_bIntentOpen;

Quit:
	return;
}

bool CFrobDoor::UsedBy(idEntity *ent)
{
	bool bRc = false;
	int i, n;
	CFrobDoor *master;
	idEntity *e;

	if(ent == NULL)
		return false;

	DM_LOG(LC_FROBBING, LT_INFO).LogString("[%s] used by [%s] (%u)  Masterlock: [%s]\r", 
		name.c_str(), ent->name.c_str(), m_UsedBy.Num(), m_MasterLock.c_str());

	// When we are here we know that the item is usable
	// so we have to check if it is associated with this entity.
	n = m_UsedBy.Num();
	for(i = 0; i < n; i++)
	{
		if(ent->name == m_UsedBy[i])
		{
			ToggleLock();
			bRc = true;
		}
	}

	// If we haven't found the entity here. we can still try to unlock it
	// via a master
	if(bRc == false && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((master = dynamic_cast<CFrobDoor *>(e)) != NULL)
				bRc = master->UsedBy(ent);
			else
				DM_LOG(LC_FROBBING, LT_ERROR).LogString("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}

	return bRc;
}

float CFrobDoor::GetSoundLoss(void)
{
	float returnval;

	// TODO: Get open/closed state

	// TODO: check the spawnarg: sound_char, and return the 
	// appropriate loss for that door, open or closed

	// TODO: Account for double doors, get state of other door

	if (m_Open == true)
	{
		returnval = 1;
	}
	else
	{
		returnval = 15;
	}

	return returnval;
}

void CFrobDoor::DoneRotating(void)
{
	idMover::DoneRotating();

	DM_LOG(LC_FROBBING, LT_DEBUG).LogString("FrobDoor: Done rotating\r" );
	
	// if the door is not completely opened or closed, do nothing
	if( m_bInterrupted )
		goto Quit;

	// door has completely closed
	if( !m_bIntentOpen )
	{
		DM_LOG(LC_FROBBING, LT_DEBUG).LogString("FrobDoor: Closed completely\r" );
		// close the visportal
		Event_ClosePortal();

		m_bIntentOpen = true;
		m_Open = false;

		// play the closing sound when the door closes completely
		StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );
	}
	// door has completely opened
	else
	{
		DM_LOG(LC_FROBBING, LT_DEBUG).LogString("FrobDoor: Opened completely\r" );
		m_bIntentOpen = false;
	}

Quit:
	return;
}
