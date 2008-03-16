/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2126 $
 * $Date: 2008-03-07 18:02:53 +0100 (Fr, 07 Mrz 2008) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: FrobButton.cpp 2126 2008-03-07 17:02:53Z angua $", init_version);

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
