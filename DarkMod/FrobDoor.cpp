/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2004/11/14 20:19:11  sparhawk
 * Initial Release
 *
 *
 ***************************************************************************/

// Copyright (C) 2004 Gerhard W. Gruber <sparhawk@gmx.at>
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "../game/Game_local.h"
#include "DarkModGlobals.h"
#include "FrobDoor.h"

//===============================================================================
//CFrobDoor
//===============================================================================

CLASS_DECLARATION( idMover, CFrobDoor )
END_CLASS

CFrobDoor::CFrobDoor(void)
{
	DM_LOG(LC_FUNCTION, LT_DEBUG).LogString("this: %08lX [%s]\r", this, __FUNCTION__);
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
	idMover::Spawn();

	LoadTDMSettings();

	// If this is a frobable door we need to activate the frobcode.
	if(m_FrobDistance != 0)
	{
		DM_LOG(LC_FROBBING, LT_INFO).LogString("%s: RenderEntity: %08lX  Frob activated\r", __FUNCTION__, renderEntity, renderView);
		renderEntity.callback = CFrobDoor::ModelCallback;
	}
}

bool CFrobDoor::ModelCallback(renderEntity_s *renderEntity, const renderView_t *renderView)
{
	DM_LOG(LC_FROBBING, LT_INFO).LogString("%s: RenderEntity: %08lX  RenderView: %08lX\r", __FUNCTION__, renderEntity, renderView);

	// this may be triggered by a model trace or other non-view related source
	if(!renderView)
		return false;

	bool bRc = false;
	CFrobDoor *ent;

	ent = static_cast<CFrobDoor *>(gameLocal.entities[ renderEntity->entityNum ]);
	if(!ent)
	{
		gameLocal.Error( "CFrobDoor::ModelCallback: callback with NULL game entity" );
		bRc = false;
	}
	else
		bRc = ent->Frob(CONTENTS_OPAQUE|CONTENTS_PLAYERCLIP|CONTENTS_RENDERMODEL, &ent->renderEntity.shaderParms[11]);

	return bRc;
}

void CFrobDoor::FrobAction(void)
{
	function_t *pScriptFkt = gameLocal.program.FindFunction("try_door");
	DM_LOG(LC_FROBBING, LT_INFO).LogString("FrobAction has been triggered (%08lX)\r", pScriptFkt);
	if(pScriptFkt)
	{
		DM_LOG(LC_FROBBING, LT_INFO).LogString("ScriptDef: (%08lX)  Elements: %u   Typedef: %u\r", pScriptFkt->def, pScriptFkt->parmSize.Num(), pScriptFkt->def->TypeDef()->Type());

		idThread *pThread = new idThread(pScriptFkt);
		pThread->CallFunction(this, pScriptFkt, true);
		pThread->DelayedStart(0);
	}
}

