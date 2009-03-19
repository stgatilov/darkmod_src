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
const idEventDef EV_TDM_Door_HandleLockRequest( "HandleLockRequest", NULL ); // used for periodic checks to lock the door once it is fully closed
const idEventDef EV_TDM_Door_IsPickable( "IsPickable", NULL, 'f' );
const idEventDef EV_TDM_Door_GetDoorhandle( "GetDoorhandle", NULL, 'e' );

// Internal events, no need to expose these to scripts
const idEventDef EV_TDM_LockpickSoundFinished("TDM_LockpickSoundFinished", "d"); // pass the next state as argument
const idEventDef EV_TDM_LockpickUpdateHandlePos("TDM_UpdateHandlePosition", NULL);

CLASS_DECLARATION( CBinaryFrobMover, CFrobDoor )
	EVENT( EV_TDM_Door_OpenDoor,			CFrobDoor::Event_OpenDoor)
	EVENT( EV_TDM_Door_HandleLockRequest,	CFrobDoor::Event_HandleLockRequest)
	EVENT( EV_TDM_Door_IsPickable,			CFrobDoor::Event_IsPickable)
	EVENT( EV_TDM_Door_GetDoorhandle,		CFrobDoor::Event_GetDoorhandle)
	EVENT( EV_TDM_LockpickSoundFinished,	CFrobDoor::Event_LockpickSoundFinished)
	EVENT( EV_TDM_LockpickUpdateHandlePos,	CFrobDoor::Event_UpdateHandlePosition)
END_CLASS

static const char* StateNames[] =
{
	"UNLOCKED",
	"LOCKED",
	"LOCKPICKING_STARTED",
	"ADVANCE_TO_NEXT_SAMPLE",
	"PIN_SAMPLE",
	"PIN_SAMPLE_SWEETSPOT",
	"PIN_DELAY",
	"AFTER_PIN_DELAY",
	"WRONG_LOCKPICK_SOUND",
	"PIN_SUCCESS",
	"PIN_FAILED",
	"LOCK_SUCCESS",
	"PICKED",
	"NUM_LPSTATES"
};

#define LOCK_REQUEST_DELAY 250 // msecs before a door locks itself after closing (if the preference is set appropriately)

CFrobDoor::CFrobDoor()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
	m_LockpickState = NUM_LPSTATES;
	m_FailedLockpickRounds = 0;
	m_FrobActionScript = "frob_door";
	m_Pickable = true;
	m_CloseOnLock = false;
	m_DoubleDoor = NULL;
	m_FirstLockedPinIndex = 0;
	m_SoundPinSampleIndex = 0;
	m_SoundTimerStarted = 0;
}

CFrobDoor::~CFrobDoor()
{
	ClearDoorTravelFlag();
}

void CFrobDoor::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(static_cast<int>(m_LockpickState));
	savefile->WriteInt(m_FailedLockpickRounds);

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
		// Write the pattern strings
		const idStringList& stringList = m_Pins[i].pattern;

		savefile->WriteInt(stringList.Num());
		for (int j = 0; j < stringList.Num(); j++)
		{
			savefile->WriteString(stringList[j]);
		}

		// Write the positions
		const idList<int>& positions = m_Pins[i].positions;

		savefile->WriteInt(positions.Num());
		for (int j = 0; j < positions.Num(); j++)
		{
			savefile->WriteInt(positions[j]);
		}
	}

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
}

void CFrobDoor::Restore( idRestoreGame *savefile )
{
	int temp;
	savefile->ReadInt(temp);
	assert(temp >= 0 && temp < NUM_LPSTATES);
	m_LockpickState = static_cast<ELockpickState>(temp);

	savefile->ReadInt(m_FailedLockpickRounds);

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
		// Read the pattern
		savefile->ReadInt(num);
		m_Pins[i].pattern.SetNum(num);

		for (int j = 0; j < num; j++)
		{
			savefile->ReadString( m_Pins[i].pattern[j] );
		}

		// Read the positions
		savefile->ReadInt(num);
		m_Pins[i].positions.SetNum(num);

		for (int j = 0; j < num; j++)
		{
			savefile->ReadInt( m_Pins[i].positions[j] );
		}
	}

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

	SetDoorTravelFlag();
}

void CFrobDoor::Spawn()
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

			PinInfo& pin = m_Pins.Alloc();

			pin.pattern = CreatePinPattern(lockPins[i] - 0x030, b, MAX_PIN_CLICKS, 2, head);

			if (cv_lp_pawlow.GetBool() == false)
			{
				pin.pattern.Insert("snd_lockpick_pin_sweetspot");
			}
			else
			{
				pin.pattern.Append("snd_lockpick_pin_sweetspot");
			}
			
			// Calculate the jiggle position list for this pattern
			// Add one extra position for the delay after the pattern
			pin.positions.SetNum(pin.pattern.Num() + 1);

			// Fill in a linear pattern 
			for (int j = 0; j <= pin.pattern.Num(); ++j)
			{
				pin.positions[j] = j;
			}

			// Random jiggling requires a part of the list to be re-"sorted"
			if (cv_lp_randomize.GetBool() && pin.positions.Num() > 2)
			{
				// Copy the existing pattern
				idList<int> candidates(pin.positions);
				
				// Remove the first and last indices from the candidates, these stay fixed
				candidates.RemoveIndex(0);
				candidates.RemoveIndex(candidates.Num() - 1);

				// Candidates are now in the range [1 .. size(pattern) - 1]

				for (int j = 1; candidates.Num() > 0; ++j)
				{
					// Choose a random candidate and move it to the position list
					int randPos = gameLocal.random.RandomInt(candidates.Num());

					pin.positions[j] = candidates[randPos];

					candidates.RemoveIndex(randPos);
				}
			}
		}
	}

	m_Pickable = spawnArgs.GetBool("pickable");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] pickable (%u)\r", name.c_str(), m_Pickable);

	//TODO: Add portal/door pair to soundprop data here, 
	//	replacing the old way in sndPropLoader

	// Initialise the lockpick state
	m_LockpickState = (IsLocked()) ? LOCKED : UNLOCKED;
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

	SetDoorTravelFlag();
}

void CFrobDoor::SetDoorTravelFlag()
{
	// Flag the AAS areas the door is located in with door travel flag
	for (int i = 0; i < gameLocal.NumAAS(); i++)
	{
		idAAS*	aas = gameLocal.GetAAS(i);
		if (aas == NULL)
		{
			continue;
		}
		
		int areaNum = GetAASArea(aas);
		aas->SetAreaTravelFlag(areaNum, TFL_DOOR);
		aas->ReferenceDoor(this, areaNum);
	}
}

void CFrobDoor::ClearDoorTravelFlag()
{
	// Flag the AAS areas the door is located in with door travel flag
	for (int i = 0; i < gameLocal.NumAAS(); i++)
	{
		idAAS*	aas = gameLocal.GetAAS(i);
		if (aas == NULL)
		{
			continue;
		}
		
		int areaNum = GetAASArea(aas);
		if (areaNum > 0)
		{
			aas->RemoveAreaTravelFlag(areaNum, TFL_DOOR);
			aas->DeReferenceDoor(this, areaNum);
		}
		else
		{
			gameLocal.Warning("Door %s is not within a valid AAS area", name.c_str());
		}
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
	m_FailedLockpickRounds = 0;

	m_LockpickState = LOCKED;

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

	m_LockpickState = UNLOCKED;

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
	CancelEvents(&EV_TDM_Door_HandleLockRequest);

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
		
		// Post a lock request in LOCK_REQUEST_DELAY msecs
		PostEventMS(&EV_TDM_Door_HandleLockRequest, LOCK_REQUEST_DELAY);
	}
}

void CFrobDoor::OnInterrupt()
{
	// Clear the close request flag
	m_CloseOnLock = false;
	CancelEvents(&EV_TDM_Door_HandleLockRequest);

	CBinaryFrobMover::OnInterrupt();
}

void CFrobDoor::OnTeamBlocked(idEntity* blockedEntity, idEntity* blockingEntity)
{
	// Clear the close request flag
	m_CloseOnLock = false;
	CancelEvents(&EV_TDM_Door_HandleLockRequest);

	CBinaryFrobMover::OnTeamBlocked(blockedEntity, blockingEntity);
}

bool CFrobDoor::CanBeUsedBy(const CInventoryItemPtr& item, const bool isFrobUse) 
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
		if (!IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobDoor %s is not pickable\r", name.c_str());
			return false;
		}

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
		if (!IsPickable())
		{
			// Lock is not pickable
			DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("FrobDoor %s is not pickable\r", name.c_str());
			return false;
		}

		// Lockpicks are different, we need to look at the button state
		// First we check if this item is a lockpick. It has to be of the toolclass lockpick
		// and the type must be set.
		idStr str = itemEntity->spawnArgs.GetString("type", "");

		if (str.Length() == 1)
		{
			// Pass the call to the lockpick routine
			return ProcessLockpickImpulse(impulseState, static_cast<int>(str[0]));
		}
		else
		{
			gameLocal.Warning("Wrong 'type' spawnarg for lockpicking on item %s, must be a single character.", itemEntity->name.c_str());
			return false;
		}
	}

	return idEntity::UseBy(impulseState, item);
}

void CFrobDoor::UpdateSoundLoss()
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

void CFrobDoor::FindDoubleDoor()
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

	if (val == false)
	{
		// Reset the lockpick fail counter when entity is losing frob focus
		m_FailedLockpickRounds = 0;
		// And cancel any pending events
		CancelEvents(&EV_TDM_LockpickSoundFinished);
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

idStringList CFrobDoor::CreatePinPattern(int clicks, int baseCount, int maxCount, int strNumLen, idStr &str)
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
	idStringList returnValue;

	idStr head = va(str+"%%0%uu", strNumLen);

	for (int i = 0; i < clicks; i++)
	{
		// Choose a different random number generator every other frame
		int r = (i % 2) ? gameLocal.random.RandomInt(maxCount) : rnd.IRandom(0, maxCount);

		idStr click = va(head, r);
		returnValue.Append(click);

		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("PinPattern %u : %s\r", i, click.c_str());
	}

	return returnValue;
}

void CFrobDoor::UpdateHandlePosition()
{
	// Calculate the fraction based on the current pin/sample state
	float fraction = CalculateHandleMoveFraction();

	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText3", idStr(fraction));
	}

	// Tell the doorhandles to update their position
	for (int i = 0; i < m_Doorhandles.Num(); ++i)
	{
		m_Doorhandles[i].GetEntity()->SetFractionalPosition(fraction);
	}
}

float CFrobDoor::CalculateHandleMoveFraction()
{
	if (m_LockpickState == UNLOCKED || m_LockpickState == PICKED || m_Pins.Num() == 0)
	{
		// unlocked handles or ones without lock pins are at the starting position
		return 0.0f; 
	}

	// Each pin moves the handle by an equal amount
	float pinStep = 1.0f / m_Pins.Num();

	// Calculate the coarse move fraction, depending on the number of unlocked pins
	float fraction = m_FirstLockedPinIndex * pinStep;

	// Sanity-check the pin index before using it as array index
	if (m_FirstLockedPinIndex >= m_Pins.Num()) 
	{
		return fraction;
	}

	// Calculate the fine fraction, based on the current sample number
	const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;

	// Sanity-check the pattern size
	if (pattern.Num() == 0) return fraction;

	float sampleStep = pinStep / pattern.Num();

	int curSampleIndex = m_Pins[m_FirstLockedPinIndex].positions[m_SoundPinSampleIndex];

	// During the delay, the handle is using the last position index
	if (m_LockpickState == PIN_DELAY) 
	{
		curSampleIndex = m_Pins[m_FirstLockedPinIndex].positions.Num() - 1;
	}

	// Add the fine movement fraction
	fraction += curSampleIndex * sampleStep;

	// Clamp the fraction to reasonable values
	fraction = idMath::ClampFloat(0.0f, 1.0f, fraction);

	return fraction;
}

void CFrobDoor::Event_UpdateHandlePosition()
{
	UpdateHandlePosition();
}

void CFrobDoor::OnLockpickPinSuccess()
{
	// Pin was picked, so we try to advance to the next pin.
	m_FirstLockedPinIndex++;

	// If it was the last pin, the user successfully picked the whole lock.
	if (m_FirstLockedPinIndex >= m_Pins.Num())
	{
		if (cv_lp_debug_hud.GetBool())
		{
			idPlayer* player = gameLocal.GetLocalPlayer();
			player->SetGuiString(player->lockpickHUD, "StatusText1", "Lock Successfully picked");
		}

		m_FirstLockedPinIndex = m_Pins.Num();
		m_FailedLockpickRounds = 0;

		// Switch to PICKED state after this sound
		m_LockpickState = LOCK_SUCCESS;

		PropPickSound("snd_lockpick_lock_picked", PICKED);
		//PropPickSound("snd_lockpick_lock_picked", cType, LPSOUND_PIN_SUCCESS, 0, HANDLE_POS_ORIGINAL, 0, 0);

		Unlock(true); // unlock this in master mode

		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] successfully picked!\r", name.c_str());
	}
	else
	{
		if (cv_lp_debug_hud.GetBool())
		{
			idPlayer* player = gameLocal.GetLocalPlayer();
			player->SetGuiString(player->lockpickHUD, "StatusText1", "Pin Successfully picked");
		}

		m_LockpickState = PIN_SUCCESS;
		m_SoundPinSampleIndex = 0;
		m_FailedLockpickRounds = 0;

		// Fall back to LOCKED state after the sound
		PropPickSound("snd_lockpick_pin_success", LOCKED);

		//PropPickSound("snd_lockpick_pin_success", cType, LPSOUND_PIN_SUCCESS, 0, HANDLE_POS_ORIGINAL, m_FirstLockedPinIndex, m_SoundPinSampleIndex);
		DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Door [%s] successfully picked!\r", name.c_str());
	}
}

void CFrobDoor::OnLockpickPinFailure()
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText1", "Pin Failed");
	}

	m_LockpickState = PIN_FAILED;

	m_SoundPinSampleIndex = 0;

	// Fall back to LOCKED state after playing the sound
	PropPickSound("snd_lockpick_pin_fail", LOCKED);

	//PropPickSound("snd_lockpick_pin_fail", cType, LPSOUND_PIN_FAILED, 0, HANDLE_POS_SAMPLE, m_FirstLockedPinIndex, m_SoundPinSampleIndex);
	DM_LOG(LC_LOCKPICK, LT_DEBUG)LOGSTRING("Pick attempt: %u/%u failed.\r", m_FirstLockedPinIndex, m_SoundPinSampleIndex);
}

void CFrobDoor::PropPickSound(const idStr& picksound, ELockpickState nextState, int additionalDelay)
{
	m_SoundTimerStarted++;

	PropSoundDirect(picksound, true, false);

	int length = FrobMoverStartSound(picksound);

	// Post the sound finished event
	PostEventMS(&EV_TDM_LockpickSoundFinished, length + additionalDelay, static_cast<int>(nextState));
}

void CFrobDoor::Event_LockpickSoundFinished(ELockpickState nextState) 
{
	m_SoundTimerStarted--;

	if (m_SoundTimerStarted < 0) 
	{
		m_SoundTimerStarted = 0;
	}

	// Set the state to the one that was requested
	m_LockpickState = nextState;
}

bool CFrobDoor::CheckLockpickType(int type)
{
	// Now check if the pick is of the correct type
	idStr pick = spawnArgs.GetString("lock_picktype");

	// Sanity-check the index
	if (m_FirstLockedPinIndex < 0 || m_FirstLockedPinIndex >= pick.Length()) 
	{
		// Incorrect indices
		return false;
	}

	return (pick[m_FirstLockedPinIndex] == '*' || pick[m_FirstLockedPinIndex] == type);
}

bool CFrobDoor::ProcessLockpickPress(int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText6", "Button Pressed");
	}

	// Check if we're still playing a sound
	if (m_SoundTimerStarted > 0) return false; // busy

	switch (m_LockpickState)
	{
		case LOCKED:	// Lockpicking not yet started, do so now
		{
			if (CheckLockpickType(type))
			{
				// Start the first sample
				m_LockpickState = LOCKPICKING_STARTED;
				return true;
			}
			else
			{
				if (cv_lp_debug_hud.GetBool())
				{
					idPlayer* player = gameLocal.GetLocalPlayer();
					player->SetGuiString(player->lockpickHUD, "StatusText1", "Wrong Lockpick Type");
				}

				// Fall back to locked after playing the sound
				m_LockpickState = WRONG_LOCKPICK_SOUND;
				PropPickSound("snd_lockpick_pick_wrong", LOCKED, 1000);
				return false;
			}
		}
		case PICKED:	// Already picked or...
		case UNLOCKED:	// ...lockpicking not possible => wrong lockpick sound
		{
			// Play wrong lockpick sound
			m_LockpickState = WRONG_LOCKPICK_SOUND;
			// Fall back to the same state as we're now
			PropPickSound("snd_lockpick_pick_wrong", m_LockpickState, 1000);
			return false;
		}
		case PIN_SAMPLE:
		case ADVANCE_TO_NEXT_SAMPLE:
			// If we encounter a lockpick press during these, reset to start
			m_LockpickState = LOCKPICKING_STARTED;
			return true;
		// Ignore button press events on all other events (while playing sounds, for instance)
		default:
		{
			return true;
		}
	};
}

bool CFrobDoor::ProcessLockpickRepeat(int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText6", "Button Held Down");
	}

	// Do this in any case
	UpdateHandlePosition();

	// Check if we're still playing a sound
	if (m_SoundTimerStarted > 0) return false; // busy

	bool success = false;

	switch (m_LockpickState)
	{
		case UNLOCKED: // ignore key-repeat events for unlocked doors
		{
			success = false;
			break;
		}
		case LOCKED:	
		{
			// Lockpicking not yet started, start it now
			if (CheckLockpickType(type))
			{
				// Start the first sample
				m_LockpickState = LOCKPICKING_STARTED;
				m_FailedLockpickRounds = 0;
				success = true;
			}
			else
			{
				// Fall back to locked after playing the sound
				m_LockpickState = WRONG_LOCKPICK_SOUND;
				PropPickSound("snd_lockpick_pick_wrong", LOCKED, 500);
				success = false;
			}

			break;
		}
		case LOCKPICKING_STARTED:
		{
			// Initialise the lockpick sample index to -1 and fall through to ADVANCE_TO_NEXT_SAMPLE
			m_SoundPinSampleIndex = -1;
			m_LockpickState = ADVANCE_TO_NEXT_SAMPLE;

			// Fall through to ADVANCE_TO_NEXT_SAMPLE
		}
		case ADVANCE_TO_NEXT_SAMPLE:
		{
			m_SoundPinSampleIndex++;

			const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;

			if (m_SoundPinSampleIndex >= pattern.Num())
			{
				// Switch to the delay after the last sample sound
				m_LockpickState = PIN_DELAY;

				// Use a "nosound" to simulate the silence - then switch back to the first sample
				PropPickSound("nosound", AFTER_PIN_DELAY, cv_lp_pick_timeout.GetInteger());

				success = true;
				break;
			}

			// There are more samples to play, are we in pavlov mode and hitting the last sample?
			// Check if this was the last sample or the last but one
			if (m_SoundPinSampleIndex == pattern.Num() - 1 && cv_lp_pawlow.GetBool())
			{
				// greebo: In Pavlov mode, we're entering the hotspot with the beginning
				// of the last sample in the pattern (which is the sweetspot click sound)
				m_LockpickState = PIN_SAMPLE_SWEETSPOT;
			}
			else 
			{
				// Not the last sample, or not in pavlov mode, proceed
				m_LockpickState = PIN_SAMPLE;
			}

			// Fall through
		}
		// Fire the sample sounds
		case PIN_SAMPLE:
		case PIN_SAMPLE_SWEETSPOT:
		{
			// Play the current sample and fall back to ADVANCE_TO_NEXT_SAMPLE
			const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;

			// Sanity-check the sample index
			if (m_SoundPinSampleIndex >= pattern.Num())
			{
				m_LockpickState = ADVANCE_TO_NEXT_SAMPLE; // wrap around
				success = true;
				break;
			}

			// Get the pick sound and start playing
			const idStr& pickSound = pattern[m_SoundPinSampleIndex];

			// Pad the sound with a sample delay
			PropPickSound(pickSound, ADVANCE_TO_NEXT_SAMPLE, cv_lp_sample_delay.GetInteger());

			success = true;
			break;
		}
		case PIN_DELAY:
		{
			// We're in delay mode, so ignore this one
			// Either the user hits the hotspot when not in pavlov mode
			// or the "finished sound" event fires and we're back to LOCKPICKING_STARTED
			success = true;
			break;
		}
		case AFTER_PIN_DELAY:
			// The player didn't succeed to get that pin, increase the failed count
			m_FailedLockpickRounds++;

			// greebo: Check if auto-pick should kick in
			if (cv_lp_auto_pick.GetBool() && cv_lp_max_pick_attempts.GetInteger() > 0 && 
				m_FailedLockpickRounds > cv_lp_max_pick_attempts.GetInteger())
			{
				OnLockpickPinSuccess();
				success = true;
				break;
			}

			// Goto start
			m_LockpickState = LOCKPICKING_STARTED;

			break;
		case PICKED:	// Already picked
		{
			success = false;
			break;
		}
		// Ignore button press events on all other events (while playing sounds, for instance)
		default:
		{
			success = true;
			break;
		}
	};

	if (cv_lp_debug_hud.GetBool()) 
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText5", StateNames[m_LockpickState]);
	}

	return success;
}

bool CFrobDoor::ProcessLockpickRelease(int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetGuiString(player->lockpickHUD, "StatusText6", "Button Released");
	}
	
	// Cancel all previous events on release
	CancelEvents(&EV_TDM_LockpickSoundFinished);
	m_SoundTimerStarted = 0;

	// Check if we're in the "hot spot" of the lock pick sequence
	if (LockpickHotspotActive())
	{
		// Success
		OnLockpickPinSuccess();
	}
	else 
	{
		// Failure
		OnLockpickPinFailure();
		// greebo: Reset the lockpick rounds on failure, the player needs to hold
		// down the key to get the auto-pick feature to kick in
		m_FailedLockpickRounds = 0;
	}

	return true;
}

void CFrobDoor::UpdateLockpickHUD()
{
	idPlayer* player = gameLocal.GetLocalPlayer();

	player->SetGuiString(player->lockpickHUD, "StatusText2", idStr("Failed lockpick rounds: ") + idStr(m_FailedLockpickRounds));
	player->SetGuiString(player->lockpickHUD, "StatusText4", (idStr("Sounds started: ") + idStr(m_SoundTimerStarted)).c_str());
	player->SetGuiString(player->lockpickHUD, "StatusText5", StateNames[m_LockpickState]);
	player->CallGui(player->lockpickHUD, "OnLockPickProcess");

	idStr patternText = "Current Pattern: ";
	patternText += idStr(m_FirstLockedPinIndex + 1) + idStr(" of ") + idStr(m_Pins.Num());
	
	const idStringList& pattern = m_Pins[m_FirstLockedPinIndex].pattern;
	for (int i = 0; i < pattern.Num(); ++i)
	{
		idStr p = pattern[i];
		p.StripLeadingOnce("snd_lockpick_pin_");

		player->SetGuiString(player->lockpickHUD, "Sample" + idStr(i+1), p);

		float opacity = (i < m_SoundPinSampleIndex) ? 0.2f : 0.6f;

		if (i == m_SoundPinSampleIndex) opacity = 1;

		player->SetGuiFloat(player->lockpickHUD, "SampleOpacity" + idStr(i+1), opacity);
		player->SetGuiInt(player->lockpickHUD, "SampleBorder" + idStr(i+1), (i == m_SoundPinSampleIndex) ? 1 : 0);
	}

	player->SetGuiString(player->lockpickHUD, "PatternText", patternText);
}

bool CFrobDoor::ProcessLockpickImpulse(EImpulseState impulseState, int type)
{
	if (cv_lp_debug_hud.GetBool())
	{
		UpdateLockpickHUD();
	}
	
	switch (impulseState)
	{
		case EPressed:			// just pressed
		{
			return ProcessLockpickPress(type);
		}
		case ERepeat:			// held down
		{
			return ProcessLockpickRepeat(type);
		}
		case EReleased:			// just released
		{
			return ProcessLockpickRelease(type);
		}
		default:
			// Error
			DM_LOG(LC_LOCKPICK, LT_ERROR)LOGSTRING("Unhandled impulse state in ProcessLockpick.\r");
			return false;
	};
}

bool CFrobDoor::LockpickHotspotActive()
{
	if (cv_lp_pawlow.GetBool()) // pavlov mode
	{
		return (m_LockpickState == PIN_SAMPLE_SWEETSPOT);
	}
	else // pattern mode
	{
		return (m_LockpickState == PIN_DELAY);
	}
}

void CFrobDoor::AttackAction(idPlayer* player)
{
	if (cv_lp_debug_hud.GetBool())
	{
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->CallGui(player->lockpickHUD, "OnAttackButtonPress");
	}

	// Cancel all previous events on attack button hit
	CancelEvents(&EV_TDM_LockpickSoundFinished);
	m_SoundTimerStarted = 0;

	// Check if we're in the "hot spot" of the lock pick sequence
	if (LockpickHotspotActive())
	{
		// Success
		OnLockpickPinSuccess();
	}
	else 
	{
		// Failure
		OnLockpickPinFailure();
	}
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

	// Allow closing if all lock peers are at their closed position
	return AllLockPeersAtClosedPosition();
}

bool CFrobDoor::AllLockPeersAtClosedPosition()
{
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

void CFrobDoor::Event_HandleLockRequest()
{
	// Check if all the peers are at their "closed" position, if yes: lock the door(s), if no: postpone the event
	if (AllLockPeersAtClosedPosition())
	{
		// Yes, all lock peers are at their "closed" position, lock ourselves and all peers
		Lock(true);
	}
	else
	{
		// One or more peers are not at their closed position (yet), postpone the event
		PostEventMS(&EV_TDM_Door_HandleLockRequest, LOCK_REQUEST_DELAY);
	}
}
