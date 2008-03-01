/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "DarkModGlobals.h"
#include "BinaryFrobMover.h"
#include "FrobDoor.h"
#include "FrobDoorHandle.h"
#include "sndProp.h"
#include "randomizer/randomc.h"
#include "StimResponse/StimResponseTimer.h"

extern TRandomCombined<TRanrotWGenerator,TRandomMersenne> rnd;

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
const idEventDef EV_TDM_Door_GetDoorhandle( "GetDoorhandle", NULL, 'e' );
const idEventDef EV_TDM_LockpickTimer( "LockpickTimer", "dd");			// boolean 1 = init, 0 = regular processing, type of lockpick
const idEventDef EV_TDM_Door_Init( "Init", NULL);						// set the bar/handle entity on the door

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoor )
	EVENT( EV_TDM_Door_Init,				CFrobDoor::Event_Init)
	EVENT( EV_TDM_Door_Open,				CFrobDoor::Open)
	EVENT( EV_TDM_Door_Close,				CFrobDoor::Close)
	EVENT( EV_TDM_Door_Lock,				CFrobDoor::Lock)
	EVENT( EV_TDM_Door_Unlock,				CFrobDoor::Unlock)
	EVENT( EV_TDM_Door_FindDouble,			CFrobDoor::FindDoubleDoor)
	EVENT( EV_TDM_Door_GetPickable,			CFrobDoor::GetPickable)
	EVENT( EV_TDM_Door_GetDoorhandle,		CFrobDoor::GetDoorhandle)
	EVENT( EV_TDM_LockpickTimer,			CFrobDoor::LockpickTimerEvent)
END_CLASS

static char *sSampleTypeText[] = 
{
	"LPSOUND_INIT",					// Initial call (impulse has been triggered)
	"LPSOUND_REPEAT",				// Call from the keyboardhandler for repeated presses
	"LPSOUND_RELEASED",				// Call from the keyboardhandler for released presses
	"LPSOUND_PIN_SAMPLE",			// Callback for pin sample
	"LPSOUND_PIN_FAILED",			// Callback when the pin failed sound is finished
	"LPSOUND_PIN_SUCCESS",			// Callback for the success sound sample
	"LPSOUND_WRONG_LOCKPICK",		// Callback for the wrong lockpick sample
	"LPSOUND_LOCK_PICKED"			// Callback for the pin picked
};


E_SDK_SIGNAL_STATE SigOpen(idEntity *oEntity, void *pData)
{
	E_SDK_SIGNAL_STATE rc = SIG_REMOVE;
	CFrobDoor *e;

	if((e = dynamic_cast<CFrobDoor *>(oEntity)) == NULL)
		goto Quit;

	e->OpenDoor(false);

Quit:
	return rc;
}


CFrobDoor::CFrobDoor(void)
: CBinaryFrobMover()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
	m_FrobActionScript = "frob_door";
	m_Pickable = true;
	m_DoubleDoor = NULL;
	m_Doorhandle = NULL;
	m_FirstLockedPinIndex = 0;
	m_SoundPinSampleIndex = 0;
	m_SoundTimerStarted = 0;
	m_PinTranslationFractionFlag = false;
	m_PinRotationFractionFlag = false;
	m_KeyReleased = false;
}

CFrobDoor::~CFrobDoor(void)
{
}

void CFrobDoor::Save(idSaveGame *savefile) const
{
	savefile->WriteString(m_MasterOpen.c_str());

	savefile->WriteInt(m_OpenList.Num());
	for (int i = 0; i < m_OpenList.Num(); i++)
		savefile->WriteString(m_OpenList[i].c_str());

	savefile->WriteString(m_MasterLock.c_str());

	savefile->WriteInt(m_LockList.Num());
	for (int i = 0; i < m_LockList.Num(); i++)
		savefile->WriteString(m_LockList[i].c_str());

	savefile->WriteInt(m_Pins.Num());
	for (int i = 0; i < m_Pins.Num(); i++)
	{
		idStringList& stringList = *m_Pins[i];

		savefile->WriteInt(stringList.Num());
		for (int j = 0; j < stringList.Num(); j++)
			savefile->WriteString(stringList[j].c_str());
	}

	savefile->WriteInt(m_RandomPins.Num());
	for (int i = 0; i < m_RandomPins.Num(); i++)
	{
		idStringList& stringList = *m_RandomPins[i];

		savefile->WriteInt(stringList.Num());
		for (int j = 0; j < stringList.Num(); j++)
			savefile->WriteString(stringList[j].c_str());
	}

	savefile->WriteBool(m_PinTranslationFractionFlag);
	savefile->WriteVec3(m_PinTranslationFraction);
	savefile->WriteVec3(m_SampleTranslationFraction);

	savefile->WriteBool(m_PinRotationFractionFlag);
	savefile->WriteAngles(m_PinRotationFraction);
	savefile->WriteAngles(m_SampleRotationFraction);

	savefile->WriteVec3(m_OriginalPosition);
	savefile->WriteAngles(m_OriginalAngle);

	savefile->WriteBool(m_Pickable);
	savefile->WriteInt(m_FirstLockedPinIndex);
	savefile->WriteInt(m_SoundPinSampleIndex);
	savefile->WriteInt(m_SoundTimerStarted);

	m_DoubleDoor.Save(savefile);
	m_Doorhandle.Save(savefile);
	m_Bar.Save(savefile);
}

void CFrobDoor::Restore( idRestoreGame *savefile )
{
	int num;

	savefile->ReadString(m_MasterOpen);

	savefile->ReadInt(num);
	m_OpenList.SetNum(num);
	for (int i = 0; i < num; i++)
		savefile->ReadString(m_OpenList[i]);

	savefile->ReadString(m_MasterLock);

	savefile->ReadInt(num);
	m_LockList.SetNum(num);
	for (int i = 0; i < num; i++)
		savefile->ReadString(m_LockList[i]);

	int numPins;
	savefile->ReadInt(numPins);
	m_Pins.SetNum(numPins);
	for (int i = 0; i < numPins; i++)
	{
		m_Pins[i] = new idStringList;
		
		savefile->ReadInt(num);
		m_Pins[i]->SetNum(num);
		for (int j = 0; j < num; j++)
			savefile->ReadString( (*m_Pins[i])[j] );
	}

	savefile->ReadInt(numPins);
	m_RandomPins.SetNum(numPins);
	for (int i = 0; i < numPins; i++)
	{
		m_RandomPins[i] = new idStringList;
		
		savefile->ReadInt(num);
		m_RandomPins[i]->SetNum(num);
		for (int j = 0; j < num; j++)
			savefile->ReadString( (*m_RandomPins[i])[j] );
	}

	savefile->ReadBool(m_PinTranslationFractionFlag);
	savefile->ReadVec3(m_PinTranslationFraction);
	savefile->ReadVec3(m_SampleTranslationFraction);

	savefile->ReadBool(m_PinRotationFractionFlag);
	savefile->ReadAngles(m_PinRotationFraction);
	savefile->ReadAngles(m_SampleRotationFraction);

	savefile->ReadVec3(m_OriginalPosition);
	savefile->ReadAngles(m_OriginalAngle);

	savefile->ReadBool(m_Pickable);
	savefile->ReadInt(m_FirstLockedPinIndex);
	savefile->ReadInt(m_SoundPinSampleIndex);
	savefile->ReadInt(m_SoundTimerStarted);

	m_DoubleDoor.Restore(savefile);
	m_Doorhandle.Restore(savefile);
	m_Bar.Restore(savefile);
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

	// If a door is locked but has no pins, it means it can not be picked and needs a key.
	// In that case we can ignore the pins, otherwise we must create the patterns.
	if(spawnArgs.GetString("lock_pins", "", str))
	{
		idStr head = "lockpick_pin_";
		idStr empty = "";
		int n = str.Length();
		int b = cv_lp_pin_base_count.GetInteger();
		if(b < MIN_CLICK_NUM)
			b = MIN_CLICK_NUM;

		for(int i = 0; i < n; i++)
		{
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pin: %u - %c\r", i, str[i]);
			idStringList *p = CreatePinPattern(str[i] - 0x030, b, MAX_PIN_CLICKS, 2, head);
			if(p)
			{
				m_Pins.Append(p);
				if(cv_lp_pawlow.GetBool() == false)
					p->Insert("lockpick_pin_sweetspot");
				else
					p->Append("lockpick_pin_sweetspot");
			}
			else
				DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Door [%s]: couldn't create pin pattern for pin %u value %c\r", name.c_str(), i, str[i]);

			if(cv_lp_randomize.GetBool() == true)
			{
				// TODO: Hardcoded 9 is wrong here. Actually the number must be determined by
				// seeing how many positions the lock can have while in transit.
				p = CreatePinPattern(str[i] - 0x030, b, 9, 1, empty);
				if(p)
				{
					p->Insert("0");
					m_RandomPins.Append(p);
				}
				else
					DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Door [%s]: couldn't create pin jiggle pattern for pin %u value %c\r", name.c_str(), i, str[i]);
			}
		}
	}

	m_Pickable = spawnArgs.GetBool("pickable");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] pickable (%u)\r", name.c_str(), m_Pickable);

	//schedule intialization of the doors for after all entities have spawned
	PostEventMS(&EV_TDM_Door_Init, 0);

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
	idAngles tempAng;

	// If the door is already open, we don't have anything to do. :)
	if(m_Open == true && !m_bInterrupted && !IsBlocked())
	{
		m_bIntentOpen = false;
		return;
	}

	// If we have a doorhandle we want to tap it before the door starts to open if the door wasn't
	// already interrupted
	if (m_Doorhandle.GetEntity() != NULL && !m_bInterrupted)
	{
		m_StateChange = true;
		m_Doorhandle.GetEntity()->Tap();
	}
	else
	{
		OpenDoor(bMaster);
		m_bInterrupted = false;
	}
}

void CFrobDoor::OpenDoor(bool bMaster)
{
	CFrobDoor *ent;
	idEntity *e;
	idAngles tempAng;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Opening\r" );

	// Open door handle if there is one
	if(m_Doorhandle.GetEntity() != NULL)
		m_Doorhandle.GetEntity()->Open(false);

	// Handle master mode
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

				// trigger our targets on opening, if set to do so
				if( spawnArgs.GetBool("trigger_on_open","") )
					ActivateTargets( this );
			}

			physicsObj.GetLocalAngles( tempAng );
			
			m_Open = true;
			m_Rotating = true;
			m_Translating = true;

			Event_RotateOnce( (m_OpenAngles - tempAng).Normalize180() );
			
			if( m_TransSpeed )
				Event_SetMoveSpeed( m_TransSpeed );

			idVec3 tv3 = ( m_StartPos +  m_Translation );
			Event_MoveToPos( tv3 );

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

	if(m_Open == false)
		return;

	m_bInterrupted = false;

	// When we close the door, we don't want to do the handle tap, as 
	// it looks a bit strange
//	if(m_Doorhandle)
//		m_Doorhandle->Tap();


	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closing\r" );

	// Open door handle if there is one
	//if(m_Doorhandle)
	//	m_Doorhandle->Open(false);

	// Handle master mode
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

bool CFrobDoor::UsedBy(IMPULSE_STATE nState, idEntity *ent)
{
	bool bRc = false;
	int i, n;
	CFrobDoor *master;
	idEntity *e;
	idStr s;
	char type = 0;

	if(ent == NULL)
		return false;

	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("[%s] used by [%s] (%u)  Masterlock: [%s]\r", 
		name.c_str(), ent->name.c_str(), m_UsedBy.Num(), m_MasterLock.c_str());

	// First we check if this item is a lockpick. It has to be of the toolclass lockpick
	// and the type must be set.
	ent->spawnArgs.GetString("toolclass", "", s);
	if(s == "lockpick")
	{
		ent->spawnArgs.GetString("type", "", s);
		if(s.Length() == 1)
			type = s[0];
	}

	// Process the lockpick
	if(type != 0)
	{
		ELockpickSoundsample v;

		if(nState == IS_PRESSED)
			v = LPSOUND_INIT;
		else if(nState == IS_RELEASED)
			v = LPSOUND_RELEASED;
		else
			v = LPSOUND_REPEAT;

		ProcessLockpick((int)type, v);
	}

	// When we are here we know that the item is usable
	// so we have to check if it is associated with this entity.
	// We ignore all repeat or release events to make it a true 
	// IMPULSE event.
	if(nState != IS_PRESSED)
		goto Quit;

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
				bRc = master->UsedBy(nState, ent);
			else
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), e->name.c_str());
		}
	}

Quit:
	return bRc;
}

void CFrobDoor::UpdateSoundLoss(void)
{
	float SetVal(0.0f);
	bool bDoubleOpen(true);

	if( !areaPortal )
		goto Quit;

	if( m_DoubleDoor.GetEntity() )
		bDoubleOpen = m_DoubleDoor.GetEntity()->m_Open;

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

	// Open the portal if either of the doors is open
	if( m_Open || (m_DoubleDoor.GetEntity() && m_DoubleDoor.GetEntity()->m_Open) )
		Event_OpenPortal();
}

void CFrobDoor::GetPickable(void)
{
	idThread::ReturnInt(m_Pickable);
}

void CFrobDoor::GetDoorhandle(void)
{
	idThread::ReturnEntity(m_Doorhandle.GetEntity());
}

CFrobDoor* CFrobDoor::GetDoubleDoor( void )
{
	return m_DoubleDoor.GetEntity();
}

void CFrobDoor::ClosePortal( void )
{
	if( !m_DoubleDoor.GetEntity() || !m_DoubleDoor.GetEntity()->m_Open )
		Event_ClosePortal();
}

void CFrobDoor::SetFrobbed(bool val)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("door_body [%s] %08lX is frobbed\r", name.c_str(), this);
	idEntity::SetFrobbed(val);
	if(m_Doorhandle.GetEntity())
		m_Doorhandle.GetEntity()->SetFrobbed(val);
}

bool CFrobDoor::IsFrobbed(void)
{
	// If the door has a handle and it is frobbed, then we are also considered 
	// to be frobbed. Maybe this changes later, when the lockpicking is
	// implemented, but usually this should be true.
	if(m_Doorhandle.GetEntity())
	{
		if(m_Doorhandle.GetEntity()->IsFrobbed() == true)
			return true;
	}

	return idEntity::IsFrobbed();
}

void CFrobDoor::ToggleOpen(void)
{
	CBinaryFrobMover::ToggleOpen();
	if(m_Doorhandle.GetEntity())
		m_Doorhandle.GetEntity()->ToggleOpen();
}

void CFrobDoor::ToggleLock(void)
{
	CBinaryFrobMover::ToggleLock();
	if(m_Doorhandle.GetEntity())
		m_Doorhandle.GetEntity()->ToggleLock();
}

idStringList *CFrobDoor::CreatePinPattern(int Clicks, int BaseCount, int MaxCount, int StrNumLen, idStr &str)
{
	idStringList *rc = NULL;
	int i, r;
	idStr click;

	if(!(Clicks >= 0 && Clicks <= 9))
		return NULL;

	if(Clicks == 0)
		Clicks = 10;

	Clicks += BaseCount;
	rc = new idStringList();

	idStr head;
	sprintf(head, str+"%%0%uu", StrNumLen);

	for(i = 0; i < Clicks; i++)
	{
		if(i % 2)
			r = gameLocal.random.RandomInt(MaxCount);
		else
			r = rnd.IRandom(0, MaxCount);

		sprintf(click, head, r);
		rc->Append(click);
		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("PinPattern %u : %s\r", i, click.c_str());
	}

	return rc;
}

void CFrobDoor::LockpickTimerEvent(int cType, ELockpickSoundsample nSampleType)
{
	DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Lockpick Timerevent\r");
	ProcessLockpick(cType, nSampleType);
}


void CFrobDoor::SetHandlePosition(EHandleReset nPos, int msec, int pin, int sample)
{
	idAngles a;
	idVec3 v;
	double n;

	idEntity *m = m_Bar.GetEntity();
	if(!m)
		m = m_Doorhandle.GetEntity();

	if(!m)
		return;

	if(nPos == HANDLE_POS_ORIGINAL)
	{
		m->GetPhysics()->SetAxis(m_OriginalAngle.ToMat3());
		v = m->GetLocalCoordinates(m_OriginalPosition);
		m->SetOrigin(v);
		m->UpdateVisuals();
	}
	else
	{
		n = m_Pins[pin]->Num();
		if(sample < 0)
			sample = 0;

		if(m_RandomPins.Num() > 0)
		{
			idList<idStr> &sl = *m_RandomPins[pin];
			idStr s = sl[sample];
			sample = s[0] - '0';
		}

		if(m_PinRotationFractionFlag == true)
		{
			m_SampleRotationFraction = m_PinRotationFraction/n;

			a = (m_PinRotationFraction * pin) + (m_SampleRotationFraction * sample);
			m->GetPhysics()->SetAxis(a.ToMat3());
		}

		if(m_PinTranslationFractionFlag == true)
		{
			m_SampleTranslationFraction = m_PinTranslationFraction/n;

			v = (m_PinTranslationFraction * pin) + (m_SampleTranslationFraction * sample);
			v += m_OriginalPosition;
			v = m->GetLocalCoordinates(v);
			m->SetOrigin(v);
		}

		m->UpdateVisuals();
	}
}

void CFrobDoor::ProcessLockpick(int cType, ELockpickSoundsample nSampleType)
{
	int sample_delay, pick_timeout;
	idStr oPickSound;
	char type = cType;
	int length = 0;
	idStr pick;
	idVec3 pos;
	idAngles angle;

	if(common->ButtonState(KEY_FROM_IMPULSE(IMPULSE_51)) == false)
		m_KeyReleased = true;

	// If a key has been pressed and the lock is already picked, we play a sample
	// to indicate that the lock doesn't need picking anymore. This we do only
	// if there is not currently a sound sample still playing, in which case we 
	// can ignore that event and wait for all sample events to arrive.
	if(m_FirstLockedPinIndex >= m_Pins.Num())
	{
		if(nSampleType == LPSOUND_INIT || nSampleType == LPSOUND_REPEAT)
		{
			if(m_SoundTimerStarted <= 0)
			{
				oPickSound = "lockpick_lock_picked";
				PropPickSound(oPickSound, cType, LPSOUND_LOCK_PICKED, 0, HANDLE_POS_ORIGINAL, -1, -1);
				DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] already picked\r", name.c_str());
			}
		}
		else
		{
			if(nSampleType == LPSOUND_PIN_SAMPLE)
			{
				m_SoundTimerStarted--;
				if(m_SoundTimerStarted <= 0)
					m_SoundTimerStarted = 0;
			}
		}

		goto Quit;
	}

	// Now check if the pick is of the correct type. If no picktype is given, or
	// the mapper doesn't care, we ignore it.
	spawnArgs.GetString("lock_picktype", "", pick);
	if(m_FirstLockedPinIndex < pick.Length())
	{
		if(!(pick[m_FirstLockedPinIndex] == '*' || pick[m_FirstLockedPinIndex] == type))
		{
			if(m_SoundTimerStarted == 0)
			{
				oPickSound = "lockpick_pick_wrong";
				PropPickSound(oPickSound, cType, LPSOUND_WRONG_LOCKPICK, 0, HANDLE_POS_ORIGINAL, -1, -1);
				DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pick attempt: %u/%u failed (len: %u).\r", m_FirstLockedPinIndex, m_SoundPinSampleIndex, length);
			}
			else
			{
				if(!(nSampleType == LPSOUND_INIT || nSampleType == LPSOUND_REPEAT))
					m_SoundTimerStarted--;
			}

			goto Quit;
		}
	}

	DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("%s : Timer: %u  PinIndex: %u  SampleIndex: %u\r", sSampleTypeText[nSampleType], m_SoundTimerStarted, m_FirstLockedPinIndex, m_SoundPinSampleIndex);
	switch(nSampleType)
	{
		case LPSOUND_INIT:
		{
			// If we receive an INIT call, and the soundtimer has already been started, it means that
			// the user released the lockpick key and pressed it again, before the sample has been finished.
			// We can safely ignore this case, because this should be treated the same as if the user
			// didn't release the key at all while playing the lockpick samples.
			if(m_SoundTimerStarted > 0)
			{
				m_KeyReleased = false;
				goto Quit;
			}

			// Otherwise we reset the lock to the initial soundsample for the current pin. Pins are not
			// reset, so the player doesn't have to start all over if he gets interrupted while picking.
			m_SoundPinSampleIndex = -1;
			SetHandlePosition(HANDLE_POS_SAMPLE, 0, m_FirstLockedPinIndex);
		}
		break;

		case LPSOUND_PIN_FAILED:
		case LPSOUND_PIN_SUCCESS:
		case LPSOUND_WRONG_LOCKPICK:
		case LPSOUND_LOCK_PICKED:			// Should never happen but it doesn't hurt either. :)
			m_SoundTimerStarted--;
		break;

		case LPSOUND_PIN_SAMPLE:
			m_SoundTimerStarted--;
			if(m_SoundTimerStarted <= 0)
			{
				m_SoundTimerStarted = 0;
				break;
			}

		// If the pin sample has been finished and we get the callback we check if
		// the key is still pressed. If the user released the key in this intervall
		// and we have to check whether it was the correct pin, and if yes, it will
		// be unlocked.
		case LPSOUND_RELEASED:
		{
			CancelEvents(&EV_TDM_LockpickTimer);
			m_SoundTimerStarted--;

			if(m_SoundTimerStarted <= 0)
				m_SoundTimerStarted = 0;

			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pick attempt: %u/%u Type: %c\r", m_FirstLockedPinIndex, m_SoundPinSampleIndex, type);

			idList<idStr> &l = *m_Pins[m_FirstLockedPinIndex];

			if(m_SoundPinSampleIndex == l.Num()-1)
			{
				// It was correct so we advance to the next pin.
				m_FirstLockedPinIndex++;

				// If it was the last pin, the user successfully picked the lock.
				if(m_FirstLockedPinIndex >= m_Pins.Num())
				{
					m_FirstLockedPinIndex = m_Pins.Num();
					oPickSound = "lockpick_lock_picked";
					PropPickSound(oPickSound, cType, LPSOUND_PIN_SUCCESS, 0, HANDLE_POS_ORIGINAL, 0, 0);
					Unlock(true);
					DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] successfully picked!\r", name.c_str());
					goto Quit;
				}
				else
				{
					oPickSound = "lockpick_pin_success";
					m_SoundPinSampleIndex = 0;
					PropPickSound(oPickSound, cType, LPSOUND_PIN_SUCCESS, 0, HANDLE_POS_ORIGINAL, m_FirstLockedPinIndex, m_SoundPinSampleIndex);
					DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] successfully picked!\r", name.c_str());
				}
			}
			else
			{
				m_SoundPinSampleIndex = 0;
				oPickSound = "lockpick_pin_fail";
				PropPickSound(oPickSound, cType, LPSOUND_PIN_FAILED, 0, HANDLE_POS_SAMPLE, m_FirstLockedPinIndex, m_SoundPinSampleIndex);
				DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pick attempt: %u/%u failed (len: %u).\r", m_FirstLockedPinIndex, m_SoundPinSampleIndex, length);
			}
		}
		break;

		case LPSOUND_REPEAT:				// Here is the interesting part.
		{
			// If we are still playing a sample, we can ignore that keypress.
			if(m_SoundTimerStarted > 0)
				goto Quit;

			sample_delay = cv_lp_sample_delay.GetInteger();
			idList<idStr> &l = *m_Pins[m_FirstLockedPinIndex];


			m_SoundPinSampleIndex++;
			pick_timeout = 0;
			if(cv_lp_pawlow.GetBool() == false && m_SoundPinSampleIndex == 0)
				pick_timeout = cv_lp_pick_timeout.GetInteger();

			if(m_SoundPinSampleIndex >= l.Num()-1)
			{
				if(m_SoundPinSampleIndex >= l.Num())
					m_SoundPinSampleIndex = 0;
				else
				{
					if(cv_lp_pawlow.GetBool() == true)
						pick_timeout = cv_lp_pick_timeout.GetInteger();
				}
			}

			oPickSound = l[m_SoundPinSampleIndex];
			PropPickSound(oPickSound, cType, LPSOUND_PIN_SAMPLE, sample_delay+pick_timeout, HANDLE_POS_SAMPLE, m_FirstLockedPinIndex, m_SoundPinSampleIndex);
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Picksound started [%s] %u/%u Type: %c\r", oPickSound.c_str(), m_FirstLockedPinIndex, m_SoundPinSampleIndex, type);
		}
		break;
	}

Quit:
	return;
}

void CFrobDoor::PropPickSound(idStr &oPickSound, int cType, ELockpickSoundsample nSampleType, int time, EHandleReset nHandlePos, int PinIndex, int SampleIndex)
{
	int length = 0;

	m_SoundTimerStarted++;
	PropSoundDirect(oPickSound, true, false );
	idSoundShader const *shader = declManager->FindSound(oPickSound);
	StartSoundShader(shader, SND_CHANNEL_ANY, 0, false, &length);
	if(PinIndex != -1)
		SetHandlePosition(nHandlePos, length, PinIndex, SampleIndex);
	PostEventMS(&EV_TDM_LockpickTimer, length+time, cType, nSampleType);
}

void CFrobDoor::Event_Init(void)
{
	idStr str;
	idEntity *e;
	CFrobDoor *master;
	idAngles tempAngle, partialAngle;
	bool flag = true;

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
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_open [%s] not spawned\r", m_MasterOpen.c_str());
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
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_open [%s] not spawned\r", m_MasterOpen.c_str());
	}

	if(spawnArgs.GetString("door_handle", "", str))
	{
		e = gameLocal.FindEntity(str);

		if(e)
		{
			CFrobDoorHandle *dh = dynamic_cast<CFrobDoorHandle *>(e);
			if(dh)
			{
				m_Doorhandle = dh;
				m_OriginalPosition = e->GetPhysics()->GetOrigin();
				m_OriginalAngle = e->GetPhysics()->GetAxis().ToAngles();
				dh->m_Door = this;
			}
			else
				DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Doorhandle entity not a valid doorhandle: %s\r", str.c_str());

			spawnArgs.GetBool("door_handle_bind_flag", "1", flag);
			if(flag == true)
			{
				m_FrobPeers.AddUnique(e->name);
				e->GetFrobPeers().AddUnique(name);
				e->m_bFrobable = m_bFrobable;
				e->Bind(this, true);
			}
		}
		else
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Doorhandle entity not found: %s\r", str.c_str());
	}

	if(spawnArgs.GetString("lockpick_bar", "", str))
	{
		e = gameLocal.FindEntity(str);

		if(e)
		{
			m_Bar = e;

			// The bar overrides the handle info if it exists, because
			// this is the one that has to move if the lock is picked.
			m_OriginalPosition = e->GetPhysics()->GetOrigin();
			m_OriginalAngle = e->GetPhysics()->GetAxis().ToAngles();

			flag = true;
			spawnArgs.GetBool("lockpick_bar_bind_flag", "1", flag);
			if(flag == true)
			{
				m_FrobPeers.AddUnique(e->name);
				e->GetFrobPeers().AddUnique(name);
				e->m_bFrobable = m_bFrobable;
				e->Bind(this, true);
			}
		}
		else
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Bar entity name not found: %s\r", str.c_str());
	}

	m_PinRotationFraction = spawnArgs.GetAngles("lockpick_rotate", "0 0 0")/m_Pins.Num();
	// Check if the rotation is empty and set the flag.
	{
		idAngles cmp;
		cmp.Zero();
		if(m_PinRotationFraction.Compare(cmp, VECTOR_EPSILON) == true)
			m_PinRotationFractionFlag = false;
		else
			m_PinRotationFractionFlag = true;
	}

	m_PinTranslationFraction = spawnArgs.GetVector("lockpick_translate", "0 0 0")/m_Pins.Num();
	// Check if the translation is empty and set the flag.
	{
		idVec3 cmp;
		cmp.Zero();
		if(m_PinTranslationFraction.Compare(cmp, VECTOR_EPSILON) == true)
			m_PinTranslationFractionFlag = false;
		else
			m_PinTranslationFractionFlag = true;
	}

	FindDoubleDoor();
}
