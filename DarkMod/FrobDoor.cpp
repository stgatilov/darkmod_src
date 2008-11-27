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
	m_CloseOnLock = false;
	m_DoubleDoor = NULL;
	m_FirstLockedPinIndex = 0;
	m_SoundPinSampleIndex = 0;
	m_SoundTimerStarted = 0;
	m_PinTranslationFractionFlag = false;
	m_PinRotationFractionFlag = false;
}

void CFrobDoor::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(m_OpenPeers.Num());
	for (int i = 0; i < m_OpenPeers.Num(); i++)
	{
		savefile->WriteString(m_OpenPeers[i]);
	}

	savefile->WriteInt(m_LockPeers.Num());
	for (int i = 0; i < m_LockPeers.Num(); i++)
	{
		savefile->WriteString(m_LockPeers[i]);
	}

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
	savefile->WriteBool(m_CloseOnLock);
	savefile->WriteInt(m_FirstLockedPinIndex);
	savefile->WriteInt(m_SoundPinSampleIndex);
	savefile->WriteInt(m_SoundTimerStarted);

	m_DoubleDoor.Save(savefile);

	savefile->WriteInt(m_Doorhandles.Num());
	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		m_Doorhandles[i].Save(savefile);
	}
	m_Bar.Save(savefile);
}

void CFrobDoor::Restore( idRestoreGame *savefile )
{
	int num;

	savefile->ReadInt(num);
	m_OpenPeers.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(m_OpenPeers[i]);
	}

	savefile->ReadInt(num);
	m_LockPeers.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(m_LockPeers[i]);
	}

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
	savefile->ReadBool(m_CloseOnLock);
	savefile->ReadInt(m_FirstLockedPinIndex);
	savefile->ReadInt(m_SoundPinSampleIndex);
	savefile->ReadInt(m_SoundTimerStarted);

	m_DoubleDoor.Restore(savefile);
	
	savefile->ReadInt(num);
	m_Doorhandles.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_Doorhandles[i].Restore(savefile);
	}

	m_Bar.Restore(savefile);
}

void CFrobDoor::Spawn( void )
{
	idStr lockPins = spawnArgs.GetString("lock_pins", "");

	// If a door is locked but has no pins, it means it can not be picked and needs a key.
	// In that case we can ignore the pins, otherwise we must create the patterns.
	if (!lockPins.IsEmpty())
	{
		idStr head = "snd_lockpick_pin_";
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
					pattern->Insert("snd_lockpick_pin_sweetspot");
				}
				else
				{
					pattern->Append("snd_lockpick_pin_sweetspot");
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

	// Locate the double door entity before closing our portal
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

	// Search for all spawnargs matching "open_peer_N" and add them to our open peer list
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("open_peer"); kv != NULL; kv = spawnArgs.MatchPrefix("open_peer", kv))
	{
		// Add the peer
		AddOpenPeer(kv->GetValue());
	}

	// Search for all spawnargs matching "lock_peer_N" and add them to our lock peer list
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("lock_peer"); kv != NULL; kv = spawnArgs.MatchPrefix("lock_peer", kv))
	{
		// Add the peer
		AddLockPeer(kv->GetValue());
	}

	idStr doorHandleName = spawnArgs.GetString("door_handle", "");
	if (!doorHandleName.IsEmpty())
	{
		idEntity* handleEnt = gameLocal.FindEntity(doorHandleName);

		if (handleEnt != NULL && handleEnt->IsType(CFrobDoorHandle::Type))
		{
			// Convert to frobdoorhandle pointer and call the helper function
			CFrobDoorHandle* handle = static_cast<CFrobDoorHandle*>(handleEnt);
			AddDoorhandle(handle);

			// Check if we should bind the named handle to ourselves
			if (spawnArgs.GetBool("door_handle_bind_flag", "1"))
			{
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
				bar->AddFrobPeer(name);
				bar->SetFrobable(m_bFrobable);
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

	// greebo: Should we auto-setup the doorhandles?
	if (spawnArgs.GetBool("auto_setup_door_handles", "1"))
	{
		AutoSetupDoorHandles();
	}

	// greebo: Should we auto-setup the double door behaviour?
	if (spawnArgs.GetBool("auto_setup_double_door", "1"))
	{
		AutoSetupDoubleDoor();
	}
}

bool CFrobDoor::IsPickable()
{
	return m_Pickable;
}

void CFrobDoor::Lock(bool bMaster)
{
	// Pass the call to the base class, the OnLock() event will be fired 
	// if the locking process is allowed
	CBinaryFrobMover::Lock(bMaster);
}

void CFrobDoor::OnLock(bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnLock(bMaster);

	// greebo: Reset the lockpicking values
	m_FirstLockedPinIndex = 0;
	m_SoundTimerStarted = 0;
	m_SoundPinSampleIndex = -1;

	if (bMaster)
	{
		LockPeers();
	}
}

void CFrobDoor::Unlock(bool bMaster)
{
	// Pass the call to the base class, the OnUnlock() event will be fired 
	// if the locking process is allowed
	CBinaryFrobMover::Unlock(bMaster);
}

void CFrobDoor::OnUnlock(bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnUnlock(bMaster);

	if (bMaster) 
	{
		UnlockPeers();
	}
}

void CFrobDoor::Open(bool bMaster)
{
	// If we have a doorhandle we want to tap it before the door starts to open if the door wasn't
	// already interrupted
	if (m_Doorhandles.Num() > 0 && !m_bInterrupted)
	{
		// Relay the call to the handles, the OpenDoor() call will come back to us
		for (int i = 0; i < m_Doorhandles.Num(); i++)
		{
			CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
			if (handle == NULL) continue;

			handle->Tap();
		}

		if (bMaster)
		{
			TapPeers();
		}
	}
	else
	{
		// No handle present or interrupted, let's just proceed with our own open routine
		OpenDoor(bMaster);
	}
}

void CFrobDoor::OpenDoor(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Opening\r" );

	// Now pass the call to the base class, which will invoke PreOpen() and the other events
	CBinaryFrobMover::Open(bMaster);
}

void CFrobDoor::OnStartOpen(bool wasClosed, bool bMaster)
{
	// Call the base class first
	CBinaryFrobMover::OnStartOpen(wasClosed, bMaster);

	// We are actually opening, open the visportal too
	OpenPortal();

	// Update soundprop
	UpdateSoundLoss();

	// Clear the lock request flag in any case
	m_CloseOnLock = false;

	if (bMaster)
	{
		OpenPeers();
	}
}

void CFrobDoor::Close(bool bMaster)
{
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closing\r" );

	// Invoke the close method in any case
	CBinaryFrobMover::Close(bMaster);
}

void CFrobDoor::OnStartClose(bool wasOpen, bool bMaster)
{
	CBinaryFrobMover::OnStartClose(wasOpen, bMaster);

	if (bMaster)
	{
		ClosePeers();
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

	// Do we have a close request?
	if (m_CloseOnLock)
	{
		// Clear the flag, regardless what happens
		m_CloseOnLock = false;

		// Post a lock event in 250 msecs
		PostEventMS(&EV_TDM_FrobMover_Lock, 250);
	}
}

void CFrobDoor::OnInterrupt()
{
	// Clear the close request flag
	m_CloseOnLock = false;

	CBinaryFrobMover::OnInterrupt();
}

void CFrobDoor::OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity)
{
	// Clear the close request flag
	m_CloseOnLock = false;

	CBinaryFrobMover::OnTeamBlocked(blockedEntity, blockingEntity);
}

bool CFrobDoor::CanBeUsedBy(const CInventoryItemPtr& item, bool isFrobUse) 
{
	if (item == NULL) return false;

	assert(item->Category() != NULL);

	// TODO: Move this to idEntity to some sort of "usable_by_inv_category" list?
	const idStr& name = item->Category()->GetName();
	if (name == "Keys")
	{
		// Keys can always be used on doors
		// Exception: for "frob use" this only applies when the door is locked
		return (isFrobUse) ? IsLocked() : true;
	}
	else if (name == "Lockpicks")
	{
		// Lockpicks behave similar to keys
		return (isFrobUse) ? IsLocked() : true;
	}

	return idEntity::CanBeUsedBy(item, isFrobUse);
}

bool CFrobDoor::UseBy(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	if (item == NULL) return false;

	assert(item->Category() != NULL);

	// Retrieve the entity behind that item and reject NULL entities
	idEntity* itemEntity = item->GetItemEntity();
	if (itemEntity == NULL) return false;

	// Get the name of this inventory category
	const idStr& name = item->Category()->GetName();
	
	if (name == "Keys" && impulseState == EPressed) 
	{
		// Keys can be used on button PRESS event, let's see if the key matches
		if (m_UsedBy.FindIndex(itemEntity->name) != -1)
		{
			// If we're locked or closed, just toggle the lock. 
			if (IsLocked() || IsAtClosedPosition())
			{
				ToggleLock();
			}
			// If we're open, set a lock request and start closing.
			else
			{
				// Close the door and set the lock request to true
				m_CloseOnLock = true;
				ToggleOpen();
			}

			return true;
		}
		else
		{
			FrobMoverStartSound("snd_wrong_key");
			return false;
		}
	}
	else if (name == "Lockpicks")
	{
		// Lockpicks are different, we need to look at the button state
		// First we check if this item is a lockpick. It has to be of the toolclass lockpick
		// and the type must be set.
		char type = 0;
		idStr str = itemEntity->spawnArgs.GetString("type", "");

		if (str.Length() == 1)
		{
			type = str[0];

			ELockpickSoundsample sample;

			switch (impulseState)
			{
			case EPressed:	sample = LPSOUND_INIT; 
				break;
			case EReleased:	sample = LPSOUND_RELEASED;
				break;
			default:		sample = LPSOUND_REPEAT;
			};
			
			// Pass the call to the lockpick routine
			return ProcessLockpick(static_cast<int>(type), sample);
		}
		
		// Wrong type...
		return false;
	}

	return idEntity::UseBy(impulseState, item);
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

	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
		if (handle == NULL) continue;

		handle->SetFrobbed(val);
	}
}

bool CFrobDoor::IsFrobbed()
{
	// If the door has a handle and it is frobbed, then we are also considered 
	// to be frobbed.
	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
		if (handle == NULL) continue;

		if (handle->IsFrobbed())
		{
			return true;
		}
	}

	// None of the handles are frobbed
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
		if (m_Doorhandles.Num() > 0)
		{
			handle = m_Doorhandles[0].GetEntity();
		}
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

			idAngles angles = m_OriginalAngle + (m_PinRotationFraction * pin) + (m_SampleRotationFraction * sample);

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

bool CFrobDoor::ProcessLockpick(int cType, ELockpickSoundsample nSampleType)
{
	int sample_delay, pick_timeout;
	idStr oPickSound;
	char type = cType;
	int length = 0;
	idStr pick;
	idVec3 pos;
	idAngles angle;

	bool success = true;

	// Has the lock already been picked?
	if (m_FirstLockedPinIndex >= m_Pins.Num())
	{
		if (nSampleType == LPSOUND_INIT)
		{
			if (m_SoundTimerStarted <= 0)
			{
				oPickSound = "snd_lockpick_pick_wrong";
				PropPickSound(oPickSound, cType, LPSOUND_WRONG_LOCKPICK, 0, HANDLE_POS_ORIGINAL, -1, -1);
				DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] already picked\r", name.c_str());
			}
		}
		else if (nSampleType == LPSOUND_PIN_SAMPLE || nSampleType == LPSOUND_WRONG_LOCKPICK)
		{
			m_SoundTimerStarted--;
			if(m_SoundTimerStarted <= 0)
				m_SoundTimerStarted = 0;
		}

		success = false;
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
				oPickSound = "snd_lockpick_pick_wrong";
				PropPickSound(oPickSound, cType, LPSOUND_WRONG_LOCKPICK, 0, HANDLE_POS_ORIGINAL, -1, -1);
				DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pick attempt: %u/%u failed (len: %u).\r", m_FirstLockedPinIndex, m_SoundPinSampleIndex, length);
			}
			else
			{
				if (!(nSampleType == LPSOUND_INIT || nSampleType == LPSOUND_REPEAT))
					m_SoundTimerStarted--;
			}

			success = false;
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
				success = false;
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
					oPickSound = "snd_lockpick_lock_picked";
					PropPickSound(oPickSound, cType, LPSOUND_PIN_SUCCESS, 0, HANDLE_POS_ORIGINAL, 0, 0);
					Unlock(true);
					DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] successfully picked!\r", name.c_str());
					goto Quit;
				}
				else
				{
					oPickSound = "snd_lockpick_pin_success";
					m_SoundPinSampleIndex = 0;
					PropPickSound(oPickSound, cType, LPSOUND_PIN_SUCCESS, 0, HANDLE_POS_ORIGINAL, m_FirstLockedPinIndex, m_SoundPinSampleIndex);
					DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] successfully picked!\r", name.c_str());
				}
			}
			else
			{
				m_SoundPinSampleIndex = 0;
				oPickSound = "snd_lockpick_pin_fail";
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
	return success;
}

void CFrobDoor::PropPickSound(const idStr& pickSound, int cType, ELockpickSoundsample nSampleType, int time, EHandleReset nHandlePos, int PinIndex, int SampleIndex)
{
	m_SoundTimerStarted++;
	PropSoundDirect(pickSound, true, false );

	int length = FrobMoverStartSound(pickSound);

	if(PinIndex != -1)
	{
		SetHandlePosition(nHandlePos, length, PinIndex, SampleIndex);
	}

	PostEventMS(&EV_TDM_LockpickTimer, length+time, cType, nSampleType);
}

void CFrobDoor::OpenPeers()
{
	OpenClosePeers(true);
}

void CFrobDoor::ClosePeers()
{
	OpenClosePeers(false);
}

void CFrobDoor::OpenClosePeers(bool open)
{
	// Cycle through our "open peers" list and issue the call to them
	for (int i = 0; i < m_OpenPeers.Num(); i++)
	{
		const idStr& openPeer = m_OpenPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", openPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(openPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for OpenClose\r", openPeer.c_str());

			CFrobDoor* door = static_cast<CFrobDoor*>(ent);

			if (open)
			{
				if (door->IsAtClosedPosition())
				{
					// Other door is at closed position, let the handle tap
					door->Open(false);
				}
				else
				{
					// The other door is open or in an intermediate state, just call open
					door->OpenDoor(false);
				}
			}
			else
			{
				door->Close(false);
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] not spawned or not of class CFrobDoor\r", name.c_str(), openPeer.c_str());
		}
	}
}

void CFrobDoor::LockPeers()
{
	LockUnlockPeers(true);
}

void CFrobDoor::UnlockPeers()
{
	LockUnlockPeers(false);
}

void CFrobDoor::LockUnlockPeers(bool lock)
{
	// Go through the list and issue the call
	for (int i = 0; i < m_LockPeers.Num(); i++)
	{
		const idStr& lockPeer = m_LockPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", lockPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(lockPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", lockPeer.c_str());

			CFrobDoor* door = static_cast<CFrobDoor*>(ent);
			
			if (lock)
			{
				door->Lock(false);
			}
			else
			{
				door->Unlock(false);
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found or of wrong type\r", lockPeer.c_str());
		}
	}
}

void CFrobDoor::TapPeers()
{
	// In master mode, tap the handles of the master_open chain too
	// Cycle through our "open peers" list and issue the call to them
	for (int i = 0; i < m_OpenPeers.Num(); i++)
	{
		const idStr& openPeer = m_OpenPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", openPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(openPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for tapping\r", openPeer.c_str());
			CFrobDoor* other = static_cast<CFrobDoor*>(ent);

			if (other->GetDoorhandle() != NULL)
			{
				other->GetDoorhandle()->Tap();
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("[%s] Linked entity [%s] not spawned or not of class CFrobDoor\r", name.c_str(), openPeer.c_str());
		}
	}
}

void CFrobDoor::AddOpenPeer(const idStr& peerName)
{
	// Find the entity and check if it's actually a door
	idEntity* entity = gameLocal.FindEntity(peerName);

	if (entity != NULL && entity->IsType(CFrobDoor::Type))
	{
		m_OpenPeers.AddUnique(peerName);
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: open_peer %s added to local list (%u)\r", name.c_str(), peerName.c_str(), m_OpenPeers.Num());
	}
	else
	{
		DM_LOG(LC_ENTITY, LT_ERROR)LOGSTRING("open_peer [%s] not spawned or of wrong type.\r", peerName.c_str());
	}
}

void CFrobDoor::RemoveOpenPeer(const idStr& peerName)
{
	m_OpenPeers.Remove(peerName);
}

void CFrobDoor::AddLockPeer(const idStr& peerName)
{
	// Find the entity and check if it's actually a door
	idEntity* entity = gameLocal.FindEntity(peerName);

	if (entity != NULL && entity->IsType(CFrobDoor::Type))
	{
		m_LockPeers.AddUnique(peerName);
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: lock_peer %s added to local list (%u)\r", name.c_str(), peerName.c_str(), m_LockPeers.Num());
	}
	else
	{
		DM_LOG(LC_ENTITY, LT_ERROR)LOGSTRING("lock_peer [%s] not spawned or of wrong type.\r", peerName.c_str());
	}
}

void CFrobDoor::RemoveLockPeer(const idStr& peerName)
{
	m_LockPeers.Remove(peerName);
}

CFrobDoorHandle* CFrobDoor::GetDoorhandle()
{
	return (m_Doorhandles.Num() > 0) ? m_Doorhandles[0].GetEntity() : NULL;
}

void CFrobDoor::AddDoorhandle(CFrobDoorHandle* handle)
{
	// Store the pointer and the original position
	idEntityPtr<CFrobDoorHandle> handlePtr;
	handlePtr = handle;

	if (m_Doorhandles.FindIndex(handlePtr) != -1)
	{
		return; // handle is already known
	}

	m_Doorhandles.Append(handlePtr);

	m_OriginalPosition = handle->GetPhysics()->GetOrigin();
	m_OriginalAngle = handle->GetPhysics()->GetAxis().ToAngles();

	// Let the handle know about us
	handle->SetDoor(this);

	// Set up the frob peer relationship between the door and the handle
	m_FrobPeers.AddUnique(handle->name);
	handle->AddFrobPeer(name);
	handle->SetFrobable(m_bFrobable);
}

void CFrobDoor::AutoSetupDoorHandles()
{
	// Find a suitable teamchain member
	idEntity* part = FindMatchingTeamEntity(CFrobDoorHandle::Type);

	while (part != NULL)
	{
		// Found the handle, set it up
		AddDoorhandle(static_cast<CFrobDoorHandle*>(part));

		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: Auto-added door handle %s to local list.\r", name.c_str(), part->name.c_str());

		// Get the next handle
		part = FindMatchingTeamEntity(CFrobDoorHandle::Type, part);
	}

	for (int i = 0; i < m_Doorhandles.Num(); i++)
	{
		CFrobDoorHandle* handle = m_Doorhandles[i].GetEntity();
		if (handle == NULL) continue;

		// The first handle is the master, all others get their master flag set to FALSE
		handle->SetMaster(i == 0);
	}
}

void CFrobDoor::AutoSetupDoubleDoor()
{
	CFrobDoor* doubleDoor = m_DoubleDoor.GetEntity();

	if (doubleDoor != NULL)
	{
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%s: Auto-setting up %s as double door.\r", name.c_str(), doubleDoor->name.c_str());

		if (spawnArgs.GetBool("auto_setup_double_door_frob_peer", "0"))
		{
			// Add the door to our frob_peer set
			m_FrobPeers.AddUnique(doubleDoor->name);

			// Add ourselves to the double door as frob peer
			doubleDoor->AddFrobPeer(name);
			doubleDoor->SetFrobable(m_bFrobable);
		}

		if (spawnArgs.GetBool("auto_setup_double_door_open_peer", "0"))
		{
			// Add "self" to the peers of the other door
			doubleDoor->AddOpenPeer(name);
			// Now add the name of the other door to our own peer list
			AddOpenPeer(doubleDoor->name);
		}

		if (spawnArgs.GetBool("auto_setup_double_door_lock_peer", "0"))
		{
			// Add "self" to the peers of the other door
			doubleDoor->AddLockPeer(name);
			// Now add the name of the other door to our own peer list
			AddLockPeer(doubleDoor->name);
		}
	}
}

bool CFrobDoor::PreLock(bool bMaster)
{
	if (!CBinaryFrobMover::PreLock(bMaster))
	{
		return false;
	}

	// Go through the list and check whether the lock peers are closed
	for (int i = 0; i < m_LockPeers.Num(); i++)
	{
		const idStr& lockPeer = m_LockPeers[i];

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Trying linked entity [%s]\r", lockPeer.c_str());

		idEntity* ent = gameLocal.FindEntity(lockPeer);

		if (ent != NULL && ent->IsType(CFrobDoor::Type))
		{
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Calling linked entity [%s] for lock\r", lockPeer.c_str());

			CFrobDoor* door = static_cast<CFrobDoor*>(ent);
			
			if (!door->IsAtClosedPosition())
			{
				return false;
			}
		}
		else
		{
			DM_LOG(LC_FROBBING, LT_ERROR)LOGSTRING("Linked entity [%s] not found or of wrong type\r", lockPeer.c_str());
		}
	}

	return true;
}

int CFrobDoor::FrobMoverStartSound(const char* soundName)
{
	if (m_Doorhandles.Num() > 0)
	{
		CFrobDoorHandle* handle = m_Doorhandles[0].GetEntity();

		if (handle != NULL)
		{
			// Let the sound play from the first handle, but use the soundshader
			// as defined on this entity.
			idStr sound = spawnArgs.GetString(soundName, "");
			const idSoundShader* shader = declManager->FindSound(sound);

			int length = 0;
			handle->StartSoundShader(shader, SND_CHANNEL_ANY, 0, false, &length);
			return length;
		}
	}

	// No handle or NULL handle, just call the base class
	return CBinaryFrobMover::FrobMoverStartSound(soundName);
}

void CFrobDoor::Event_GetDoorhandle()
{
	idThread::ReturnEntity(m_Doorhandles.Num() > 0 ? m_Doorhandles[0].GetEntity() : NULL);
}

void CFrobDoor::Event_IsPickable()
{
	idThread::ReturnInt(m_Pickable);
}

void CFrobDoor::Event_OpenDoor(float master)
{
	OpenDoor(master != 0.0f ? true : false);
}
