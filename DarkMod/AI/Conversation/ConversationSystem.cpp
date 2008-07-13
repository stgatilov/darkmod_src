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

void ConversationSystem::Save(idSaveGame* savefile) const
{
	// TODO
}

void ConversationSystem::Restore(idRestoreGame* savefile)
{
	// TODO
}

void ConversationSystem::LoadConversationEntity(idMapEntity* entity)
{
	assert(entity != NULL);

	DM_LOG(LC_CONVERSATION, LT_DEBUG)LOGSTRING("Investigating conversation entity %s.\r", entity->epairs.GetString("name"));

	for (int i = 0; i < INT_MAX; i++)
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
