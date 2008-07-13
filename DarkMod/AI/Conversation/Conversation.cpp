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

	savefile->WriteInt(_actors.Num());
	for (int i = 0; i < _actors.Num(); i++)
	{
		savefile->WriteString(_actors[i]);
	}
}

void Conversation::Restore(idRestoreGame* savefile)
{
	savefile->ReadString(_name);
	savefile->ReadBool(_isValid);
	savefile->ReadFloat(_talkDistance);

	int num;
	savefile->ReadInt(num);
	_actors.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_actors[i]);
	}
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

	// Parse "global" conversation settings 
	_talkDistance = dict.GetFloat(prefix + "talk_distance");

	// Parse participant actors
	// Check if this entity can be used by others.
	idStr actorPrefix = prefix + "actor_";
	for (const idKeyValue* kv = dict.MatchPrefix(actorPrefix); kv != NULL; kv = dict.MatchPrefix(actorPrefix, kv))
	{
		// Add each actor name to the list
		_actors.AddUnique(kv->GetValue());
	}

	// TODO: Add more sophisticated validity check here
	_isValid = true;
}

} // namespace ai
