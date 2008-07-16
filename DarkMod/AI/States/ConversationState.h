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
public:
	enum Status
	{
		EDoingNothing = 0,
		EExecuting,
		EDone,
		ENumConversationStati, // invalid index
	};

private:
	// The conversation index
	int _conversation;

	// The current execution status (e.g. talking or waiting for something)
	Status _status;

public:
	// Get the name of this state
	virtual const idStr& GetName() const;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	// Sets the conversation this state should handle
	void SetConversation(int index);

	// Handles the given command, returns FALSE on failure
	bool Execute(ConversationCommand& command);

	// Returns the conversation status to let outsiders know if the current action is finished
	Status GetStatus();

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner);

	static StatePtr CreateInstance();

private:
	// Returns true if the conversation can be started
	bool CheckConversationPrerequisites();
};
typedef boost::shared_ptr<ConversationState> ConversationStatePtr;

} // namespace ai

#endif /* __AI_CONVERSATION_STATE_H__ */
