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

#include "GreetingBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

const int MINIMUM_TIME_BETWEEN_GREETING_SAME_ACTOR = 3*60*1000; // 3 minutes

GreetingBarkTask::GreetingBarkTask() :
	SingleBarkTask()
{}

GreetingBarkTask::GreetingBarkTask(const idStr& soundName, idActor* greetingTarget) :
	SingleBarkTask(soundName),
	_greetingTarget(greetingTarget)
{}

// Get the name of this task
const idStr& GreetingBarkTask::GetName() const
{
	static idStr _name(TASK_GREETING_BARK);
	return _name;
}

void GreetingBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	SingleBarkTask::Init(owner, subsystem);

	// Check the prerequisites - are both AI available for greeting?
	if (owner->greetingState == ECannotGreet || 
		_greetingTarget->greetingState == ECannotGreet)
	{
		// Owner or other AI cannot do greetings
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Actors cannot great each other: %s to %s\r", owner->name.c_str(), _greetingTarget->name.c_str());
		subsystem.FinishTask();
		return;
	}

	if (owner->greetingState != ENotGreetingAnybody || 
		_greetingTarget->greetingState != ENotGreetingAnybody)
	{
		// Target is busy
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Cannot greet: one of the actors is busy: %s to %s\r", owner->name.c_str(), _greetingTarget->name.c_str());
		subsystem.FinishTask();
		return;
	}

	// Check the last time we greeted this AI
	int lastGreetingTime = owner->GetMemory().GetLastGreetingTime(_greetingTarget);

	if (lastGreetingTime > 0 && lastGreetingTime < gameLocal.time + MINIMUM_TIME_BETWEEN_GREETING_SAME_ACTOR)
	{
		// Too early
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Cannot greet: time since last greet too short: %s to %s, %d msecs\r", 
			owner->name.c_str(), _greetingTarget->name.c_str(), gameLocal.time - lastGreetingTime);
		
		subsystem.FinishTask();
		return;
	}
	
	// Both AI are not greeting each other so far, continue
	owner->greetingState = EGoingToGreet;
	_greetingTarget->greetingState = EWaitingForGreeting;
}

bool GreetingBarkTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("GreetingBarkTask performing.\r");

	// Let the SingleBarkTask do the timing and act upon the result
	bool done = SingleBarkTask::Perform(subsystem);

	// Set the Greeting State according to our base class' work
	idAI* owner = _owner.GetEntity();

	if (done)
	{
		owner->greetingState = EAfterGreeting;

		if (_greetingTarget != NULL && _greetingTarget->IsType(idAI::Type))
		{
			idAI* otherAI = static_cast<idAI*>(_greetingTarget);

			CommMessage message(
				CommMessage::Greeting_CommType, 
				owner, otherAI, // from this AI to the other
				NULL,
				owner->GetPhysics()->GetOrigin()
			);

			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Sending AI Comm Message to %s.\r", otherAI->name.c_str());
			otherAI->GetMind()->GetState()->OnAICommMessage(message, 0);
		}

		// Owner is done here
	}
	else if (_endTime < 0)
	{
		// Greeting not yet dispatched, waiting
		owner->greetingState = EGoingToGreet;
	}
	else 
	{
		// End time is set, we're currently barking
		owner->greetingState = EIsGreeting;

		// Remember the time we greeted this actor
		owner->GetMemory().SetLastGreetingTime(_greetingTarget, gameLocal.time);

		int timeLeft = _endTime - gameLocal.time;

		if (timeLeft > 32)
		{
			owner->Event_LookAtPosition(_greetingTarget->GetEyePosition(), MS2SEC(timeLeft));
		}
	}

	if (_barkStartTime > 0 && gameLocal.time > _barkStartTime + 50000)
	{
		gameLocal.Printf("Force termination of GreetingBarkTask after 50 seconds: %s.\n", owner->name.c_str());
		DM_LOG(LC_AI, LT_WARNING)LOGSTRING("Force termination of GreetingBarkTask after 50 seconds: %s.\r", owner->name.c_str());
		return true;
	}

	return done;
}

void GreetingBarkTask::OnFinish(idAI* owner)
{
	if (owner != NULL && owner->greetingState != ECannotGreet)
	{
		owner->greetingState = ENotGreetingAnybody;
	}
}

// Save/Restore methods
void GreetingBarkTask::Save(idSaveGame* savefile) const
{
	SingleBarkTask::Save(savefile);

	savefile->WriteObject(_greetingTarget);
}

void GreetingBarkTask::Restore(idRestoreGame* savefile)
{
	SingleBarkTask::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_greetingTarget));
}

GreetingBarkTaskPtr GreetingBarkTask::CreateInstance()
{
	return GreetingBarkTaskPtr(new GreetingBarkTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar greetingBarkTaskRegistrar(
	TASK_GREETING_BARK, // Task Name
	TaskLibrary::CreateInstanceFunc(&GreetingBarkTask::CreateInstance) // Instance creation callback
);

} // namespace ai
