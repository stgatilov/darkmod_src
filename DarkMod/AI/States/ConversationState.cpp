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
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/PlayAnimationTask.h"
#include "../Tasks/InteractionTask.h"
#include "../Tasks/ScriptTask.h"
#include "ObservantState.h"
#include "../Library.h"
#include "../Conversation/Conversation.h"
#include "../Conversation/ConversationSystem.h"
#include "../Conversation/ConversationCommand.h"

// greebo: This spawnarg holds the currently played conversation sound
#define CONVERSATION_SPAWNARG "snd_TEMP_conv"
#define DEFAULT_LOOKAT_DURATION 5.0f
#define DEFAULT_WALKTOENTITY_DISTANCE 50.0f
#define FALLBACK_ANIM_LENGTH 5000 // msecs
#define DEFAULT_BLEND_FRAMES 4

namespace ai
{

ConversationState::ConversationState() :
	_conversation(-1),
	_state(ENotReady),
	_commandType(ConversationCommand::ENumCommands),
	_finishTime(-1)
{}

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
	if (gameLocal.m_ConversationSystem->GetConversation(index) == NULL)
	{
		gameLocal.Warning("AI ConversationState: Could not find conversation %d\n", index);
	}

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
		owner->GetMind()->EndState();
		return;
	}

	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->StopMove(MOVE_STATUS_DONE);

	ConversationPtr conversation = gameLocal.m_ConversationSystem->GetConversation(_conversation);
	if (conversation == NULL)
	{
		owner->GetMind()->EndState();
		return;
	}

	// We're initialised, so let's set this state to ready
	_state = EReady;

	// Check the conversation property to see if we should move before we are ready
	if (conversation->ActorsMustBeWithinTalkdistance())
	{
		// Not ready yet
		_state = ENotReady;

		idEntity* targetActor = NULL;

		// Get the first actor who is not <self>
		for (int i = 0; i < conversation->GetNumActors(); i++)
		{
			idEntity* candidate = conversation->GetActor(i);
			if (candidate != owner)
			{
				targetActor = candidate;
				break;
			}
		}

		if (targetActor != NULL)
		{
			float talkDistance = conversation->GetTalkDistance();

			owner->GetSubsystem(SubsysMovement)->PushTask(
				TaskPtr(new MoveToPositionTask(targetActor, talkDistance))
			);
		}
	}
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
		// Allow new incoming commands
		_state = EReady;

		// Reset the finish time
		_finishTime = -1;
	}

	DrawDebugOutput(owner);
}

ConversationState::ExecutionState ConversationState::GetExecutionState()
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
	if (_state != EExecuting && _state != ENotReady && _state != EBusy) return;

	if (subSystem == SubsysMovement)
	{
		// greebo: Are we still in preparation phase?
		if (_state == ENotReady)
		{
			// The movement task has ended, set the state to ready
			_state = EReady;
			return;
		}

		// In case of active "walk" commands, set the state to ready
		if (_commandType == ConversationCommand::EWalkToEntity || 
			_commandType == ConversationCommand::EWalkToPosition || 
			_commandType == ConversationCommand::EWalkToActor)
		{
			_state = EReady; // ready for new commands
			return;
		}
	}
	else if (subSystem == SubsysAction)
	{
		// In case of active "Interact" commands, set the state to "finished"
		if (_commandType == ConversationCommand::EInteractWithEntity || _commandType == ConversationCommand::ERunScript)
		{
			_state = EReady;
			return;
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
		_state = EBusy; // block new commands until finished
	break;

	case ConversationCommand::EWalkToActor:
	{
		// Reduce the actor index by 1 before passing them to the conversation
		idAI* ai = conversation.GetActor(atoi(command.GetArgument(0)) - 1);

		if (ai != NULL)
		{
			float distance = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_WALKTOENTITY_DISTANCE;
			
			owner->GetSubsystem(SubsysMovement)->PushTask(
				TaskPtr(new MoveToPositionTask(ai, distance))
			);

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'WalkToActor' could not find actor: %s", command.GetArgument(0).c_str());
		}
	}
	break;
	case ConversationCommand::EWalkToPosition:
	{
		idVec3 goal = command.GetVectorArgument(0);

		// Start moving
		owner->GetSubsystem(SubsysMovement)->PushTask(
			TaskPtr(new MoveToPositionTask(goal))
		);

		// Check if we should wait until the command is finished and set the _state accordingly
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::EWalkToEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);
		if (ent != NULL)
		{
			float distance = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_WALKTOENTITY_DISTANCE;
			
			owner->GetSubsystem(SubsysMovement)->PushTask(
				TaskPtr(new MoveToPositionTask(ent, distance))
			);

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'WalkToEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::EStopMove:
		owner->StopMove(MOVE_STATUS_DONE);
		_state = EReady;
		break;

	case ConversationCommand::ETalk:
	{
		int length = Talk(owner, command.GetArgument(0));

		// Check if we need to look at the listener
		if (conversation.ActorsAlwaysFaceEachOtherWhileTalking())
		{
			idAI* talker = owner;
			
			for (int i = 0; i < conversation.GetNumActors(); i++)
			{
				if (i != command.GetActor())
				{
					// Listeners turn towards the talker
					// Reduce the actor index by 1 before passing them to the conversation
					idAI* listener = conversation.GetActor(i);

					listener->TurnToward(owner->GetEyePosition());
					listener->Event_LookAtPosition(owner->GetEyePosition(), MS2SEC(length));
				}
				else
				{
					// The talker should turn to any other listener

					// Are there any other actors at all?
					if (conversation.GetNumActors() > 1) 
					{
						idAI* listener = (i == 0) ? conversation.GetActor(1) : conversation.GetActor(0);

						talker->TurnToward(listener->GetEyePosition());
						talker->Event_LookAtPosition(listener->GetEyePosition(), MS2SEC(length));
					}
				}
			}
		}
		
		_finishTime = gameLocal.time + length + 200;
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::EPlayAnimOnce:
	{
		idStr animName = command.GetArgument(0);
		int blendFrames = (command.GetNumArguments() >= 2) ? atoi(command.GetArgument(1)) : DEFAULT_BLEND_FRAMES;

		// Tell the animation subsystem to play the anim
		owner->GetSubsystem(SubsysAction)->PushTask(
			TaskPtr(new PlayAnimationTask(animName, blendFrames))
		);

		int length = (owner->GetAnimator() != NULL) ? 
			owner->GetAnimator()->AnimLength(owner->GetAnimator()->GetAnim(animName)) : FALLBACK_ANIM_LENGTH;

		// Set the finish conditions for the current action
		_finishTime = gameLocal.time + length;
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::EPlayAnimCycle:
	{
		idStr animName = command.GetArgument(0);
		int blendFrames = (command.GetNumArguments() >= 2) ? atoi(command.GetArgument(1)) : DEFAULT_BLEND_FRAMES;

		// Tell the animation subsystem to play the anim
		owner->GetSubsystem(SubsysAction)->PushTask(
			TaskPtr(new PlayAnimationTask(animName, blendFrames, true)) // true == playCycle
		);

		// For PlayCycle, "wait until finished" doesn't make sense, as it lasts forever
		_state = EReady;
	}
	break;

	case ConversationCommand::EActivateTarget:
	{
		idEntity* ent = command.GetEntityArgument(0);
		if (ent != NULL)
		{
			// Post a trigger event
			ent->PostEventMS(&EV_Activate, 0, owner);
			// We're done
			_state = EReady;
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

			_finishTime = gameLocal.time + SEC2MS(duration);
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
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

		_finishTime = gameLocal.time + SEC2MS(duration);
		_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
	}
	break;

	case ConversationCommand::ELookAtEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			float duration = (command.GetNumArguments() >= 2) ? command.GetFloatArgument(1) : DEFAULT_LOOKAT_DURATION;
			owner->Event_LookAtEntity(ent, duration);
			
			_finishTime = gameLocal.time + SEC2MS(duration);
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
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
			_state = EReady;
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
		_state = EReady;
	}
	break;
	
	case ConversationCommand::ETurnToEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			owner->TurnToward(ent->GetPhysics()->GetOrigin());
			_state = EReady;
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
			_state = EReady;
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
			_state = EReady;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'AttackEntity' could not find entity or entity is of wrong type: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::EInteractWithEntity:
	{
		idEntity* ent = command.GetEntityArgument(0);

		if (ent != NULL)
		{
			// Tell the action subsystem to do its job
			owner->GetSubsystem(SubsysAction)->PushTask(
				TaskPtr(new InteractionTask(ent))
			);

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'InteractWithEntity' could not find entity: %s", command.GetArgument(0).c_str());
		}
	}
	break;

	case ConversationCommand::ERunScript:
	{
		idStr scriptFunction = command.GetArgument(0);

		if (!scriptFunction.IsEmpty())
		{
			// Tell the action subsystem to do its job
			owner->GetSubsystem(SubsysAction)->PushTask(
				TaskPtr(new ScriptTask(scriptFunction))
			);

			// Check if we should wait until the command is finished and set the _state accordingly
			_state = (command.WaitUntilFinished()) ? EBusy : EExecuting;
		}
		else
		{
			gameLocal.Warning("Conversation Command: 'RunScript' has empty scriptfunction argument 0.");
		}
	}
	break;

	default:
		gameLocal.Warning("Unknown command type found %d", command.GetType());
		DM_LOG(LC_CONVERSATION, LT_ERROR)LOGSTRING("Unknown command type found %d", command.GetType());
		_state = EReady;
	};

	// Store the command type
	_commandType = command.GetType();
}

void ConversationState::ProcessCommand(ConversationCommand& command)
{
	ConversationPtr conversation = gameLocal.m_ConversationSystem->GetConversation(_conversation);

	if (conversation == NULL) return;

	// Check the incoming command
	ConversationCommand::State cmdState = command.GetState();

	if (cmdState == ConversationCommand::EReadyForExecution)
	{
		// This is a new command, are we ready for it?
		if (_state == EReady || _state == EExecuting)
		{
			// Yes, we're able to handle new commands
			StartCommand(command, *conversation);
		}
		else
		{
			// Not ready for new commands yet, wait...
		}
	}
	else if (cmdState == ConversationCommand::EExecuting)
	{
		// We are already executing this command, continue
		Execute(command, *conversation);
	}
	else
	{
		// Ignore the other cases
	}

	// Now update the command state, based on our execution state
	switch (_state)
	{
		case ENotReady:
			// not ready yet, state is still preparing for takeoff
			// don't change the command
			break;
		case EReady:
			command.SetState(ConversationCommand::EFinished);
			break;
		case EExecuting:
			// We're executing the command, but it's basically done.
			command.SetState(ConversationCommand::EFinished);
			break;
		case EBusy:
			// We're executing the command, and must wait until it's finished, set it to "executing"
			command.SetState(ConversationCommand::EExecuting);
			break;
		default:
			// Unknown state?
			gameLocal.Warning("Unknown execution state found: %d", static_cast<int>(_state));
			// Set the command to finished anyway, to avoid blocking
			command.SetState(ConversationCommand::EFinished);
			break;
	};
}

void ConversationState::Execute(ConversationCommand& command, Conversation& conversation)
{
	// Nothing to do so far.
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
		case ENotReady: str = "Not Ready"; break;
		case EReady: str = "Ready"; break;
		case EExecuting: str = "Executing"; break;
		case EBusy: str = "Busy"; break;
		default:break;
	};

	gameRenderWorld->DrawText(str, owner->GetEyePosition() - idVec3(0,0,20), 0.25f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis, 1, 48);

	str = (_commandType < ConversationCommand::ENumCommands) ? ConversationCommand::TypeNames[_commandType] : "";
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
	assert(temp >= 0 && temp <= ENumExecutionStates); // sanity check
	_state = static_cast<ExecutionState>(temp);

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
