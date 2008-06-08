/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "DarkModGlobals.h"
#include "FrobLever.h"

const idEventDef EV_TDM_Lever_Operate( "Operate", NULL );
const idEventDef EV_TDM_Lever_Switch( "Switch", "d" );

CLASS_DECLARATION( CBinaryFrobMover, CFrobLever )
	EVENT( EV_TDM_Lever_Operate,		CFrobLever::Event_Operate)
	EVENT( EV_TDM_Lever_Switch,			CFrobLever::Event_Switch)
END_CLASS

void CFrobLever::Save(idSaveGame *savefile) const
{
	// nothing to save (yet)
}

void CFrobLever::Restore( idRestoreGame *savefile )
{
	// nothing to restore (yet)
}

void CFrobLever::Spawn()
{
}

void CFrobLever::SwitchState(bool newState)
{
	if (newState)
	{
		Open(false);
	}
	else
	{
		Close(false);
	}
}

void CFrobLever::Operate()
{
	if (IsOpen())
	{
		Close(false);
	}
	else
	{
		Open(false);
	}
}

void CFrobLever::Open(bool bMaster)
{
	// If the mover is already open, we don't have anything to do. :)
	if (m_Open == true)
		return;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobLever: Opening\r" );

	// don't play the sound if the door was not closed all the way
	m_StateChange = true;
		
	StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );
		
	// trigger our targets on opening, if set to do so
	if( spawnArgs.GetBool("trigger_on_open","") )
	{
		ActivateTargets( this );
	}

	idAngles tempAng;
	physicsObj.GetLocalAngles( tempAng );

	m_Open = true;
	m_Rotating = true;
	m_Translating = true;
	idAngles angleDelta = (m_OpenAngles - tempAng).Normalize180();
	
	if (!angleDelta.Compare(idAngles(0,0,0)))
	{
		Event_RotateOnce(angleDelta);
	}
	else
	{
		m_Rotating = false;
	}
	
	if( m_TransSpeed )
	{
		Event_SetMoveSpeed( m_TransSpeed );
	}

			

	if (!physicsObj.GetLocalOrigin().Compare(m_OpenOrigin, VECTOR_EPSILON))
	{
		MoveToLocalPos( m_OpenOrigin );
	}
	else
	{
		m_Translating = false;
	}
}

void CFrobLever::Close(bool bMaster)
{
	// If the mover is already closed, we don't have anything to do. :)
	if(m_Open == false)
		return;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("FrobLever: Closing\r" );

	m_StateChange = true;
	m_Rotating = true;
	m_Translating = true;

	idAngles tempAng;
	physicsObj.GetLocalAngles( tempAng );
	idAngles angleDelta = (m_ClosedAngles - tempAng).Normalize180();

	if (!angleDelta.Compare(idAngles(0,0,0)))
	{
		Event_RotateOnce(angleDelta);
	}
	else
	{
		m_Rotating = false;
	}
	
	if( m_TransSpeed )
	{
		Event_SetMoveSpeed( m_TransSpeed );
	}

	if (!physicsObj.GetLocalOrigin().Compare(m_ClosedOrigin, VECTOR_EPSILON))
	{
		MoveToLocalPos(m_ClosedOrigin);
	}
	else
	{
		m_Translating = false;
	}
}

void CFrobLever::Event_Operate()
{
	Operate();
}

void CFrobLever::Event_Switch(int newState)
{
	SwitchState(newState == 0 ? false : true);
}

// A lever can't close or open a portal, so we block it.
void CFrobLever::ClosePortal()
{
}

void CFrobLever::OpenPortal()
{
}
