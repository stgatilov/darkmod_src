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

static bool init_version = FileVersionList("$Id: Stim.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "Stim.h"

#include "StimResponseTimer.h"

/********************************************************************/
/*                     CStim                                        */
/********************************************************************/
CStim::CStim(idEntity *e, int Type)
: CStimResponse(e, Type)
{
	m_bUseEntBounds = false;
	m_TimeInterleave = 0;
	m_TimeInterleaveStamp = 0;
	m_Radius = 0.0;
	m_FallOffExponent = 0;
	m_Magnitude = 0.0;
	m_MaxResponses = 0;
	m_CurResponses = 0;
	m_ApplyTimer = 0;
	m_ApplyTimerVal = 0;
	m_MaxFireCount = -1;
}

CStim::~CStim(void)
{
	gameLocal.m_StimTimer.Remove(this);
}

void CStim::AddResponseIgnore(idEntity *e)
{
	if(CheckResponseIgnore(e) != true)
		m_ResponseIgnore.Append(e);
}

void CStim::RemoveResponseIgnore(idEntity *e)
{
	m_ResponseIgnore.Remove(e);
}

bool CStim::CheckResponseIgnore(idEntity *e)
{
	bool rc = false;
	int i, n;

	n = m_ResponseIgnore.Num();
	for(i = 0; i < n; i++)
	{
		if(m_ResponseIgnore[i] == e)
		{
			rc = true;
			break;
		}
	}

	return rc;
}


CStimResponseTimer* CStim::AddTimerToGame(void)
{
	gameLocal.m_StimTimer.AddUnique(this);
	m_Timer.SetTicks(sys->ClockTicksPerSecond());

	return(&m_Timer);
}

void CStim::RemoveTimerFromGame(void)
{
	gameLocal.m_StimTimer.Remove(this);
}

void CStim::PostFired (int numResponses)
{
}
