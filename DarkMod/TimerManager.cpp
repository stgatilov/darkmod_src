/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2443 $
 * $Date: 2008-06-07 15:48:49 +0200 (Sa, 07 Jun 2008) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#ifdef TIMING_BUILD

#include "TimerManager.h"

namespace debugtools {

void TimerManager::Save(idSaveGame* savefile) const
{
	int num = _timers.size();
	savefile->WriteInt(num);
	for (TimerMap::const_iterator iterator = _timers.begin(); iterator != _timers.end(); iterator++)
	{
		savefile->WriteInt(iterator->first);

		savefile->WriteString(iterator->second.entityName);
		savefile->WriteString(iterator->second.name);
		savefile->WriteInt(iterator->second.runCount);
		savefile->WriteFloat(static_cast<float>(iterator->second.runTime));
		savefile->WriteFloat(static_cast<float>(iterator->second.maxTime));
		savefile->WriteInt(iterator->second.maxTimeCall);
	}
}

void TimerManager::Restore(idRestoreGame* savefile)
{
	int num;
	savefile->ReadInt(num);
	for (int i = 1; i <= num; i++)
	{
		int timerId;
		savefile->ReadInt(timerId);
		TimerInfo info;
		savefile->ReadString(info.entityName);
		savefile->ReadString(info.name);
		savefile->ReadInt(info.runCount);
		float temp;
		savefile->ReadFloat(temp);
		info.runTime = static_cast<double>(temp);
		savefile->ReadFloat(temp);
		info.maxTime = static_cast<double>(temp);
		savefile->ReadInt(info.maxTimeCall);

		_timers.insert(TimerMap::value_type(timerId, info));
	}
}


int	TimerManager::CreateTimer(const idStr& entityname, const idStr& name)
{
	int timerId = _timers.size() + 1;

	TimerInfo info;
	info.entityName = entityname;
	info.name = name;
	info.runCount = 0;
	info.runTime = 0;
	info.maxTime = 0;
	info.maxTimeCall = 0;

	_timers.insert(TimerMap::value_type(timerId, info));

	return timerId;
}

void TimerManager::StartTimer(int timerId)
{
	TimerMap::iterator found = _timers.find(timerId);
	assert(found != _timers.end());

	TimerInfo& info = found->second;
	info.timer.Clear();
	info.timer.Start();

}

void TimerManager::StopTimer(int timerId)
{
	TimerMap::iterator found = _timers.find(timerId);
	assert(found != _timers.end());

	TimerInfo& info = found->second;
	info.timer.Stop();
	info.runCount++;
	info.runTime += info.timer.Milliseconds();
	if (info.timer.Milliseconds() > info.maxTime)
	{
		info.maxTime = info.timer.Milliseconds();
		info.maxTimeCall = info.runCount;
	}
	info.timer.Clear();
}


void TimerManager::PrintTimerResults()
{
	for (TimerMap::iterator iterator = _timers.begin(); iterator != _timers.end(); iterator++)
	{
		TimerInfo& info = iterator->second;
		gameLocal.Printf("%s %s \n", info.entityName.c_str(), info.name.c_str());
		gameLocal.Printf("Number of calls: %d\n", info.runCount);
		gameLocal.Printf("Total run time: %lf ms\n", info.runTime);
		gameLocal.Printf("Mean run time per call: %lf ms\n", info.runTime / info.runCount);
		gameLocal.Printf("Max runtime: %lf ms at call number %d\n", info.maxTime, info.maxTimeCall);
		gameLocal.Printf("---------------------------------\n");
	}
}

void Timermanager::Clear()
{
	_timers.clear();
}

TimerManager& TimerManager::Instance() 
{
	static TimerManager _manager;
	return _manager;
}

} // namespace debugtools

#endif /* ifdef TIMING_BUILD */
