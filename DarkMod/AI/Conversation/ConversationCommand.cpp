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

// These are the various command type strings, needed for parsing
const char* const ConversationCommand::TypeNames[ConversationCommand::ENumCommands] =
{
	"WaitSeconds",
	"WaitForTrigger",
	"WaitForActor",
	"WalkToPosition",
	"WalkToEntity",
	"StopMove",
	"Talk",
	"PlayAnimOnce",
	"PlayAnimCycle",
	"ActivateTarget",
	"LookAtActor",
	"LookAtPosition",
	"LookAtEntity",
	"TurnToActor",
	"TurnToPosition",
	"TurnToEntity",
	"AttackActor",
	"AttackEntity"
};

ConversationCommand::Type ConversationCommand::GetType()
{
	return _type;
}

int ConversationCommand::GetNumArguments()
{
	return _arguments.Num();
}

// Returns the given argument (starting with index 0) or "" if the argument doesn't exist
idStr ConversationCommand::GetArgument(int index)
{
	return (index > 0 && index < _arguments.Num()) ? _arguments[index] : "";
}

bool ConversationCommand::Parse(const idDict& dict, const idStr& prefix)
{
	// Get the type
	_type = GetType(dict.GetString(prefix + "type", idStr(ENumCommands)));

	if (_type == ENumCommands)
	{
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Could not find type for command prefix %s.\r", prefix.c_str());
		return false; // invalid command type
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Found command type %s for prefix %s.\r", TypeNames[_type], prefix.c_str());

	// Parse the arguments
	_arguments.Clear();

	idStr argPrefix = prefix + "arg_";
	for (const idKeyValue* kv = dict.MatchPrefix(argPrefix); kv != NULL; kv = dict.MatchPrefix(argPrefix, kv))
	{
		// Add each actor name to the list
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Adding argument %s to conversation command %s.\r", kv->GetValue().c_str(), TypeNames[_type]);
		_arguments.Append(kv->GetValue());
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Command type %s has %d arguments.\r", TypeNames[_type], _arguments.Num());

	return true;
}

void ConversationCommand::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(static_cast<int>(_type));

	savefile->WriteInt(_arguments.Num());
	for (int i = 0; i < _arguments.Num(); i++)
	{
		savefile->WriteString(_arguments[i]);
	}
}

void ConversationCommand::Restore(idRestoreGame* savefile)
{
	int typeInt;
	savefile->ReadInt(typeInt);
	assert(typeInt >= 0 && typeInt <= ENumCommands); // sanity check
	_type = static_cast<Type>(typeInt);

	int num;
	savefile->ReadInt(num);
	_arguments.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_arguments[i]);
	}
}

ConversationCommand::Type ConversationCommand::GetType(const idStr& cmdString)
{
	for (int i = 0; i < ENumCommands; i++)
	{
		if (cmdString == TypeNames[i])
		{
			return static_cast<Type>(i);
		}
	}

	return ENumCommands;
}

} // namespace ai
