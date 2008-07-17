/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AI_CONVERSATION_STATE_H__
#define __AI_CONVERSATION_STATE_H__

#include "State.h"
#include "../Conversation/ConversationCommand.h"

namespace ai
{

#define STATE_CONVERSATION "Conversation"

class ConversationState :
	public State
{
	// The conversation index
	int _conversation;

	// The state we're in
	ConversationCommand::State _state;

	int _finishTime;

public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	// Sets the conversation this state should handle
	void SetConversation(int index);

	// Starts execution of the given command, returns FALSE on failure
	void StartCommand(ConversationCommand& command);

	// Handles the given command, returns FALSE on failure
	void Execute(ConversationCommand& command);

	// Returns the current conversation command execution state
	ConversationCommand::State GetExecutionState();

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner);

	static StatePtr CreateInstance();

private:
	// Plays the given sound (shader) and returns the length in msecs
	int Talk(idAI* owner, const idStr& soundName);

	// Returns true if the conversation can be started
	bool CheckConversationPrerequisites();

	// Private helper for debug output
	void DrawDebugOutput(idAI* owner);
};
typedef boost::shared_ptr<ConversationState> ConversationStatePtr;

} // namespace ai

#endif /* __AI_CONVERSATION_STATE_H__ */
