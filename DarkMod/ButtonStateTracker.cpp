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

class CButtonTrackerData {
public:
	bool mSupressRepeat;
	void *mData;			// arbitrary data that can be passed along
	int mHoldTime;
};


ButtonStateTracker::ButtonStateTracker(idPlayer* owner) :
	_owner(owner),
	_lastCheckTime(0)
{
}

void ButtonStateTracker::update()
{
	//DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("Updating button states\r");
	for (ButtonHoldTimeMap::iterator i = _buttons.begin(); i != _buttons.end(); /* in-loop increment */)
	{
		int impulse = i->first;
		int timeSinceLastCheck = gameLocal.time - _lastCheckTime;
		
		if (common->ButtonState(KEY_FROM_IMPULSE(impulse)))
		{
			// Key is still held down, increase the hold time 
			i->second += timeSinceLastCheck;

			_owner->PerformKeyRepeat(impulse, i->second);

			// Increase the iterator
			i++;
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

void ButtonStateTracker::startTracking(int impulse)
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
