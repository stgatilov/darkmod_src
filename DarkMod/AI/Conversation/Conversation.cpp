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

const int MAX_ALERT_LEVEL_TO_START_CONVERSATION = 1;

namespace ai {

Conversation::Conversation() :
	_isValid(false),
	_talkDistance(0.0f),
	_playCount(0)
{}

Conversation::Conversation(const idDict& spawnArgs, int index) :
	_isValid(false),
	_talkDistance(0.0f),
	_playCount(0)
{
	// Pass the call to the parser
	InitFromSpawnArgs(spawnArgs, index);
}

bool Conversation::IsValid()
{
	return _isValid;
}

const idStr& Conversation::GetName() const
{
	return _name;
}

int Conversation::GetPlayCount()
{
	return _playCount;
}

bool Conversation::CheckConditions()
{
	// greebo: Check if all actors are available
	for (int i = 0; i < _actors.Num(); i++)
	{
		// Get the actor
		idActor* actor = GetActor(i);

		if (actor != NULL && (actor->IsKnockedOut() || actor->health <= 0))
		{
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Actor %s in conversation %s is KO or dead!.\r", _actors[i].c_str(), _name.c_str());
			return false;
		}

		if (actor->IsType(idAI::Type))
		{
			idAI* ai = static_cast<idAI*>(actor);

			if (ai->AI_AlertIndex > MAX_ALERT_LEVEL_TO_START_CONVERSATION)
			{
				// AI is too alerted to start this conversation
				return false;
			}
		}
	}

	// All actors alive and kicking

	// TODO: Other checks to perform?

	return true;
}

void Conversation::Start()
{
	// Switch all participating AI to conversation state
	for (int i = 0; i < _actors.Num(); i++)
	{
		idActor* actor = GetActor(i);

		if (actor->IsType(idAI::Type))
		{
			static_cast<idAI*>(actor)->SwitchToConversationState(_name);
		}
	}

	_playCount++;

	// Set the index to the first command
	_currentCommand = 0;
}

void Conversation::Process()
{
	// Check for index out of bounds in debug builds
	assert(_currentCommand >= 0 && _currentCommand < _commands.Num());

	// Get the command as specified by the pointer
	const ConversationCommandPtr& command = _commands[_currentCommand];

	//command->GetActor();
}

idActor* Conversation::GetActor(int index)
{
	if (index < 0 || index >= _actors.Num()) 
	{
		return NULL; // index out of bounds
	}

	// Resolve the entity name and get the pointer
	idEntity* ent = gameLocal.FindEntity(_actors[index]);

	if (ent == NULL || !ent->IsType(idActor::Type)) 
	{
		// not an actor!
		DM_LOG(LC_CONVERSATION, LT_ERROR)LOGSTRING("Actor %s in conversation %s is not existing or of wrong type.\r", _actors[index].c_str(), _name.c_str());
		return NULL; 
	}

	return static_cast<idActor*>(ent);
}

idActor* Conversation::GetActor(const idStr& name)
{
	// greebo: Check if all actors are available
	for (int i = 0; i < _actors.Num(); i++)
	{
		if (_actors[i] == name) 
		{
			// index found, get the actor
			return GetActor(i);
		}
	}

	return NULL;
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

	savefile->WriteInt(_commands.Num());
	for (int i = 0; i < _commands.Num(); i++)
	{
		_commands[i]->Save(savefile);
	}

	savefile->WriteInt(_currentCommand);
	savefile->WriteInt(_playCount);
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

	savefile->ReadInt(num);
	_commands.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		_commands[i] = ConversationCommandPtr(new ConversationCommand);
		_commands[i]->Restore(savefile);
	}

	savefile->ReadInt(_currentCommand);
	savefile->ReadInt(_playCount);
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
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Adding actor %s to conversation %s.\r", kv->GetValue().c_str(), _name.c_str());
		_actors.AddUnique(kv->GetValue());
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Conversation %s has %d actors.\r", _name.c_str(), _actors.Num());

	if (_actors.Num() == 0)
	{
		_isValid = false; // no actors, no conversation
		gameLocal.Warning("Ignoring conversation %s as it has no actors.\n", _name.c_str());
		return;
	}

	// Start parsing the conversation scripts (i.e. the commands), start with index 1
	for (int i = 1; i < INT_MAX; i++)
	{
		idStr cmdPrefix = va(prefix + "cmd_%d_", i);

		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Attempting to find command with index %d matching prefix %s.\r", i, cmdPrefix.c_str());

		if (dict.MatchPrefix(cmdPrefix) != NULL)
		{
			// Found a matching "conv_N_cmd_M..." spawnarg, start parsing 
			ConversationCommandPtr cmd(new ConversationCommand);

			// Let the command parse itself
			if (cmd->Parse(dict, cmdPrefix))
			{
				// Parsing succeeded, add this to the command list
				_commands.Append(cmd);
			}
		}
		else 
		{
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("No match found, terminating loop on index %d.\r", i);
			break;
		}
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("%d Commands found for Conversation %s.\r", _commands.Num(), _name.c_str());

	// Sanity check the commands
	if (_commands.Num() == 0) 
	{
		// No commands, what kind of conversation is this?
		_isValid = false;
		gameLocal.Warning("Ignoring conversation %s as it has no commands.\n", _name.c_str());
		return;
	}

	// Sanity check the talk distance
	if (_talkDistance <= 0.0f)
	{
		_isValid = false;
		gameLocal.Warning("Ignoring conversation %s as it has a talk distance <= 0.\n", _name.c_str());
		return;
	}

	// Seems like we have everything we need
	_isValid = true;
}

} // namespace ai
