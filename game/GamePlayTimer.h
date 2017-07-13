/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

#ifndef __GAMEPLAY_TIMER_H__
#define __GAMEPLAY_TIMER_H__

//#include "precompiled.h"
#include <ctime>

/**
 * greebo: This class keeps track of the total gameplay time. Just call Update()
 * in regular intervals and the class checks the time difference since the last update call.
 *
 * For saving and loading, this class provides separate routines.
 *
 * duzenko #4409: added a high-preciosion timer to drive game time under com_fixedTic
 */
class GamePlayTimer
{
private:
	std::time_t _lastTime;
	std::time_t _curTime;

	// msec timer stuff
	uint64_t _lastMsec;
	uint64_t _curMsec;
	int _lastTick;

	// The passed time in seconds
	uint32_t _timePassed;
	uint32_t _msecPassed;

	// TRUE if the timer updates the passed time
	bool _enabled;

	void msec(uint64_t *msec) {
		*msec = Sys_GetTimeMicroseconds() / 1000;
	}

public:
	GamePlayTimer() :
		_timePassed(0),
		_msecPassed(0)
	{}

	// Defines the starting point
	void Start()
	{
		// Remember this time as starting point
		std::time(&_lastTime);
		msec(&_curMsec);

		SetEnabled(true);
	}

	void Stop() 
	{
		SetEnabled(false);
	}

	bool IsEnabled() const
	{
		return _enabled;
	}

	void SetEnabled(bool enabled)
	{
		_enabled = enabled;
	}

	void Clear()
	{
		_timePassed = 0;
		_msecPassed = 0;

		std::time(&_lastTime);
		msec(&_curMsec);
	}

	void Update()
	{
		if (!_enabled) 
		{
			return;
		}

		// Get the current time and calculate the difference
		std::time(&_curTime);

		// Increase the time that has passed
		_timePassed += _curTime - _lastTime;

		// Remember this last check time
		_lastTime = _curTime;

		_lastMsec = _curMsec;
		msec(&_curMsec);
		assert(_curMsec >= _lastMsec);
		_msecPassed += _lastTick = uint32_t(_curMsec - _lastMsec);
	}

	idStr GetTime() const {
		return TimeToStr(_timePassed);
	}

	// Returns the gameplay time in seconds
	uint32_t GetTimeInSeconds() const
	{
		return _timePassed;
	}

	// Returns diff between last two updates in milliseconds, capped to avoid physics glitches
	uint32_t LastTickCapped() const
	{
		if (_lastTick < 0)
			return 0;
		else
			if (_lastTick < 50)
				return _lastTick;
		return 50;
	}

	void Save(idSaveGame *savefile) const
	{
#if 1
		savefile->WriteUnsignedInt(_msecPassed);
#else
		savefile->WriteUnsignedInt(_timePassed);
#endif
		savefile->WriteBool(_enabled);
	}

	void Restore(idRestoreGame *savefile)
	{
#if 1
		savefile->ReadUnsignedInt(_msecPassed);
		_timePassed = _msecPassed / 1000;
#else
		savefile->ReadUnsignedInt(_timePassed);
#endif
		savefile->ReadBool(_enabled);
	}

	// Formats the given gameplay time
	static idStr TimeToStr(unsigned int time) {
		unsigned int hours = static_cast<unsigned int>(idMath::Floor(time / 3600.0f));
		unsigned int minutes = time % 3600;
		minutes = static_cast<unsigned int>(idMath::Floor(minutes / 60.0f));
		unsigned int seconds = time % 60;

		return va("%02d:%02d:%02d", hours, minutes, seconds);
	}
};

#endif /* __GAMEPLAY_TIMER_H__ */
