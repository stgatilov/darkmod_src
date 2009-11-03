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

#include "../DarkMod/DarkModGlobals.h"
#include "../DarkMod/PlayerData.h"

CDarkModPlayer::CDarkModPlayer()
{
	m_LightgemValue = 0;
}

void CDarkModPlayer::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(m_LightgemValue);
	savefile->WriteFloat(m_fColVal);
}

void CDarkModPlayer::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(m_LightgemValue);
	savefile->ReadFloat(m_fColVal);
}

