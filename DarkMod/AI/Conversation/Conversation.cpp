/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2649 $
 * $Date: 2008-07-13 15:16:43 +0200 (So, 13 Jul 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: Conversation.cpp 2649 2008-07-13 13:16:43Z greebo $", init_version);

#include "Conversation.h"

namespace ai {

Conversation::Conversation() :
	_isValid(false)
{}

Conversation::Conversation(const idDict& spawnArgs, int index) :
	_isValid(false)
{
	// Pass the call to the parser
	InitFromSpawnArgs(spawnArgs, index);
}

bool Conversation::IsValid()
{
	return _isValid;
}

void Conversation::Save(idSaveGame* savefile) const
{
	// TODO
}

void Conversation::Restore(idRestoreGame* savefile)
{
	// TODO
}

void Conversation::InitFromSpawnArgs(const idDict& dict, int index)
{
	// TODO
}

} // namespace ai
