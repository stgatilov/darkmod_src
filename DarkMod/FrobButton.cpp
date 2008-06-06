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
//CFrobButton
//===============================================================================


const idEventDef EV_TDM_Button_Open( "Open", "f" );
const idEventDef EV_TDM_Button_Close( "Close", "f" );
const idEventDef EV_TDM_Button_Operate("Operate", NULL);

CLASS_DECLARATION( CBinaryFrobMover, CFrobButton )
	EVENT( EV_TDM_Button_Open,				CFrobButton::Open)
	EVENT( EV_TDM_Button_Close,				CFrobButton::Close)
	EVENT( EV_TDM_Button_Operate,			CFrobButton::Operate)
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
	Open(false);
}

void CFrobButton::Open(bool bMaster)
{
	CBinaryFrobMover::Open(false);
}

void CFrobButton::Close(bool bMaster)
{
	CBinaryFrobMover::Close(false);
}

void CFrobButton::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse )
{
	// Check if the impulse is applied in the right direction
	if (impulse * m_Translation >= 0)
	{
		Operate();
	}
}


// A button can't close or open a portal, so we block it.
void CFrobButton::ClosePortal()
{
}

void CFrobButton::OpenPortal()
{
}