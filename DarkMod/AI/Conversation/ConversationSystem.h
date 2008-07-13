/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_CONVERSATION_SYSTEM_H__
#define __AI_CONVERSATION_SYSTEM_H__

#include "../../../idlib/precompiled.h"

#include "Conversation.h"

namespace ai {

class ConversationSystem
{
public:
	// Clears and removes all allocated data
	void Clear();

	/**
	 * greebo: Initialises this class. This means loading the conversation entities
	 * containing all conversations for this map.
	 */
	void Init(idMapFile* mapFile);

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	// Helper to load a conversation from an entity's spawnargs
	void LoadConversationEntity(idMapEntity* entity);
};
typedef boost::shared_ptr<ConversationSystem> ConversationSystemPtr;

} // namespace ai

#endif /* __AI_CONVERSATION_SYSTEM_H__ */
