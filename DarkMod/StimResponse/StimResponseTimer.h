/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef SR_RESPONSETIMER__H
#define SR_RESPONSETIMER__H

/*
#define GetHours(x)		((x >> 24) & 0xff)
#define GetMinutes(x)	((x >> 16) & 0xff)
#define GetSeconds(x)	((x >> 8) & 0xff)
#define GetMSeconds(x)	(x & 0xff)

#define SetHours(x)		(x << 24)
#define SetMinutes(x)	(x << 16)
#define SetSeconds(x)	(x << 8)
#define SetMSeconds(x)	(x)
*/

#define TIMER_UNDEFINED		-1

#pragma warning( push )
#pragma warning( disable : 4201 ) // non standard extension nameless struct/union
typedef union {
public:
	struct {
		signed char Flags;
		signed char Hour;
		signed char Minute;
		signed char Second;
		signed short Millisecond;
	};
	struct {
        signed long TimerVal;
		signed short Millisecond;
	};
} TimerValue;
#pragma warning( pop )

/**
 * CStimResponseTimer handles all timing aspects of stimuli.
 * Each of the values as a copy, which is used to store the actual value at runtime (<X>Val).
 * The original value is used for initialisation but is never changed during runtime
 * as only the Val counterparts will be used to track the current state.
 *
 * m_Timer is the actual timer that determines how long it takes before the stim
 * is getting enabled. After this timer is expired, the stim will be activated. 
 * If a duration timer is also set, it will fire as long as the duration lasts and will
 * be disabled this duration time. If no duration is specified, the timer will fire 
 * exactly once, and then reset itself if it is a restartable timer (SRTT_RELOAD) and 
 * the cycle will begin again. If a reload value is set, it will only be restarted 
 * until the reloadcounter has been depleted (0). If the reloadvalue is -1, it means 
 * that it will reset itself infinitely (or until manually stopped).
 */
class CStimResponseTimer {
friend CStim;
friend CStimResponseCollection;

public:
	typedef enum {
		SRTT_SINGLESHOT,			// Stimuls can be used exactly once
		SRTT_RELOAD,				// Stimulus can be reused
		SRTT_DEFAULT
	} TimerType;

	typedef enum {
		SRTS_DISABLED,				// Timer is not active
		SRTS_TRIGGERED,				// Stimuls just became active
		SRTS_RUNNING,				// Stimulus is progressing (only relevant for durations)
		SRTS_EXPIRED,				// Stimulus has been expired (can be reloaded of reload is set)
		SRTS_DEFAULT
	} TimerState;

public:
	/**
	 * If the stim contains information for a timed event, this function parses the string
	 * and returns a timervalue.
	 *
	 * The timer is initialized by a string on the entity which reads like this:
	 * HH:MM:SS
	 *
	 * HH are the hours in 24 hour format 0-23.
	 * MM are the minutes 0-59.
	 * SS are the seconds 0-59.
	 */
	static TimerValue ParseTimeString(idStr &s);

	virtual void SetTicks(double const &TicksPerSecond);
	virtual void SetTimer(int Hour, int Minute, int Seconds, int Milisecond);
	virtual void SetReload(int Reload);

	/**
	 * Start the timer again, after it has been stopped. If the timer 
	 * has been stopped before, but has not yet expired, it will
	 * just continue where it stopped which is different to Restart().
	 */
	virtual void Start(double const &t);

	/**
	 * Stop will simply stop the timer without any changes
	 */
	virtual void Stop(void);

	/**
	 * Restart will restart the timer with the next cycle. If a reload
	 * is specified it will be decreased, which means that if no more
	 * reloads are possible, restart will have no effect.
	 */
	virtual void Restart(double const &t);
	
	/**
	 * Reset will reset the timer. This means that also the reload
	 * value will be reset as well.
	 */
	virtual void Reset(void);

	void SetState(TimerState State);
	inline TimerState GetState(void) { return m_State; };

	/**
	 * The timer returns -1 if it is not working. Otherwise it will
	 * return the number of times it was triggered since the last
	 * time it was advanced. Usually this should be 1. If the number is
	 * consequently higher, it could mean that the machine is to slow
	 * to handle this timer frequency.
	 */
	virtual int Tick(double const &Ticks);

	/**
	 * Calculate the difference between two timervalues. This is usefull
	 * if you want a countdown instead of a normal clock. In this case, you
	 * can use the timer just like normal, but if you want to know the 
	 * state of the countdown just calculate the difference to your
	 * original value.
	 *
	 * The parameters are:
	 * A - B = Result
	 */
	void GetTimerValueDiff(TimerValue const &A, TimerValue const &B, TimerValue &Result) const;

protected:
	CStimResponseTimer();
	virtual ~CStimResponseTimer(void);

protected:
	double			m_LastTick;
	double			m_Ticker;
	double			m_TicksPerSecond;
	double			m_TicksPerMilliSecond;

	/**
	* The Timer type specifies if this is a single-use timer or a
	* "reloadable" timer.
	*/
	TimerType		m_Type;

	TimerState		m_State;

	/**
	 * How often can the stimulus be reused. -1 = unlimited
	 */
	int				m_Reload;
	int				m_ReloadVal;

	/**
	 * Timer
	 */
	TimerValue		m_Timer;
	TimerValue		m_TimerVal;
};

#endif /* SR_RESPONSETIMER__H */
