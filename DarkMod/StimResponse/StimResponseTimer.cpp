/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 M�r 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: StimResponseTimer.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "StimResponseTimer.h"

/********************************************************************/
/*                 CStimResponseTimer                               */
/********************************************************************/
CStimResponseTimer::CStimResponseTimer()
{
	m_Type = SRTT_SINGLESHOT;
	m_State = SRTS_DISABLED;
	m_Reload = 0;
	m_ReloadVal = 0;
	m_Timer.A.Flags = TIMER_UNDEFINED;
	m_TimerVal.A.Flags = TIMER_UNDEFINED;
	m_LastTick = 0;
	m_Ticker = 0;
	m_TicksPerMilliSecond = 0;
}

CStimResponseTimer::~CStimResponseTimer(void)
{
}

void CStimResponseTimer::SetTicks(double const &TicksPerSecond)
{
	m_TicksPerMilliSecond = static_cast<unsigned long>(TicksPerSecond);
}

TimerValue CStimResponseTimer::ParseTimeString(idStr &str)
{
	TimerValue v;
	int h, m, s, ms;
	idStr source = str;

	v.A.Flags = TIMER_UNDEFINED;

	if(str.Length() == 0)
		goto Quit;

	h = m = s = ms = 0;

	// Get the first few characters that define the hours
	h = atoi( source.Left(source.Find(":")).c_str() );
	
	// Strip the first few numbers plus the colon from the source string
	source = source.Right(source.Length() - source.Find(":") - 1);
	
	// Parse the minutes
	m = atoi( source.Left(source.Find(":")).c_str() );
	if (!(m >= 0 && m <= 59))
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid minute string [%s]\r", str.c_str());
		goto Quit;
	}
	// Strip the first few numbers plus the colon from the source string
	source = source.Right(source.Length() - source.Find(":") - 1);
	
	// Parse the seconds
	s = atoi( source.Left(source.Find(":")).c_str() );
	if (!(s >= 0 && s <= 59))
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid second string [%s]\r", str.c_str());
		goto Quit;
	}

	// Parse the milliseconds, this is the remaining part of the string
	ms = atoi( source.Right(source.Length() - source.Find(":") - 1).c_str() );
	if (!(ms >= 0 && ms <= 999))
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid millisecond string [%s]\r", str.c_str());
		goto Quit;
	}

	DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Parsed timer string: [%s] to %d:%d:%d:%d\r", str.c_str(), h, m, s, ms);

	v.A.Hour = h;
	v.A.Minute = m;
	v.A.Second = s;
	v.A.Millisecond = ms;

Quit:
	return v;
}

void CStimResponseTimer::SetReload(int Reload)
{
	m_Reload = Reload;
	m_ReloadVal = Reload;
}

void CStimResponseTimer::SetTimer(int Hour, int Minute, int Second, int Millisecond)
{
//	m_Timer = SetHours(Hour) |  SetMinutes(Minute) | SetSeconds(Seconds) | SetMSeconds(Milisecond);
	m_TimerVal.A.Hour = Hour;
	m_TimerVal.A.Minute = Minute;
	m_TimerVal.A.Second = Second;
	m_TimerVal.A.Millisecond = Millisecond;
	memset(&m_Timer, 0, sizeof(TimerValue));
}

void CStimResponseTimer::Stop(void)
{
	SetState(SRTS_DISABLED);
}

void CStimResponseTimer::Start(unsigned long sysTicks)
{
	m_LastTick = sysTicks;
	SetState(SRTS_RUNNING);
}

void CStimResponseTimer::Restart(unsigned long sysTicks)
{
	// Switch to the next timer cycle if reloading is still possible or 
	// reloading is ignored.
	m_Ticker = sysTicks;

	if(m_Reload > 0 || m_Reload == -1)
	{
		memset(&m_Timer, 0, sizeof(TimerValue));
		m_Reload--;
		Start(sysTicks);
	}
	else
		Stop();
}

void CStimResponseTimer::Reset(void)
{
	memset(&m_Timer, 0, sizeof(TimerValue));
	m_Reload = m_ReloadVal;
}

void CStimResponseTimer::SetState(TimerState State)
{
	m_State = State;
}

bool CStimResponseTimer::Tick(unsigned long sysTicks)
{
	bool returnValue = false;
	unsigned long ticksPassed;
	double msPassed;
	
	if(m_State != SRTS_RUNNING)
		goto Quit;

	// We don't really care for an overrun of the ticckcounter. If 
	// it really happens, the worst thing would be that a particular
	// timer object would take longer to complete, because for this
	// one cycle, the tick would become negative and thus would subtract
	// the value instead of adding it. In the next cylce, everything 
	// should work again though, since we always store the current
	// value to remember it for the next cycle.
	/* unsigned long */ ticksPassed = sysTicks - m_LastTick;
	
	// If the overrun happened, we just ignore this tick. It's the easiest
	// thing to do and the safest.
	if (ticksPassed < 0.0)
		goto Quit;

	m_Ticker += ticksPassed;

	// Calculate the number of milliseconds that have passed since the last visit
	/* double */ msPassed = floor(static_cast<double>(m_Ticker) / m_TicksPerMilliSecond);
	
	// The remaining ticks are what's left after the division (modulo)
	m_Ticker %= m_TicksPerMilliSecond;

	// Increase the hours/minutes/seconds/milliseconds
	m_Timer.A.Millisecond += msPassed;

	if (m_Timer.A.Millisecond > 999) {
		// Increase the seconds
		m_Timer.A.Second += floor(static_cast<double>(m_Timer.A.Millisecond) / 1000);
		m_Timer.A.Millisecond %= 1000;

		m_Timer.A.Minute += floor(static_cast<double>(m_Timer.A.Second) / 60);
		m_Timer.A.Second %= 60;

		m_Timer.A.Hour += floor(static_cast<double>(m_Timer.A.Minute) / 60);
		m_Timer.A.Minute %= 60;
	}

	// Now check if the timer already expired.
	if (m_Timer.A.Hour >= m_TimerVal.A.Hour && m_Timer.A.Minute >= m_TimerVal.A.Minute && 
		m_Timer.A.Second >= m_TimerVal.A.Second && m_Timer.A.Millisecond >= m_TimerVal.A.Millisecond) 
	{
		returnValue = true;
		if(m_Type == SRTT_SINGLESHOT) {
			Stop();
		}
		else {
			Restart(sysTicks);
		}
	}

Quit:
	m_LastTick = sysTicks;

	return returnValue;
}
