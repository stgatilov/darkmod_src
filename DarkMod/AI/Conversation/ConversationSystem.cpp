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

#define CONVERSATION_ENTITYCLASS "atdm:conversation_info"

namespace ai {

void ConversationSystem::Clear()
{
	_conversations.Clear();
}

void ConversationSystem::Init(idMapFile* mapFile)
{
	DM_LOG(LC_CONVERSATION, LT_INFO)LOGSTRING("Searching for difficulty setting on worldspawn.\r");

	if (mapFile->GetNumEntities() <= 0) {
		return; // no entities!
	}

	// Fetch the worldspawn
	for (int i = 0; i < mapFile->GetNumEntities(); i++)
	{
		idMapEntity* mapEnt = mapFile->GetEntity(i);

		idStr className = mapEnt->epairs.GetString("classname");

		if (className == CONVERSATION_ENTITYCLASS)
		{
			// Found an entity, parse the conversation from it
			LoadConversationEntity(mapEnt);
		}
	}

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("%d Conversations found in this map.\r", _conversations.Num());
	gameLocal.Printf("ConversationManager: Found %d valid conversations.\n", _conversations.Num());
}

ConversationPtr ConversationSystem::GetConversation(const idStr& name)
{
	// Find the index and pass the call to the overload
	return GetConversation(GetConversationIndex(name));
}

ConversationPtr ConversationSystem::GetConversation(int index)
{
	return (index >= 0 && index < _conversations.Num()) ? _conversations[index] : ConversationPtr();
}

int ConversationSystem::GetConversationIndex(const idStr& name)
{
	for (int i = 0; i < _conversations.Num(); i++)
	{
		if (_conversations[i]->GetName() == name)
		{
			return i;
		}
	}

	return -1;
}

void ConversationSystem::StartConversation(int index)
{
	ConversationPtr conv = GetConversation(index);

	if (conv == NULL)
	{
		gameLocal.Warning("StartConversation: Can't find conversation with index %d\n", index);
		return;
	}

	DM_LOG(LC_CONVERSATION, LT_INFO)LOGSTRING("Trying to start conversation %s.\r", conv->GetName().c_str());


}

void ConversationSystem::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(_conversations.Num());
	for (int i = 0; i < _conversations.Num(); i++)
	{
		_conversations[i]->Save(savefile);
	}
}

void ConversationSystem::Restore(idRestoreGame* savefile)
{
	_conversations.Clear();

	int num;
	savefile->ReadInt(num);
	_conversations.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		// Allocate a new conversation and restore it
		_conversations[i] = ConversationPtr(new Conversation);
		_conversations[i]->Restore(savefile);
	}
}

void ConversationSystem::LoadConversationEntity(idMapEntity* entity)
{
	assert(entity != NULL);

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Investigating conversation entity %s.\r", entity->epairs.GetString("name"));

	// The conversation index starts with 1, not zero
	for (int i = 1; i < INT_MAX; i++)
	{
		DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Attempting to parse using conversation index %d.\r", i);

		// Attempt to construct a new Conversation object
		ConversationPtr conv(new Conversation(entity->epairs, i));

		if (conv->IsValid())
		{
			// Add the conversation to the list
			_conversations.Append(conv);
		}
		else
		{
			// This loop breaks on the first invalid conversation
			DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Conversation entity %s: found %d valid conversations.\r", entity->epairs.GetString("name"), i);
			break;
		}
	}
}

} // namespace ai
