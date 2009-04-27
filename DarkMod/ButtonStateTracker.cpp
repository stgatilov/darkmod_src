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

#include "ButtonStateTracker.h"

ButtonStateTracker::ButtonStateTracker(idPlayer* owner) :
	_owner(owner),
	_lastCheckTime(0)
{}

void ButtonStateTracker::Update()
{
	int timeSinceLastCheck = gameLocal.time - _lastCheckTime;

	if (timeSinceLastCheck == 0) return; // no double-checking in the same frame

	//DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("Updating button states\r");
	for (ButtonHoldTimeMap::iterator i = _buttons.begin(); i != _buttons.end(); /* in-loop increment */)
	{
		int impulse = i->first;
		
		if (common->ButtonState(KEY_FROM_IMPULSE(impulse)))
		{
			// Key is still held down, increase the hold time 
			i->second += timeSinceLastCheck;

			_owner->PerformKeyRepeat(impulse, i->second);

			// Increase the iterator
			++i;
		}
		else
		{
			int holdTime = i->second + timeSinceLastCheck;
			
			// Delete the impulse from the map, and increase the iterator immediately afterwards
			_buttons.erase(i++);

			// Notify the player class about the keyrelease event
			_owner->PerformKeyRelease(impulse, holdTime);
		}
	}

	// Remember the last time the buttons have been checked
	_lastCheckTime = gameLocal.time;
}

void ButtonStateTracker::StartTracking(int impulse)
{
	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("Impulse registered for tracking: %d\r", impulse);
	// Initialise the given impulse with a hold time of zero
	_buttons[impulse] = 0;

	if (_lastCheckTime == 0)
	{
		// Initialise the last check time, as it is 0 up to now
		_lastCheckTime = gameLocal.time;
	}
}

bool ButtonStateTracker::ButtonIsHeld(int impulse)
{
	return (common->ButtonState(KEY_FROM_IMPULSE(impulse)) != 0);
}

void ButtonStateTracker::StopTracking(int impulse)
{
	ButtonHoldTimeMap::iterator i = _buttons.find(impulse);

	if (i != _buttons.end())
	{
		_buttons.erase(i);
	}
}
