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

	float talkDistance = entity->epairs.GetFloat("talk_distance", "100");
}

} // namespace ai
