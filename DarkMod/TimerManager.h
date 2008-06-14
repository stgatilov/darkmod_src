/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

// Shortcut macros for code execution timing, use these (only!) to access the TimerManager class below
#ifdef TIMING_BUILD
  #define START_TIMING(id) debugtools::TimerManager::Instance().StartTimer(id);
  #define STOP_TIMING(id) debugtools::TimerManager::Instance().StopTimer(id);
  #define START_SCOPED_TIMING(id, varname) debugtools::ScopedTimer varname(id);
  #define CREATE_TIMER(outId, entityname, name) outId = debugtools::TimerManager::Instance().CreateTimer(entityname, name);
  #define INIT_TIMER_HANDLE(outId) outId = 0;
  #define SAVE_TIMER_HANDLE(id, savefilePtr) savefilePtr->WriteInt(id);
  #define RESTORE_TIMER_HANDLE(id, savefilePtr) savefilePtr->ReadInt(id);
  #define PRINT_TIMERS debugtools::TimerManager::Instance().PrintTimerResults();
#else
  #define START_TIMING(id)
  #define STOP_TIMING(id)
  #define START_SCOPED_TIMING(id, varname)
  #define CREATE_TIMER(outId, entityname, name)
  #define INIT_TIMER_HANDLE(outId)
  #define SAVE_TIMER_HANDLE(id, savefilePtr)
  #define RESTORE_TIMER_HANDLE(id, savefilePtr)
  #define PRINT_TIMERS
#endif

#ifdef TIMING_BUILD

namespace debugtools {

class TimerManager
{
	struct TimerInfo
	{
		idTimer timer;
		idStr entityName;
		idStr name;
		double runTime;	// total running time
		double maxTime;	// maximum time between start and stop
		int maxTimeCall; // the number of the call where maxTime was found
		int runCount;	
	};

public:
	void	Save(idSaveGame* savefile) const;
	void	Restore(idRestoreGame* savefile);

	int		CreateTimer(const idStr& entityname, const idStr& name);
	void	StartTimer(int timerId);
	void	StopTimer(int timerId);
	void	PrintTimerResults();
	void	Clear();


	// Contains the static singleton
	static TimerManager& Instance();

private:
	typedef std::map<int, TimerInfo> TimerMap;
	TimerMap _timers;
};

// Scoped object, stops timing at destruction
class ScopedTimer
{
	int _id;
public:
	ScopedTimer(int timerId) :
		_id(timerId)
	{
		TimerManager::Instance().StartTimer(_id);
	}

	~ScopedTimer()
	{
		TimerManager::Instance().StopTimer(_id);
	}
};

} // namespace debugtools

#endif /* ifdef TIMING_BUILD */

#endif /* TIMER_MANAGER_H */
