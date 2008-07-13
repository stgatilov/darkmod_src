/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2395 $
 * $Date: 2008-05-27 20:40:03 +0200 (Di, 27 Mai 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_CONVERSATION_SYSTEM_H__
#define __AI_CONVERSATION_SYSTEM_H__

#include "../../../idlib/precompiled.h"

#include <boost/shared_ptr.hpp>

namespace ai {

class ConversationSystem
{
	// Clears and removes all allocated data
	void Clear();

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};
typedef boost::shared_ptr<ConversationSystem> ConversationSystemPtr;

} // namespace ai

#endif /* __AI_CONVERSATION_SYSTEM_H__ */
