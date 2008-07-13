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

#include "ConversationCommand.h"

namespace ai {

void ConversationCommand::Save(idSaveGame* savefile) const
{
	// TODO
}

void ConversationCommand::Restore(idRestoreGame* savefile)
{
	// TODO
}

} // namespace ai
