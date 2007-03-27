/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
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
	m_Timer.Flags = TIMER_UNDEFINED;
	m_TimerVal.Flags = TIMER_UNDEFINED;
	m_LastTick = 0.0;
	m_Ticker = 0.0;
	m_TicksPerSecond = 0.0;
	m_TicksPerMilliSecond = 0.0;
}

CStimResponseTimer::~CStimResponseTimer(void)
{
}

void CStimResponseTimer::SetTicks(double const &TicksPerSecond)
{
	m_TicksPerSecond = TicksPerSecond;
	m_TicksPerMilliSecond = TicksPerSecond/1000.0;
}

TimerValue CStimResponseTimer::ParseTimeString(idStr &str)
{
	TimerValue v;
	int i, h, m, s, ms;

	v.Flags = TIMER_UNDEFINED;

	if(str.Length() == 0)
		goto Quit;

	if(str[2] != ':' && str[5]  != ':')
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid timer string [%s]\r", str.c_str());
		goto Quit;
	}

	h = m = s = ms = 0;
	for(i = 0; i < 4; i++)
	{
		switch(i)
		{
			case 0:
			{
				h = atoi(str.c_str());
				if(!(h >= 0 && h <= 23))
				{
					DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid hour string [%s]\r", str.c_str());
					goto Quit;
				}
			}
			break;

			case 1:
			{
				m = atoi(str.c_str());
				if(!(m >= 0 && m <= 59))
				{
					DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid minute string [%s]\r", str.c_str());
					goto Quit;
				}
			}
			break;

			case 2:
			{
				s = atoi(str.c_str());
				if(!(s >= 0 && s <= 59))
				{
					DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid second string [%s]\r", str.c_str());
					goto Quit;
				}
			}
			break;

			case 3:
			{
				ms = atoi(str.c_str());
				if(!(ms >= 0 && ms <= 999))
				{
					DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid millisecond string [%s]\r", str.c_str());
					goto Quit;
				}
			}
			break;
		}
	}

	v.Hour = h;
	v.Minute = m;
	v.Second = s;
	v.Millisecond = ms;

//	v = SetHours(h) + SetMinutes(m) + SetSeconds(s);

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
	m_TimerVal.Hour = Hour;
	m_TimerVal.Minute = Minute;
	m_TimerVal.Second = Second;
	m_TimerVal.Millisecond = Millisecond;
	memset(&m_Timer, 0, sizeof(TimerValue));
}

void CStimResponseTimer::Stop(void)
{
	SetState(SRTS_DISABLED);
}

void CStimResponseTimer::Start(double const &t)
{
	m_LastTick = t;
	SetState(SRTS_RUNNING);
}

void CStimResponseTimer::Restart(double const &t)
{
	// Switch to the next timer cycle if reloading is still possible or 
	// reloading is ignored.
	m_Ticker = t;

	if(m_Reload > 0 || m_Reload == -1)
	{
		memset(&m_Timer, 0, sizeof(TimerValue));
		m_Reload--;
		Start(t);
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

void CStimResponseTimer::GetTimerValueDiff(TimerValue const &A, TimerValue const &B, TimerValue &rc) const
{
}


int CStimResponseTimer::Tick(double const &t)
{
	int rc = -1;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("this: %08lX %s\r", this, __FUNCTION__);

	if(m_State != SRTS_RUNNING)
		goto Quit;

	rc = 0;

	// We don't really care for an overrun of the ticckcounter. If 
	// it really happens, the worst thing would be that a particular
	// timer object would take longer to complete, because for this
	// one cycle, the tick would become negative and thus would subtract
	// the value instead of adding it. In the next cylce, everything 
	// should work again though, since we always store the current
	// value to remember it for the next cycle.
	double tick = t - m_LastTick;

	// If the overrun happened, we just ignore this tick. It's the easiest
	// thing to do and the safest.
	if(tick < 0.0)
		goto Quit;

	m_Ticker =+ tick;

	// It could be possible that one haertbeat took longer than one millisecond.
	// In this case we loop and advance the timer for each millisecond that 
	// expired. The expectation is, that this will not happen most of the time
	// of if it does, then mostly on slow machines.
	while(m_Ticker > m_TicksPerMilliSecond)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Millisecs triggered: %f/%f\r", m_TicksPerMilliSecond, m_Ticker);

		m_Ticker -= m_TicksPerMilliSecond;
		m_Timer.Millisecond++;
		if(m_Timer.Millisecond > 999)
		{
			m_Timer.Millisecond = 0;
			m_Timer.Second++;

			if(m_Timer.Second > 59)
			{
				m_Timer.Second = 0;
				m_Timer.Minute++;

				if(m_Timer.Minute > 59)
				{
					m_Timer.Minute = 0;
					m_Timer.Hour++;
				}
			}
		}

		// Now check if the timer already expired.
		if(m_Timer.Millisecond >= m_TimerVal.Millisecond)
		{
			if(m_Timer.Second >= m_TimerVal.Second)
			{
				if(m_Timer.Minute >= m_TimerVal.Minute)
				{
					if(m_Timer.Hour >= m_TimerVal.Hour)
					{
						rc++;
						if(m_Type == SRTT_SINGLESHOT)
							Stop();
						else
							Restart(t);

						break;
					}
				}
			}
		}
	}


Quit:
	m_LastTick = t;

	return rc;
}
