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

#include "ConversationState.h"
#include "../Memory.h"
#include "../Tasks/IdleAnimationTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "ObservantState.h"
#include "../Library.h"
#include "../Conversation/Conversation.h"
#include "../Conversation/ConversationCommand.h"

// greebo: This spawnarg holds the currently played conversation sound
#define CONVERSATION_SPAWNARG "snd_TEMP_conv"
#define DEFAULT_LOOKAT_DURATION 5.0f
#define DEFAULT_WALKTOENTITY_DISTANCE 50.0f

namespace ai
{

// Get the name of this state
const idStr& ConversationState::GetName() const
{
	static idStr _name(STATE_CONVERSATION);
	return _name;
}

bool ConversationState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex > 0)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(STATE_OBSERVANT);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void ConversationState::SetConversation(int index)
{
	// TODO: Sanity-Check

	_conversation = index;
}

void ConversationState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ConversationState initialised.\r");
	assert(owner);

	// Memory shortcut
	Memory& memory = owner->GetMemory();
	memory.alertClass = EAlertNone;
	memory.alertType = EAlertTypeNone;

	_alertLevelDecreaseRate = 0.01f;

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Check dialogue prerequisites
	if (!CheckConversationPrerequisites())
	{
		owner->mind->EndState();
		return;
	}

	// We haven't started doing our stuff yet
	_finishTime = -1;
	_commandType = ConversationCommand::ENumCommands;
	_state = ConversationCommand::ENotStartedYet;

	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
}

// Gets called each time the mind is thinking
void ConversationState::Think(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) return;

	// Let the AI check its senses
	owner->PerformVisualScan();

	if (_finishTime > 0 && gameLocal.time > _finishTime)
	{
		_state = ConversationCommand::EFinished;
	}

	DrawDebugOutput(owner);
}

ConversationCommand::State ConversationState::GetExecutionState()
{
	return _state;
}

bool ConversationState::CheckConversationPrerequisites()
{
	// TODO
	return true;
}

void ConversationState::OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem)
{
	if (_state != ConversationCommand::EExecuting) return;

	if (subSystem == SubsysMovement)
	{
		// In case of active "walk" commands, set the state to "finished"
		if (_commandType == ConversationCommand::EWalkToEntity || _commandType == ConversationCommand::EWalkToPosition)
		{
			_state = ConversationCommand::EFinished;
		}
	}
}

void ConversationState::StartCommand(ConversationCommand& command, Conversation& conversation)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	switch (command.GetType())
	{
	case ConversationCommand::EWaitSeconds:
		if (!idStr::IsNumeric(command.GetArgument(0)))
		{
			gameLocal.Warning("Conversation Command argument for 'WaitSeconds' is not numeric: %s", command.GetArgument(0).c_str());
		}
		_finishTime = gameLocal.time + SEC2MS(atof(command.GetArgument(0)));
		_state = ConversationCommand::EExecuting;
	break;
	case ConversationCommand::EWaitForTrigger:
	case ConversationCommand::EWaitForActor:
	case ConversationCommand::EWalkToPosition:
		break;
	case ConversationCommand::EWalkToEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);
		if (ent != NULL)
		{
			// Start moving
			idVec3 delta = ent->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
			idVec3 deltaNorm(delta);
			deltaNorm.NormalizeFast();

			float distance = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_WALKTOENTITY_DISTANCE;
			
			idVec3 goal = owner->GetPhysics()->GetOrigin() + delta - deltaNorm*distance;

			owner->GetSubsystem(SubsysMovement)->PushTask(
				TaskPtr(new MoveToPositionTask(goal))
			);
			//owner->MoveToPosition(ent->GetPhysics()->GetOrigin());
			_state = ConversationCommand::EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'WalkToEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::EStopMove:
		owner->StopMove(MOVE_STATUS_DONE);
		_state = ConversationCommand::EFinished;
		break;

	case ConversationCommand::ETalk:
	{
		int length = Talk(owner, command.GetArgument(0));

		// Set the finish conditions for the current action
		_state = ConversationCommand::EExecuting;
		_finishTime = gameLocal.time + length + 200;
	}
	break;

	case ConversationCommand::EPlayAnimOnce:
	case ConversationCommand::EPlayAnimCycle:
		break;

	case ConversationCommand::EActivateTarget:
	{
		idEntity* ent = command.GetEntityArgument(0);
		if (ent != NULL)
		{
			// Post a trigger event
			ent->PostEventMS(&EV_Activate, 0, owner);
			// We're done
			_state = ConversationCommand::EFinished;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'ActivateTarget' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;
	case ConversationCommand::ELookAtActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			float duration = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_LOOKAT_DURATION;
			owner->Event_LookAtEntity(ai, duration);
			_state = ConversationCommand::EFinished;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'LookAtActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::ELookAtPosition:
	{
		idVec3 pos = command.GetVectorArgument(0);
		float duration = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_LOOKAT_DURATION;

		owner->Event_LookAtPosition(pos, duration);
		_state = ConversationCommand::EFinished;
	}
	break;

	case ConversationCommand::ELookAtEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			float duration = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_LOOKAT_DURATION;
			owner->Event_LookAtEntity(ent, duration);
			_state = ConversationCommand::EFinished;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'LookAtEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;
	case ConversationCommand::ETurnToActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			owner->TurnToward(ai->GetEyePosition());
			_state = ConversationCommand::EFinished;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'TurnToActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::ETurnToPosition:
	{
		idVec3 pos = command.GetVectorArgument(0);
		owner->TurnToward(pos);
		_state = ConversationCommand::EFinished;
	}
	break;
	
	case ConversationCommand::ETurnToEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			owner->TurnToward(ent->GetPhysics()->GetOrigin());
			_state = ConversationCommand::EFinished;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'TurnToEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;
	
	case ConversationCommand::EAttackActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			owner->SetEnemy(ai);
			owner->SetAlertLevel(owner->thresh_5 + 1);
			_state = ConversationCommand::EFinished;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'AttackActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::EAttackEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL && ent->IsType(idActor::Type))
		{
			owner->SetEnemy(static_cast<idActor*>(ent));
			owner->SetAlertLevel(owner->thresh_5 + 1);
			_state = ConversationCommand::EFinished;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'AttackEntity' could not find entity or entity is of wrong type: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	default:
		gameLocal.Warning("Unknown command type found %d", command.GetType());
		DM_LOG(LC_CONVERSATION, LT_ERROR)LOGSTRING("Unknown command type found %d", command.GetType());
		_state = ConversationCommand::EAborted;
	};

	// Store the command type
	_commandType = command.GetType();
}

void ConversationState::Execute(ConversationCommand& command, Conversation& conversation)
{

}

int ConversationState::Talk(idAI* owner, const idStr& soundName)
{
	const idKeyValue* kv = owner->spawnArgs.FindKey(soundName);

	if (kv != NULL && kv->GetValue().Icmpn( "snd_", 4 ) == 0)
	{
		// The conversation argument is pointing to a valid spawnarg on the owner
		owner->spawnArgs.Set(CONVERSATION_SPAWNARG, kv->GetValue());
	}
	else
	{
		// The spawnargs don't define the sound shader, set the shader directly
		owner->spawnArgs.Set(CONVERSATION_SPAWNARG, soundName);
	}

	// Start the sound
	int length = owner->PlayAndLipSync(CONVERSATION_SPAWNARG, "talk1");

	// Clear the spawnarg again
	owner->spawnArgs.Set(CONVERSATION_SPAWNARG, "");

	return length;
}

void ConversationState::DrawDebugOutput(idAI* owner)
{
	if (!cv_ai_show_conversationstate.GetBool()) return;

	idStr str;

	switch (_state)
	{
		case ConversationCommand::ENotStartedYet: str = "Not Started Yet"; break;
		case ConversationCommand::EExecuting: str = "Executing"; break;
		case ConversationCommand::EFinished: str = "Finished"; break;
		case ConversationCommand::EAborted: str = "Aborted"; break;
		default:break;
	};

	gameRenderWorld->DrawText(str, owner->GetEyePosition() - idVec3(0,0,10), 0.3f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis, 1, 48);
}

void ConversationState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_conversation);
	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(static_cast<int>(_commandType));
	savefile->WriteInt(_finishTime);
}

void ConversationState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_conversation);

	int temp;
	savefile->ReadInt(temp);
	assert(temp >= 0 && temp <= ConversationCommand::ENumStates); // sanity check
	_state = static_cast<ConversationCommand::State>(temp);

	savefile->ReadInt(temp);
	assert(temp >= 0 && temp <= ConversationCommand::ENumCommands); // sanity check
	_commandType = static_cast<ConversationCommand::Type>(temp);

	savefile->ReadInt(_finishTime);
}

StatePtr ConversationState::CreateInstance()
{
	return StatePtr(new ConversationState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar conversationStateRegistrar(
	STATE_CONVERSATION, // Task Name
	StateLibrary::CreateInstanceFunc(&ConversationState::CreateInstance) // Instance creation callback
);

} // namespace ai
