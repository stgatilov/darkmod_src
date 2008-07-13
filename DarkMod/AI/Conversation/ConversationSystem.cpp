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

#include "ConversationSystem.h"

namespace ai {

void ConversationSystem::Clear()
{
}

void ConversationSystem::Save(idSaveGame* savefile) const
{
	// TODO
}

void ConversationSystem::Restore(idRestoreGame* savefile)
{
	// TODO
}

} // namespace ai
