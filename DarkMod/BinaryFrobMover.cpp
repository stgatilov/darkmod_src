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
#include "sndProp.h"

//===============================================================================
//CBinaryFrobMover
//===============================================================================

const idEventDef EV_TDM_Door_Open( "Open", "f" );
const idEventDef EV_TDM_Door_Close( "Close", "f" );
const idEventDef EV_TDM_Door_ToggleOpen( "ToggleOpen", NULL );
const idEventDef EV_TDM_Door_Lock( "Lock", "f" );
const idEventDef EV_TDM_Door_Unlock( "Unlock", "f" );
const idEventDef EV_TDM_Door_ToggleLock( "ToggleLock", NULL );
const idEventDef EV_TDM_Door_GetOpen( "GetOpen", NULL, 'f' );
const idEventDef EV_TDM_Door_GetLock( "GetLock", NULL, 'f' );

CLASS_DECLARATION( idMover, CBinaryFrobMover )
	EVENT( EV_TDM_Door_Open,				CBinaryFrobMover::Open)
	EVENT( EV_TDM_Door_Close,				CBinaryFrobMover::Close)
	EVENT( EV_TDM_Door_ToggleOpen,			CBinaryFrobMover::ToggleOpen)
	EVENT( EV_TDM_Door_Lock,				CBinaryFrobMover::Lock)
	EVENT( EV_TDM_Door_Unlock,				CBinaryFrobMover::Unlock)
	EVENT( EV_TDM_Door_ToggleLock,			CBinaryFrobMover::ToggleLock)
	EVENT( EV_TDM_Door_GetOpen,				CBinaryFrobMover::GetOpen)
	EVENT( EV_TDM_Door_GetLock,				CBinaryFrobMover::GetLock)
	EVENT( EV_Activate,						CBinaryFrobMover::Event_Activate )
END_CLASS


CBinaryFrobMover::CBinaryFrobMover(void)
: idMover()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);
	m_FrobActionScript = "frob_binary_mover";
	m_Open = false;
	m_Locked = false;
	m_bInterruptable = true;
	m_bInterrupted = false;
	m_bIntentOpen = false;
	m_StateChange = false;
	m_Rotating = false;
	m_Translating = false;
	m_TransSpeed = 0;
	m_ImpulseThreshCloseSq = 0;
	m_ImpulseThreshOpenSq = 0;
	m_vImpulseDirOpen.Zero();
	m_vImpulseDirClose.Zero();
}

void CBinaryFrobMover::Save(idSaveGame *savefile) const
{
}

void CBinaryFrobMover::Restore( idRestoreGame *savefile )
{
}

void CBinaryFrobMover::WriteToSnapshot( idBitMsgDelta &msg ) const
{
}

void CBinaryFrobMover::ReadFromSnapshot( const idBitMsgDelta &msg )
{
}

void CBinaryFrobMover::Spawn( void )
{
	idStr str;
	idMover::Spawn();
	idAngles tempAngle, partialAngle;

	LoadTDMSettings();

	m_Rotate = spawnArgs.GetAngles("rotate", "0 90 0");

	m_Open = spawnArgs.GetBool("open");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] open (%u)\r", name.c_str(), m_Open);
	partialAngle = spawnArgs.GetAngles("start_rotate", "0 0 0");

	m_Locked = spawnArgs.GetBool("locked");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] locked (%u)\r", name.c_str(), m_Locked);

	m_bInterruptable = spawnArgs.GetBool("interruptable");
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("[%s] interruptable (%u)\r", name.c_str(), m_bInterruptable);

	// log if visportal was found
	if( areaPortal > 0 )
		DM_LOG(LC_SYSTEM, LT_DEBUG)LOGSTRING("FrobDoor [%s] found portal handle %d on spawn \r", name.c_str(), areaPortal);

	physicsObj.GetLocalAngles( tempAngle );

	// Original starting position of the door in case it is a sliding door.
	// Add the initial position offset in case the mapper makes the door start out inbetween states
	m_StartPos = physicsObj.GetOrigin() + spawnArgs.GetVector("start_position", "0 0 0");

	// m_Translation is the vector between start position and end position
	spawnArgs.GetVector("translate", "0 0 0", m_Translation);
	spawnArgs.GetFloat( "translate_speed", "0", m_TransSpeed );

	// set up physics impulse behavior
	spawnArgs.GetFloat("impulse_thresh_open", "0", m_ImpulseThreshOpenSq );
	spawnArgs.GetFloat("impulse_thresh_close", "0", m_ImpulseThreshCloseSq );
	m_ImpulseThreshOpenSq *= m_ImpulseThreshOpenSq;
	m_ImpulseThreshCloseSq *= m_ImpulseThreshCloseSq;
	spawnArgs.GetVector("impulse_dir_open", "0 0 0", m_vImpulseDirOpen );
	spawnArgs.GetVector("impulse_dir_close", "0 0 0", m_vImpulseDirClose );
	if( m_vImpulseDirOpen.LengthSqr() > 0 )
		m_vImpulseDirOpen.Normalize();
	if( m_vImpulseDirClose.LengthSqr() > 0 )
		m_vImpulseDirClose.Normalize();

	if(!m_Open) 
	{
		// Door starts _completely_ closed
		Event_ClosePortal();

		m_ClosedAngles = tempAngle;
		m_OpenAngles = tempAngle + m_Rotate;
	}
	else
	{
		m_ClosedAngles = tempAngle - partialAngle;
		m_OpenAngles = tempAngle + m_Rotate - partialAngle;
	}

	// set the first intent according to the initial doorstate
	m_bIntentOpen = !m_Open;

	// Let the mapper override the initial frob intent on a partially opened door
	if( spawnArgs.GetBool("first_frob_open") )
		m_bIntentOpen = true;
}

void CBinaryFrobMover::Lock(bool bMaster)
{
	StartSound("snd_unlock", SND_CHANNEL_ANY, 0, false, NULL);
	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("[%s] Door is locked\r", name.c_str());
	m_Locked = true;
	CallStateScript();
}

void CBinaryFrobMover::Unlock(bool bMaster)
{
	StartSound("snd_unlock", SND_CHANNEL_ANY, 0, false, NULL);

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("[%s] Door is unlocked\r", name.c_str());
	m_Locked = false;

	ToggleOpen();
}

void CBinaryFrobMover::ToggleLock(void)
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

void CBinaryFrobMover::Open(bool bMaster)
{
	idAngles tempAng;

	// If the door is already open, we don't have anything to do. :)
	if(m_Open == true && !m_bInterrupted)
		return;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Opening\r" );

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
		idVec3 tv3 = (m_StartPos +  m_Translation);
		Event_MoveToPos( tv3 );
	}
}

void CBinaryFrobMover::Close(bool bMaster)
{
	idAngles tempAng;

	// If the door is already closed, we don't have anything to do. :)
	if(m_Open == false)
		return;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closing\r" );

	physicsObj.GetLocalAngles( tempAng );
	
	m_StateChange = true;
	m_Rotating = true;
	m_Translating = true;

	Event_RotateOnce( (m_ClosedAngles - tempAng).Normalize180() );
	
	if( m_TransSpeed )
			Event_SetMoveSpeed( m_TransSpeed );

	Event_MoveToPos(m_StartPos);
}

void CBinaryFrobMover::ToggleOpen(void)
{
	// Check if the door is stopped.
//	if( physicsObj.GetAngularExtrapolationType() == EXTRAPOLATION_NONE )
	if( !m_Rotating && !m_Translating )
	{
//		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Was stationary on frobbing\r" );

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

//	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Interrupted!  Stopping door\r" );

	// Otherwise, door is moving.  Stop it if it is interruptable
	if(m_bInterruptable)
	{
		m_bInterrupted = true;
		Event_StopRotating();
		Event_StopMoving();

		// reverse the intent
		m_bIntentOpen = !m_bIntentOpen;
	}

Quit:
	return;
}

void CBinaryFrobMover::DoneMoving(void)
{
	idMover::DoneMoving();
    m_Translating = false;

	DoneStateChange();
}


void CBinaryFrobMover::DoneRotating(void)
{
	idMover::DoneRotating();
    m_Rotating = false;

	DoneStateChange();
}

void CBinaryFrobMover::DoneStateChange(void)
{
	bool CallScript = false;

//	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Done rotating\r" );

	// Ignore it if we already did it.
	if(m_StateChange == false)
		goto Quit;

    if(m_Rotating == true || m_Translating == true)
        goto Quit;

	// if the door is not completely opened or closed, do nothing
	if( m_bInterrupted )
		goto Quit;

	m_StateChange = false;
	CallScript = true;

	// door has completely closed
	if(!m_bIntentOpen)
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Closed completely\r" );

		m_bIntentOpen = true;
		m_Open = false;

		// play the closing sound when the door closes completely
		StartSound( "snd_close", SND_CHANNEL_ANY, 0, false, NULL );
		
		// close visportal
		ClosePortal();
	}
	else	// door has completely opened
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Opened completely\r" );
		m_Open = true;
		m_bIntentOpen = false;
	}

Quit:
	if(CallScript == true)
		CallStateScript();

	return;
}

void CBinaryFrobMover::CallStateScript(void)
{
	idStr str;
	if(spawnArgs.GetString("state_change_callback", "", str))
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobDoor: Callscript Open: %d  Locked: %d   Interrupt: %d\r", m_Open, m_Locked, m_bInterrupted);
		CallScriptFunctionArgs(str.c_str(), true, 0, "ebbb", this, m_Open, m_Locked, m_bInterrupted);
	}
}

void CBinaryFrobMover::GetOpen(void)
{
	idThread::ReturnInt(m_Open);
}

void CBinaryFrobMover::GetLock(void)
{
	idThread::ReturnInt(m_Locked);
}
void CBinaryFrobMover::ClosePortal(void)
{
	Event_ClosePortal();
}

void CBinaryFrobMover::Event_Activate( idEntity *activator ) 
{
	ToggleOpen();
}

void CBinaryFrobMover::ApplyImpulse(idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse)
{
	idVec3 SwitchDir;
	float SwitchThreshSq(0), ImpulseMagSq(0);

	if( (m_Open && (m_ImpulseThreshCloseSq > 0)) || (!m_Open && (m_ImpulseThreshOpenSq > 0)) )
	{
		if( m_Open )
		{
			SwitchDir = m_vImpulseDirClose;
			SwitchThreshSq = m_ImpulseThreshCloseSq;
		}
		else
		{
			SwitchDir = m_vImpulseDirOpen;
			SwitchThreshSq = m_ImpulseThreshOpenSq;
		}
		
		// only resolve along the axis if it is set.  Defaults to (0,0,0) if not set
		if( SwitchDir.LengthSqr() )
		{
			SwitchDir = GetPhysics()->GetAxis() * SwitchDir;
			ImpulseMagSq = impulse*SwitchDir;
			ImpulseMagSq *= ImpulseMagSq;
		}
		else
			ImpulseMagSq = impulse.LengthSqr();

		if( ImpulseMagSq >= SwitchThreshSq )
			ToggleOpen();
	}

	idEntity::ApplyImpulse( ent, id, point, impulse);
}

/*-------------------------------------------------------------------------*/

bool CBinaryFrobMover::isMoving()
{
	return ((m_Translating) || (m_Rotating));
}

/*-------------------------------------------------------------------------*/

bool CBinaryFrobMover::isChangingState()
{
	return m_StateChange;
}

/*-------------------------------------------------------------------------*/

void CBinaryFrobMover::getRemainingMovement
(
	idVec3& out_deltaPosition,
	idAngles& out_deltaAngles
)
{
	// Get remaining translation if translating
	if (m_bIntentOpen)
	{
		out_deltaPosition = (m_StartPos + m_Translation) - physicsObj.GetOrigin();
	}
	else
	{
		out_deltaPosition = m_StartPos - physicsObj.GetOrigin();
	}

	// Get remaining rotation
	idAngles curAngles;
	physicsObj.GetAngles(curAngles);

	if (m_bIntentOpen)
	{
		out_deltaAngles = m_OpenAngles - curAngles;
	}
	else
	{
		out_deltaAngles = m_ClosedAngles - curAngles;
	}

	// Done
}
