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

#include "Conversation.h"

namespace ai {

Conversation::Conversation() :
	_isValid(false),
	_talkDistance(0.0f)
{}

Conversation::Conversation(const idDict& spawnArgs, int index) :
	_isValid(false),
	_talkDistance(0.0f)
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
	savefile->WriteString(_name);
	savefile->WriteBool(_isValid);
	savefile->WriteFloat(_talkDistance);
}

void Conversation::Restore(idRestoreGame* savefile)
{
	savefile->ReadString(_name);
	savefile->ReadBool(_isValid);
	savefile->ReadFloat(_talkDistance);
}

void Conversation::InitFromSpawnArgs(const idDict& dict, int index)
{
	idStr prefix = va("conv_%d_", index);

	// A non-empty name is mandatory for a conversation
	if (!dict.GetString(prefix + "name", "", _name) || _name.IsEmpty())
	{
		// No conv_N_name spawnarg found, bail out
		_isValid = false;
		return;
	}

	_talkDistance = dict.GetFloat(prefix + "talk_distance");

	// TODO: Add more sophisticated validity check here
	_isValid = true;
}

} // namespace ai
