/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_CONVERSATION_COMMAND_H__
#define __AI_CONVERSATION_COMMAND_H__

#include "../../../idlib/precompiled.h"

#include <boost/shared_ptr.hpp>

namespace ai {

class ConversationCommand
{
public:
	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};
typedef boost::shared_ptr<ConversationCommand> ConversationCommandPtr;

} // namespace ai

#endif /* __AI_CONVERSATION_COMMAND_H__ */
