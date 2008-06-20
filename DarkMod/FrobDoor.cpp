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
#include "Inventory/Item.h"
#include "Inventory/Category.h"
#include "FrobDoor.h"
#include "FrobDoorHandle.h"
#include "../tools/compilers/aas/aasfile.h"
#include "sndProp.h"
#include "randomizer/randomc.h"
#include "StimResponse/StimResponseTimer.h"
#include "StimResponse/StimResponse.h"

#include "../game/ai/aas.h"

extern TRandomCombined<TRanrotWGenerator,TRandomMersenne> rnd;

//===============================================================================
//CFrobDoor
//===============================================================================

const idEventDef EV_TDM_Door_OpenDoor( "OpenDoor", "f" );
const idEventDef EV_TDM_Door_IsPickable( "IsPickable", NULL, 'f' );
const idEventDef EV_TDM_Door_GetDoorhandle( "GetDoorhandle", NULL, 'e' );
const idEventDef EV_TDM_LockpickTimer( "LockpickTimer", "dd");			// boolean 1 = init, 0 = regular processing, type of lockpick

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoor )
	EVENT( EV_TDM_Door_OpenDoor,			CFrobDoor::Event_OpenDoor)
	EVENT( EV_TDM_Door_IsPickable,			CFrobDoor::Event_IsPickable)
	EVENT( EV_TDM_Door_GetDoorhandle,		CFrobDoor::Event_GetDoorhandle)
	EVENT( EV_TDM_LockpickTimer,			CFrobDoor::LockpickTimerEvent)
END_CLASS

static const char *sSampleTypeText[] = 
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

CFrobDoor::CFrobDoor()
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

void CFrobDoor::Save(idSaveGame *savefile) const
{
	savefile->WriteString(m_MasterOpen);

	savefile->WriteInt(m_OpenList.Num());
	for (int i = 0; i < m_OpenList.Num(); i++)
		savefile->WriteString(m_OpenList[i]);

	savefile->WriteString(m_MasterLock);

	savefile->WriteInt(m_LockList.Num());
	for (int i = 0; i < m_LockList.Num(); i++)
		savefile->WriteString(m_LockList[i]);

	savefile->WriteInt(m_Pins.Num());
	for (int i = 0; i < m_Pins.Num(); i++)
	{
		idStringList& stringList = *m_Pins[i];

		savefile->WriteInt(stringList.Num());
		for (int j = 0; j < stringList.Num(); j++)
			savefile->WriteString(stringList[j]);
	}

	savefile->WriteInt(m_RandomPins.Num());
	for (int i = 0; i < m_RandomPins.Num(); i++)
	{
		idStringList& stringList = *m_RandomPins[i];

		savefile->WriteInt(stringList.Num());
		for (int j = 0; j < stringList.Num(); j++)
			savefile->WriteString(stringList[j]);
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

void CFrobDoor::Spawn( void )
{
	idStr lockPins = spawnArgs.GetString("lock_pins", "");

	// If a door is locked but has no pins, it means it can not be picked and needs a key.
	// In that case we can ignore the pins, otherwise we must create the patterns.
	if (!lockPins.IsEmpty())
	{
		idStr head = "lockpick_pin_";
		int b = cv_lp_pin_base_count.GetInteger();

		if (b < MIN_CLICK_NUM)
		{
			b = MIN_CLICK_NUM;
		}

		for (int i = 0; i < lockPins.Length(); i++)
		{
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pin: %u - %c\r", i, lockPins[i]);

			idStringList* pattern = CreatePinPattern(lockPins[i] - 0x030, b, MAX_PIN_CLICKS, 2, head);

			if (pattern != NULL)
			{
				m_Pins.Append(pattern);
				if (cv_lp_pawlow.GetBool() == false)
				{
					pattern->Insert("lockpick_pin_sweetspot");
				}
				else
				{
					pattern->Append("lockpick_pin_sweetspot");
				}
			}
			else
			{
				DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Door [%s]: couldn't create pin pattern for pin %u value %c\r", name.c_str(), i, lockPins[i]);
			}

			if (cv_lp_randomize.GetBool() == true)
			{
				// TODO: Hardcoded 9 is wrong here. Actually the number must be determined by
				// seeing how many positions the lock can have while in transit.
				idStr empty = "";
				pattern = CreatePinPattern(lockPins[i] - 0x030, b, 9, 1, empty);
				if (pattern != NULL)
				{
					pattern->Insert("0");
					m_RandomPins.Append(pattern);
				}
				else
				{
					DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Door [%s]: couldn't create pin jiggle pattern for pin %u value %c\r", name.c_str(), i, lockPins[i]);
				}
			}
		}
	}

	m_Pickable = spawnArgs.GetBool("pickable");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] pickable (%u)\r", name.c_str(), m_Pickable);

	//TODO: Add portal/door pair to soundprop data here, 
	//	replacing the old way in sndPropLoader

	// Flag the AAS areas the door is located in with door travel flag
	for (int i = 0; i < gameLocal.NumAAS(); i++)
	{
		idAAS*	aas = gameLocal.GetAAS(i);
		if (aas == NULL)
		{
			continue;
		}
		
		int areaNum = GetAASArea(aas);
		idStr areatext(areaNum);
		//gameRenderWorld->DrawText(areatext.c_str(), center + idVec3(0,0,i), 0.2f, colorGreen, 
		//	mat3_identity, 1, 10000000);
		aas->SetAreaTravelFlag(areaNum, TFL_DOOR);
	}
}

void CFrobDoor::PostSpawn()
{
	// Let the base class do its stuff first
	CBinaryFrobMover::PostSpawn();

	// Locate the double door entity befor closing our portal
	FindDoubleDoor();

	// Wait until here for the first update of sound loss, in case a double door is open
	UpdateSoundLoss();

	if (!m_Open)
	{
		// Door starts _completely_ closed, try to shut the portal
		ClosePortal();
	}

	// Open the portal if either of the doors is open
	CFrobDoor* doubleDoor = m_DoubleDoor.GetEntity();
	if (m_Open || (doubleDoor != NULL && doubleDoor->IsOpen()))
	{
		OpenPortal();
	}

	m_MasterOpen = spawnArgs.GetString("master_open", "");
	if (!m_MasterOpen.IsEmpty())
	{
		idEntity* entity = gameLocal.FindEntity(m_MasterOpen);

		if (entity != NULL && entity->IsType(CFrobDoor::Type))
		{
			CFrobDoor* master = static_cast<CFrobDoor*>(entity);

			AddToMasterList(master->m_OpenList, m_MasterOpen);
			
			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("master_open [%s] (%u)\r", m_MasterOpen.c_str(), master->m_OpenList.Num());
		}
		else
		{
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_open [%s] not spawned or of wrong type.\r", m_MasterOpen.c_str());
			m_MasterOpen = "";
		}
	}

	m_MasterLock = spawnArgs.GetString("master_lock", "");
	if (!m_MasterLock.IsEmpty())
	{
		idEntity* entity = gameLocal.FindEntity(m_MasterLock);

		if (entity != NULL && entity->IsType(CFrobDoor::Type))
		{
			CFrobDoor* master = static_cast<CFrobDoor*>(entity);

			AddToMasterList(master->m_LockList, m_MasterLock);

			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("master_lock [%s] (%u)\r", m_MasterLock.c_str(), master->m_LockList.Num());
		}
		else
		{
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("master_lock [%s] not spawned or of wrong type\r", m_MasterLock.c_str());
			m_MasterLock = "";
		}
	}

	idStr doorHandleName = spawnArgs.GetString("door_handle", "");
	if (!doorHandleName.IsEmpty())
	{
		idEntity* handleEnt = gameLocal.FindEntity(doorHandleName);

		if (handleEnt != NULL && handleEnt->IsType(CFrobDoorHandle::Type))
		{
			CFrobDoorHandle* handle = static_cast<CFrobDoorHandle*>(handleEnt);
			
			m_Doorhandle = handle;
			m_OriginalPosition = handle->GetPhysics()->GetOrigin();
			m_OriginalAngle = handle->GetPhysics()->GetAxis().ToAngles();
			handle->SetDoor(this);

			// Check if we should bind the handle to ourselves
			if (spawnArgs.GetBool("door_handle_bind_flag", "1"))
			{
				// Set up the frob peer relationship between the door and the handle
				m_FrobPeers.AddUnique(handle->name);
				handle->GetFrobPeers().AddUnique(name);
				handle->m_bFrobable = m_bFrobable;
				handle->Bind(this, true);
			}
		}
		else
		{
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Doorhandle entity not spawned or of wrong type: %s\r", doorHandleName.c_str());
		}
	}

	idStr lockPickBarName = spawnArgs.GetString("lockpick_bar", "");
	if (!lockPickBarName.IsEmpty())
	{
		idEntity* bar = gameLocal.FindEntity(lockPickBarName);

		if (bar != NULL)
		{
			m_Bar = bar;

			// The bar overrides the handle info if it exists, because
			// this is the one that has to move if the lock is picked.
			m_OriginalPosition = bar->GetPhysics()->GetOrigin();
			m_OriginalAngle = bar->GetPhysics()->GetAxis().ToAngles();

			// Check if we should bind the bar to ourselves
			if (spawnArgs.GetBool("lockpick_bar_bind_flag", "1"))
			{
				// Set up the frob peer relationship between the door and the bar
				m_FrobPeers.AddUnique(bar->name);
				bar->GetFrobPeers().AddUnique(name);
				bar->m_bFrobable = m_bFrobable;
				bar->Bind(this, true);
			}
		}
		else
		{
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Bar entity name not found: %s\r", lockPickBarName.c_str());
		}
	}

	m_PinRotationFraction = spawnArgs.GetAngles("lockpick_rotate", "0 0 0") / m_Pins.Num();
	// Check if the rotation is empty and set the flag.
	// Set the pin rotation flag to FALSE if the rotation fraction is zero
	m_PinRotationFractionFlag = (m_PinRotationFraction.Compare(idAngles(0,0,0), VECTOR_EPSILON) == false);
	
	m_PinTranslationFraction = spawnArgs.GetVector("lockpick_translate", "0 0 0") / m_Pins.Num();
	// Check if the translation is empty and set the flag.
	// Set the pin translation flag to FALSE if the translation fraction is zero
	m_PinTranslationFractionFlag = (m_PinTranslationFraction.Compare(idVec3(0,0,0), VECTOR_EPSILON) == false);
}

bool CFrobDoor::IsPickable()
{
	return m_Pickable;
}

void CFrobDoor::Lock(bool bMaster)
{
	if (bMaster && !m_MasterLock.IsEmpty())
	{
		// We have a master lock, re-route the call to it
		idEntity* ent = gameLocal.FindEntity(m_MasterLock);

		if (ent == NULL || !ent->IsType(CFrobDoor::Type))
		{
			static_cast<CFrobDoor*>(ent)->Lock(false);
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), m_MasterLock.c_str());
			m_MasterLock = ""; // clear the master string
		}
	}
	else
	{
		// Pass the call to the base class, the OnLock() event will be fired 
		// if the locking process is allowed
		CBinaryFrobMover::Lock(bMaster);
	}
}

void CFrobDoor::OnLock(bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnLock(bMaster);

	if (bMaster)
	{
		// When this door locks, lock all the "peers" in the list too
		for (int i = 0; i < m_LockList.Num(); i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_LockList[i].c_str());

			idEntity* ent = gameLocal.FindEntity(m_LockList[i]);

			if (ent != NULL && ent->IsType(CFrobDoor::Type))
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", m_LockList[i].c_str());
				static_cast<CFrobDoor*>(ent)->Lock(false);
			}
			else
			{
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found or of wrong type\r", m_LockList[i].c_str());
			}
		}
	}
}

void CFrobDoor::Unlock(bool bMaster)
{
	if (bMaster && !m_MasterLock.IsEmpty())
	{
		// We have a master lock, re-route the call to it
		idEntity* ent = gameLocal.FindEntity(m_MasterLock);

		if (ent == NULL || !ent->IsType(CFrobDoor::Type))
		{
			static_cast<CFrobDoor*>(ent)->Unlock(false);
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not of class CFrobDoor\r", name.c_str(), m_MasterLock.c_str());
			m_MasterLock = ""; // clear the master string
		}
	}
	else
	{
		// Pass the call to the base class, the OnUnlock() event will be fired 
		// if the locking process is allowed
		CBinaryFrobMover::Unlock(bMaster);
	}
}

void CFrobDoor::OnUnlock(bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnUnlock(bMaster);

	if (bMaster) 
	{
		// When this door unlocks, unlock all the "peers" in the list too
		for (int i = 0; i < m_LockList.Num(); i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_LockList[i].c_str());

			idEntity* ent = gameLocal.FindEntity(m_LockList[i]);

			if (ent != NULL && ent->IsType(CFrobDoor::Type))
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", m_LockList[i].c_str());
				static_cast<CFrobDoor*>(ent)->Unlock(false);
			}
			else
			{
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found or of wrong type\r", m_LockList[i].c_str());
			}
		}
	}
}

void CFrobDoor::Open(bool bMaster)
{
	// If we have a doorhandle we want to tap it before the door starts to open if the door wasn't
	// already interrupted
	CFrobDoorHandle* handle = m_Doorhandle.GetEntity();

	if (handle != NULL && !m_bInterrupted)
	{
		// Relay the call to the handle, it will come back to us
		handle->Tap();

		if (bMaster)
		{
			// In master mode, tap the handles of the master_open chain too
			// Cycle through our "open peers" list and issue the call to them
			for (int i = 0; i < m_OpenList.Num(); i++)
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_OpenList[i].c_str());

				idEntity* ent = gameLocal.FindEntity(m_OpenList[i]);

				if (ent != NULL && ent->IsType(CFrobDoor::Type))
				{
					DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for open\r", m_OpenList[i].c_str());
					CFrobDoor* other = static_cast<CFrobDoor*>(ent);

					if (other->GetDoorhandle() != NULL)
					{
						other->GetDoorhandle()->Tap();
					}
				}
				else
				{
					DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] not spawned or not of class CFrobDoor\r", name.c_str(), m_OpenList[i].c_str());
				}
			}
		}
	}
	else
	{
		// No handle present, let's just proceed with our own open routine
		OpenDoor(bMaster);
	}
}

void CFrobDoor::OpenDoor(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Opening\r" );

	// Handle master mode
	if (bMaster && !m_MasterLock.IsEmpty())
	{
		idEntity* ent = gameLocal.FindEntity(m_MasterLock);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			static_cast<CFrobDoor*>(ent)->Open(false);
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not spawned or of wrong type.\r", name.c_str(), m_MasterLock.c_str());
		}
	}
	else // Non-master mode
	{
		// Now pass the call to the base class, which will invoke PreOpen() and the other events
		CBinaryFrobMover::Open(bMaster);
	}
}

void CFrobDoor::OnStartOpen(bool wasClosed, bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnStartOpen(wasClosed, bMaster);

	// We are actually opening, open the visportal too
	OpenPortal();

	// Update soundprop
	UpdateSoundLoss();

	if (bMaster)
	{
		// Cycle through our "open peers" list and issue the call to them
		for (int i = 0; i < m_OpenList.Num(); i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_OpenList[i].c_str());

			idEntity* ent = gameLocal.FindEntity(m_OpenList[i]);

			if (ent != NULL && ent->IsType(CFrobDoor::Type))
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for open\r", m_OpenList[i].c_str());
				static_cast<CFrobDoor*>(ent)->Open(false);
			}
			else
			{
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] not spawned or not of class CFrobDoor\r", name.c_str(), m_OpenList[i].c_str());
			}
		}
	}
}

void CFrobDoor::Close(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closing\r" );

	// Handle master mode
	if (bMaster && !m_MasterLock.IsEmpty())
	{
		idEntity* ent = gameLocal.FindEntity(m_MasterLock);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			static_cast<CFrobDoor*>(ent)->Close(false);
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] is not spawned or of wrong type.\r", name.c_str(), m_MasterLock.c_str());
		}
	}
	else
	{
		// Invoke the close method in non-master mode, this will fire the events
		CBinaryFrobMover::Close(bMaster);
	}
}

void CFrobDoor::OnStartClose(bool wasOpen, bool bMaster)
{
	CBinaryFrobMover::OnStartClose(wasOpen, bMaster);

	if (bMaster)
	{
		// Cycle through our "open peers" list and issue the call to them
		for (int i = 0; i < m_OpenList.Num(); i++)
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", m_OpenList[i].c_str());

			idEntity* ent = gameLocal.FindEntity(m_OpenList[i]);

			if (ent != NULL && ent->IsType(CFrobDoor::Type))
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for close\r", m_OpenList[i].c_str());
				static_cast<CFrobDoor*>(ent)->Close(false);
			}
			else
			{
				DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] not spawned or not of class CFrobDoor\r", name.c_str(), m_OpenList[i].c_str());
			}
		}
	}
}

void CFrobDoor::OnClosedPositionReached() 
{
	// Call the base class
	CBinaryFrobMover::OnClosedPositionReached();

	// Try to close the visportal
	ClosePortal();

	// Update the sound propagation values
	UpdateSoundLoss();
}

bool CFrobDoor::UsedBy(IMPULSE_STATE nState, CInventoryItem* item)
{
	bool bRc = false;

	if (item == NULL || item->GetItemEntity() == NULL)
		return bRc;

	idEntity* ent = item->GetItemEntity();

	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("[%s] used by [%s] (%u)  Masterlock: [%s]\r", 
		name.c_str(), ent->name.c_str(), m_UsedBy.Num(), m_MasterLock.c_str());

	// First we check if this item is a lockpick. It has to be of the toolclass lockpick
	// and the type must be set.
	char type = 0;

	idStr str = ent->spawnArgs.GetString("toolclass", "");
	if (str == "lockpick")
	{
		str = ent->spawnArgs.GetString("type", "");
		if (str.Length() == 1)
		{
			type = str[0];
		}
	}

	// Process the lockpick
	if (type != 0)
	{
		ELockpickSoundsample sample;

		switch (nState)
		{
		case IS_PRESSED:	sample = LPSOUND_INIT; 
			break;
		case IS_RELEASED:	sample = LPSOUND_RELEASED;
			break;
		default:			sample = LPSOUND_REPEAT;
		};
		
		ProcessLockpick(static_cast<int>(type), sample);
	}

	// When we are here we know that the item is usable
	// so we have to check if it is associated with this entity.
	// We ignore all repeat or release events to make it a true 
	// IMPULSE event.
	if (nState != IS_PRESSED)
		return bRc;

	// Cycle through our known "UsedBy" names and see if we have a match
	int n = m_UsedBy.Num();
	for (int i = 0; i < n; i++)
	{
		if (ent->name == m_UsedBy[i])
		{
			ToggleLock();
			bRc = true;
		}
	}

	// If we haven't found the entity here. we can still try to unlock it
	// via a master
	if (bRc == false && !m_MasterLock.IsEmpty())
	{
		// Try to find the master lock entity
		idEntity* masterEnt = gameLocal.FindEntity(m_MasterLock);
		if (masterEnt != NULL)
		{
			bRc = masterEnt->UsedBy(nState, item);
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Master entity [%s] could not be found\r", name.c_str(), masterEnt->name.c_str());
		}
	}

	// angua: we can't unlock the door with this key
	if (bRc == false && IsLocked() && item->Category()->GetName() == "Keys")
	{
		StartSound("snd_wrong_key", SND_CHANNEL_ANY, 0, false, NULL);
	}

	return bRc;
}

void CFrobDoor::UpdateSoundLoss(void)
{
	if (!areaPortal) return; // not a portal door

	CFrobDoor* doubleDoor = m_DoubleDoor.GetEntity();

	// If we have no double door, assume the bool to be "open"
	bool doubleDoorIsOpen = (doubleDoor != NULL) ? doubleDoor->IsOpen() : true;
	bool thisDoorIsOpen = IsOpen();

	// TODO: check the spawnarg: sound_char, and return the 
	// appropriate loss for that door, open or closed

	float lossDB = 0.0f;

	if (thisDoorIsOpen && doubleDoorIsOpen)
	{
		lossDB = spawnArgs.GetFloat("loss_open", "1.0");
	}
	else if (thisDoorIsOpen && !doubleDoorIsOpen)
	{
		lossDB = spawnArgs.GetFloat("loss_double_open", "1.0");
	}
	else
	{
		lossDB = spawnArgs.GetFloat("loss_closed", "15.0");
	}
	
	gameLocal.m_sndProp->SetPortalLoss(areaPortal, lossDB);
}

void CFrobDoor::FindDoubleDoor(void)
{
	idClipModel* clipModelList[MAX_GENTITIES];

	idBounds clipBounds = physicsObj.GetAbsBounds();
	clipBounds.ExpandSelf( 10.0f );

	int numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, CONTENTS_SOLID, clipModelList, MAX_GENTITIES );

	for (int i = 0; i < numListedClipModels; i++) 
	{
		idClipModel* clipModel = clipModelList[i];
		idEntity* obEnt = clipModel->GetEntity();

		// Ignore self
		if (obEnt == this) continue;

		if (obEnt->IsType(CFrobDoor::Type))
		{
			// check the visportal inside the other door, if it's the same as this one => double door
			int testPortal = gameRenderWorld->FindPortal(obEnt->GetPhysics()->GetAbsBounds());

			if (testPortal == areaPortal && testPortal != 0)
			{
				DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor %s found double door %s\r", name.c_str(), obEnt->name.c_str());
				m_DoubleDoor = static_cast<CFrobDoor*>(obEnt);
			}
		}
	}
}

CFrobDoorHandle* CFrobDoor::GetDoorhandle()
{
	return m_Doorhandle.GetEntity();
}

CFrobDoor* CFrobDoor::GetDoubleDoor()
{
	return m_DoubleDoor.GetEntity();
}

void CFrobDoor::ClosePortal()
{
	CFrobDoor* doubleDoor = m_DoubleDoor.GetEntity();

	if (doubleDoor == NULL || !doubleDoor->IsOpen())
	{
		// No double door or double door is closed too
		if (areaPortal) 
		{
			SetPortalState(false);
		}
	}
}

void CFrobDoor::SetFrobbed(bool val)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("door_body [%s] %08lX is frobbed\r", name.c_str(), this);

	// First invoke the base class, then check for a doorhandle
	idEntity::SetFrobbed(val);

	CFrobDoorHandle* handle = m_Doorhandle.GetEntity();

	if (handle != NULL)
	{
		handle->SetFrobbed(val);
	}
}

bool CFrobDoor::IsFrobbed()
{
	// If the door has a handle and it is frobbed, then we are also considered 
	// to be frobbed. 
	CFrobDoorHandle* handle = m_Doorhandle.GetEntity();

	if (handle != NULL && handle->IsFrobbed())
	{
		return true;
	}

	return idEntity::IsFrobbed();
}

idStringList *CFrobDoor::CreatePinPattern(int clicks, int baseCount, int maxCount, int strNumLen, idStr &str)
{
	if (clicks < 0 || clicks > 9)
	{
		return NULL;
	}

	if (clicks == 0)
	{
		clicks = 10;
	}

	clicks += baseCount;
	idStringList* returnValue = new idStringList();

	idStr head = va(str+"%%0%uu", strNumLen);

	for (int i = 0; i < clicks; i++)
	{
		// Choose a different random number generator every other frame
		int r = (i % 2) ? gameLocal.random.RandomInt(maxCount) : rnd.IRandom(0, maxCount);

		idStr click = va(head, r);
		returnValue->Append(click);

		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("PinPattern %u : %s\r", i, click.c_str());
	}

	return returnValue;
}

void CFrobDoor::LockpickTimerEvent(int cType, ELockpickSoundsample nSampleType)
{
	DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Lockpick Timerevent\r");
	ProcessLockpick(cType, nSampleType);
}

void CFrobDoor::SetHandlePosition(EHandleReset nPos, int msec, int pin, int sample)
{
	// If we have a bar entity, this is taken as moving entity
	idEntity* handle = m_Bar.GetEntity();
	if (handle == NULL)
	{
		handle = m_Doorhandle.GetEntity();
	}

	if (handle == NULL) return; // neither handle nor bar => quit

	if (nPos == HANDLE_POS_ORIGINAL)
	{
		// Set the handle back to its original position
		handle->GetPhysics()->SetAxis(m_OriginalAngle.ToMat3());
		idVec3 position = handle->GetLocalCoordinates(m_OriginalPosition);
		handle->SetOrigin(position);

		handle->UpdateVisuals();
	}
	else
	{
		int n = m_Pins[pin]->Num();
		if (sample < 0)
		{
			sample = 0;
		}

		if (m_RandomPins.Num() > 0)
		{
			idList<idStr>& sl = *m_RandomPins[pin];
			idStr s = sl[sample];
			sample = s[0] - '0';
		}

		// Set the rotation
		if (m_PinRotationFractionFlag)
		{
			m_SampleRotationFraction = m_PinRotationFraction / n;

			idAngles angles = (m_PinRotationFraction * pin) + (m_SampleRotationFraction * sample);

			handle->GetPhysics()->SetAxis(angles.ToMat3());
		}

		// Set the translation
		if (m_PinTranslationFractionFlag)
		{
			m_SampleTranslationFraction = m_PinTranslationFraction / n;

			idVec3 position = (m_PinTranslationFraction * pin) + (m_SampleTranslationFraction * sample);
			position += m_OriginalPosition;
			position = handle->GetLocalCoordinates(position);

			handle->SetOrigin(position);
		}

		handle->UpdateVisuals();
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

	if (common->ButtonState(KEY_FROM_IMPULSE(IMPULSE_51)) == false)
	{
		m_KeyReleased = true;
	}

	// If a key has been pressed and the lock is already picked, we play a sample
	// to indicate that the lock doesn't need picking anymore. This we do only
	// if there is not currently a sound sample still playing, in which case we 
	// can ignore that event and wait for all sample events to arrive.
	if (m_FirstLockedPinIndex >= m_Pins.Num())
	{
		if (nSampleType == LPSOUND_INIT || nSampleType == LPSOUND_REPEAT)
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

void CFrobDoor::Event_GetDoorhandle()
{
	idThread::ReturnEntity(m_Doorhandle.GetEntity());
}

void CFrobDoor::Event_IsPickable()
{
	idThread::ReturnInt(m_Pickable);
}

void CFrobDoor::Event_OpenDoor(float master)
{
	OpenDoor(master != 0.0f ? true : false);
}
