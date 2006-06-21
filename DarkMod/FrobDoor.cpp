/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.21  2006/06/21 15:02:27  sparhawk
 * FrobDoor derived now from BinaryFrobMover
 *
 * Revision 1.20  2006/06/21 13:05:32  sparhawk
 * Added version tracking per cpp module
 *
 * Revision 1.19  2006/06/05 21:32:18  sparhawk
 * Timercode updated
 *
 * Revision 1.18  2006/05/07 22:51:49  ishtvan
 * fixed door closing sound
 *
 * Revision 1.17  2006/05/07 21:52:12  ishtvan
 * *) fixed interruption on opening problem
 * *) Added 'interruptable' spawnarg
 * *) Added offset position for translation in case the item starts inbetween states
 * *) Added translation speed variable
 *
 * Revision 1.16  2006/05/06 21:02:45  sparhawk
 * Fixed crash when door callback called itself.
 *
 * Revision 1.15  2006/05/06 20:23:35  sparhawk
 * Fixed problem with determining when the animation is finished.
 *
 * Revision 1.14  2006/05/03 21:31:21  sparhawk
 * Statechange callback script added.
 *
 * Revision 1.13  2006/05/02 20:39:32  sparhawk
 * Translation added
 *
 * Revision 1.12  2006/04/29 22:10:56  sparhawk
 * Added some script functions to query the state of a door.
 *
 * Revision 1.11  2006/01/23 00:18:56  ishtvan
 * fix - soundprop data now updated at spawn
 *
 * Revision 1.10  2006/01/22 09:20:24  ishtvan
 * rewrote to match new soundprop interface
 *
 * Revision 1.9  2005/12/14 23:20:18  ishtvan
 * rotation relative to orientation bugfix
 *
 * Revision 1.8  2005/11/19 17:26:48  sparhawk
 * LogString with macro replaced
 *
 * Revision 1.7  2005/09/29 04:03:08  ishtvan
 * added support for double doors
 *
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

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

#include "../game/Game_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "FrobDoor.h"
#include "sndProp.h"

//===============================================================================
//CFrobDoor
//===============================================================================

const idEventDef EV_TDM_Door_Open( "Open", "f" );
const idEventDef EV_TDM_Door_Close( "Close", "f" );
const idEventDef EV_TDM_Door_ToggleOpen( "ToggleOpen", NULL );
const idEventDef EV_TDM_Door_Lock( "Lock", "f" );
const idEventDef EV_TDM_Door_Unlock( "Unlock", "f" );
const idEventDef EV_TDM_Door_FindDouble( "FindDoubleDoor", NULL );
const idEventDef EV_TDM_Door_GetPickable( "GetPickable", NULL, 'f' );

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoor )
	EVENT( EV_TDM_Door_Open,				CFrobDoor::Open)
	EVENT( EV_TDM_Door_Close,				CFrobDoor::Close)
	EVENT( EV_TDM_Door_Lock,				CFrobDoor::Lock)
	EVENT( EV_TDM_Door_Unlock,				CFrobDoor::Unlock)
	EVENT( EV_TDM_Door_FindDouble,			CFrobDoor::FindDoubleDoor)
	EVENT( EV_TDM_Door_GetPickable,			CFrobDoor::GetPickable)
END_CLASS


CFrobDoor::CFrobDoor(void)
: CBinaryFrobMover()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
	m_FrobActionScript = "frob_door";
	m_Pickable = true;
	m_DoubleDoor = NULL;
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
	idAngles tempAngle, partialAngle;

	CBinaryFrobMover::Spawn();

	LoadTDMSettings();

	if(spawnArgs.GetString("master_open", "", str))
	{
		if((e = gameLocal.FindEntity(str.c_str())) != NULL)
		{
			if((master = dynamic_cast<CFrobDoor *>(e)) != NULL)
			{
				if(AddToMasterList(master->m_OpenList, str) == true)
					m_MasterOpen = str;
				DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("master_open [%s] (%u)\r", m_MasterOpen.c_str(), master->m_OpenList.Num());
			}
			else
				DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_open [%s] is of wrong type\r", m_MasterOpen.c_str());
		}
		else
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_open [%s] not yet defined\r", m_MasterOpen.c_str());
	}

	if(spawnArgs.GetString("master_lock", "", str))
	{
		if((e = gameLocal.FindEntity(str.c_str())) != NULL)
		{
			if((master = dynamic_cast<CFrobDoor *>(e)) != NULL)
			{
				if(AddToMasterList(master->m_LockList, str) == true)
					m_MasterLock = str;
				DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("master_open [%s] (%u)\r", m_MasterOpen.c_str(), master->m_LockList.Num());
			}
			else
				DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_open [%s] is of wrong type\r", m_MasterOpen.c_str());
		}
		else
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_open [%s] not yet defined\r", m_MasterOpen.c_str());
	}

	m_Pickable = spawnArgs.GetBool("pickable");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] pickable (%u)\r", name.c_str(), m_Pickable);

	//schedule finding the double doors for after all entities have spawned
	PostEventMS( &EV_TDM_Door_FindDouble, 0 );

	//TODO: Add portal/door pair to soundprop data here, 
	//	replacing the old way in sndPropLoader
}

void CFrobDoor::Lock(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;

	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Lock(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_LockList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", m_LockList[i].c_str());
					ent->Lock(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		CBinaryFrobMover::Lock(bMaster);
	}
}

void CFrobDoor::Unlock(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;

	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Unlock(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_LockList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", m_LockList[i].c_str());
					ent->Unlock(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		CBinaryFrobMover::Unlock(bMaster);
	}
}

void CFrobDoor::Open(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;
	idAngles tempAng;

	// If the door is already open, we don't have anything to do. :)
	if(m_Open == true && !m_bInterrupted)
		return;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Opening\r" );

	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Open(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_OpenList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", m_OpenList[i].c_str());
					ent->Open(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		if(m_Locked == true)
			StartSound( "snd_locked", SND_CHANNEL_ANY, 0, false, NULL );
		else
		{
			// don't play the sound if the door was not closed all the way
			if( !m_bInterrupted )
			{	
				m_StateChange = true;
				
				StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );
				
				// Open visportal
				Event_OpenPortal();
			}

			physicsObj.GetLocalAngles( tempAng );
			
			m_Open = true;
			m_Rotating = true;
			m_Translating = true;

			Event_RotateOnce( (m_OpenAngles - tempAng).Normalize180() );
			
			if( m_TransSpeed )
				Event_SetMoveSpeed( m_TransSpeed );

			Event_MoveToPos(m_StartPos +  m_Translation);

			// Update soundprop
			UpdateSoundLoss();
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

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closing\r" );

	if(bMaster == true && m_MasterLock.Length() != 0)
	{
		if((e = gameLocal.FindEntity(m_MasterLock.c_str())) != NULL)
		{
			if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				ent->Close(false);
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}
	else
	{
		int i, n;

		n = m_LockList.Num();
		for(i = 0; i < n; i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_LockList[i].c_str());
			if((e = gameLocal.FindEntity(m_OpenList[i].c_str())) != NULL)
			{
				if((ent = dynamic_cast<CFrobDoor *>(e)) != NULL)
				{
					DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", m_OpenList[i].c_str());
					ent->Close(false);
				}
				else
					DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
			}
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found\r", m_LockList[i].c_str());
		}

		physicsObj.GetLocalAngles( tempAng );
		
		m_StateChange = true;
		m_Rotating = true;
		m_Translating = true;

		Event_RotateOnce( (m_ClosedAngles - tempAng).Normalize180() );
		
		if( m_TransSpeed )
				Event_SetMoveSpeed( m_TransSpeed );

		Event_MoveToPos(m_StartPos);
	}
}

bool CFrobDoor::UsedBy(idEntity *ent)
{
	bool bRc = false;
	int i, n;
	CFrobDoor *master;
	idEntity *e;

	if(ent == NULL)
		return false;

	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("[%s] used by [%s] (%u)  Masterlock: [%s]\r", 
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
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}

	return bRc;
}

void CFrobDoor::UpdateSoundLoss(void)
{
	float SetVal(0.0f);
	bool bDoubleOpen(true);

	if( !areaPortal )
		goto Quit;

	if( m_DoubleDoor )
		bDoubleOpen = m_DoubleDoor->m_Open;

	// TODO: check the spawnarg: sound_char, and return the 
	// appropriate loss for that door, open or closed

	if (m_Open && bDoubleOpen)
	{
		SetVal = spawnArgs.GetFloat( "loss_open", "1.0");
	}
	else if (m_Open && !bDoubleOpen)
	{
		SetVal = spawnArgs.GetFloat( "loss_double_open", "1.0");
	}
	else
	{
		SetVal = spawnArgs.GetFloat( "loss_closed", "15.0");
	}
	
	gameLocal.m_sndProp->SetPortalLoss( areaPortal, SetVal );

Quit:
	return;
}

void CFrobDoor::DoneMoving(void)
{
	idMover::DoneMoving();
    m_Translating = false;

	DoneStateChange();
}


void CFrobDoor::DoneRotating(void)
{
	idMover::DoneRotating();
    m_Rotating = false;

	DoneStateChange();
}

void CFrobDoor::FindDoubleDoor(void)
{
	int i, numListedClipModels, testPortal;
	idBounds clipBounds;
	idEntity *obEnt;
	idClipModel *clipModel;
	idClipModel *clipModelList[ MAX_GENTITIES ];

	clipBounds = physicsObj.GetAbsBounds();
	clipBounds.ExpandSelf( 10.0f );

	numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, CONTENTS_SOLID, clipModelList, MAX_GENTITIES );

	for ( i = 0; i < numListedClipModels; i++ ) 
	{
		clipModel = clipModelList[i];
		obEnt = clipModel->GetEntity();

		// Ignore self
		if( obEnt == this )
			continue;

		if ( obEnt->IsType( CFrobDoor::Type ) ) 
		{
			// check the visportal inside the other door, if it's the same as this one, double door
			testPortal = gameRenderWorld->FindPortal( obEnt->GetPhysics()->GetAbsBounds() );

			if( testPortal == areaPortal && testPortal != 0 )
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor %s found double door %s\r", name.c_str(), obEnt->name.c_str() );
				m_DoubleDoor = static_cast<CFrobDoor *>( obEnt );
			}
		}
	}

	// Wait until here for the first update of sound loss, in case double door is open
	UpdateSoundLoss();
}

void CFrobDoor::GetPickable(void)
{
	idThread::ReturnInt(m_Pickable);
}

