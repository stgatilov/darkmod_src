/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2648 $
 * $Date: 2008-07-13 14:47:00 +0200 (So, 13 Jul 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_CONVERSATION_H__
#define __AI_CONVERSATION_H__

#include "../../../idlib/precompiled.h"

#include <boost/shared_ptr.hpp>

namespace ai {

/**
 * greebo: This class encapsulates a single conversation between
 * two or more characters in the game.
 */
class Conversation
{
	// whether this conversation has errors or not
	bool _isValid;

public:
	Conversation();

	// Construct a conversation using the given spawnargs, using the given index
	Conversation(const idDict& spawnArgs, int index);

	/**
	 * greebo: Returns TRUE if this conversation is valid.
	 * Use this to check whether the construction of this class from given
	 * spawnargs has been successful.
	 */
	bool IsValid();

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	// Helper function to parse the properties from the spawnargs
	void InitFromSpawnArgs(const idDict& dict, int index);
};
typedef boost::shared_ptr<Conversation> ConversationPtr;

} // namespace ai

#endif /* __AI_CONVERSATION_H__ */
