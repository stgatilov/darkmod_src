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
#include "FrobLock.h"

CLASS_DECLARATION( idStaticEntity, CFrobLock )
	EVENT( EV_PostSpawn,	CFrobLock::PostSpawn )
END_CLASS

void CFrobLock::Save(idSaveGame *savefile) const
{
	// TODO
}

void CFrobLock::Restore( idRestoreGame *savefile )
{
	// TODO
}

void CFrobLock::Spawn()
{
	PostEventMS(&EV_PostSpawn, 0);
}

void CFrobLock::PostSpawn()
{
	// TODO: Find lever entities
}
