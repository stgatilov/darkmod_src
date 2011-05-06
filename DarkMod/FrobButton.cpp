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
#include "FrobButton.h"
#include "sndProp.h"

//===============================================================================
// CFrobButton
//===============================================================================

const idEventDef EV_TDM_Button_Operate("Operate", NULL);

CLASS_DECLARATION( CBinaryFrobMover, CFrobButton )
	EVENT( EV_TDM_Button_Operate,	CFrobButton::Operate)
	EVENT( EV_PostSpawn,			CFrobButton::PostSpawn ) // grayman #2603
END_CLASS

void CFrobButton::Save(idSaveGame *savefile) const
{
	// nothing to save (yet)
}

void CFrobButton::Restore( idRestoreGame *savefile )
{
	// nothing to restore (yet)
}

void CFrobButton::Spawn()
{
}

void CFrobButton::Operate()
{
	ToggleOpen();
}

void CFrobButton::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse )
{
	// grayman #2603 - check "noimpact" flag
	if (spawnArgs.GetBool("noimpact"))
	{
		return; // button can't be hit, so do nothing
	}

	// Check if the impulse is applied in the right direction
	if (impulse * m_Translation >= 0)
	{
		Operate();
	}
}

/*
================
CFrobButton::PostSpawn

In a post-spawn event, provide self to all targeted lights as a switch for those lights.
================
*/
void CFrobButton::PostSpawn( void ) // grayman #2603
{
	// Let the base class do its stuff first
	CBinaryFrobMover::PostSpawn();

	// Process targets. For those that are lights, add yourself
	// to their switch list.

	for( int i = 0 ; i < targets.Num() ; i++ )
	{
		idEntity* e = targets[i].GetEntity();
		if (e)
		{
			if (e->IsType(idLight::Type))
			{
				idLight* light = static_cast<idLight*>(e);
				light->AddSwitch(this);
			}
		}
	}
}

