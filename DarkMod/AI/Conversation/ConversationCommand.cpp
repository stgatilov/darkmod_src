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

bool ConversationCommand::Parse(const idDict& dict, const idStr& prefix)
{
	return false;
}

void ConversationCommand::Save(idSaveGame* savefile) const
{
	// TODO
}

void ConversationCommand::Restore(idRestoreGame* savefile)
{
	// TODO
}

} // namespace ai
