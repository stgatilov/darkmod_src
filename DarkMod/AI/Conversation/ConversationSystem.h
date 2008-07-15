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
	// The indexed list of conversations
	idList<ConversationPtr> _conversations;

public:
	// Clears and removes all allocated data
	void Clear();

	/**
	 * greebo: Initialises this class. This means loading the conversation entities
	 * containing all conversations for this map.
	 */
	void Init(idMapFile* mapFile);

	/**
	 * greebo: Returns the conversation for the given name/index or NULL if not found.
	 */
	ConversationPtr GetConversation(const idStr& name);
	ConversationPtr GetConversation(int index);

	/**
	 * Returns the numeric index for the given conversation name or -1 if not found.
	 */
	int GetConversationIndex(const idStr& name);

	/**
	 * greebo: This starts the conversation, after checking whether all conditions are met.
	 *
	 * @index: The conversation index. Use GetConversationIndex() to convert a conversation name to an index.
	 */
	void StartConversation(int index);

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
