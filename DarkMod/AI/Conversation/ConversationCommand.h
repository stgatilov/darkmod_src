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
	// These are the various command types
	enum Type
	{
		EWaitSeconds = 0,
		EWaitForTrigger,
		EWaitForActor,
		EWalkToPosition,
		EWalkToEntity,
		EStopMove,
		ETalk,
		EPlayAnimOnce,
		EPlayAnimCycle,
		EActivateTarget,
		ELookAtActor,
		ELookAtPosition,
		ELookAtEntity,
		ETurnToActor,
		ETurnToPosition,
		ETurnToEntity,
		EAttackActor,
		EAttackEntity,
		ENumCommands,
	};

	// The string representations of the above (keep in sync please)
	static const char* const TypeNames[ENumCommands];

private:
	// The type of this command
	Type _type;

	// Argument list
	idStringList _arguments;

public:
	// Returns the type of this conversation command
	Type GetType();

	// Returns the number of arguments
	int GetNumArguments();

	// Returns the given argument (starting with index 0) or "" if the argument doesn't exist
	idStr GetArgument(int index);

	/**
	 * greebo: Parses the command parameters from the given idDict.
	 * The prefix is something along the lines "conv_2_cmd_3_" and is
	 * used to find all spawnargs relevant for this command.
	 *
	 * Returns TRUE if the parse process succeeded, FALSE otherwise.
	 */
	bool Parse(const idDict& dict, const idStr& prefix);

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	// Tries to convert the string representation of the command into a Type enum value
	static Type GetType(const idStr& cmdString);
};
typedef boost::shared_ptr<ConversationCommand> ConversationCommandPtr;

} // namespace ai

#endif /* __AI_CONVERSATION_COMMAND_H__ */
