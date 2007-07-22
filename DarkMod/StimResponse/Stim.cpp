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
	m_bCollisionBased = false;
	m_bCollisionFired = false;
	m_CollisionEnts.Clear();
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
	m_Bounds.Zero();
	m_Velocity = idVec3(0,0,0);
}

CStim::~CStim(void)
{
	gameLocal.m_StimTimer.Remove(this);
}

void CStim::Save(idSaveGame *savefile) const
{
	CStimResponse::Save(savefile);

	m_Timer.Save(savefile);

	savefile->WriteInt(m_ResponseIgnore.Num());
	for (int i = 0; i < m_ResponseIgnore.Num(); i++)
	{
		m_ResponseIgnore[i].Save(savefile);
	}

	savefile->WriteBool(m_bUseEntBounds);
	savefile->WriteBool(m_bCollisionBased);
	savefile->WriteBool(m_bCollisionFired);

	// Don't save collision ents (probably not required)
	
	savefile->WriteInt(m_TimeInterleave);
	savefile->WriteInt(m_TimeInterleaveStamp);
	savefile->WriteInt(m_MaxFireCount);
	savefile->WriteFloat(m_Radius);
	savefile->WriteBounds(m_Bounds);
	savefile->WriteVec3(m_Velocity);
	savefile->WriteFloat(m_Magnitude);
	savefile->WriteInt(m_FallOffExponent);
	savefile->WriteInt(m_MaxResponses);
	savefile->WriteInt(m_CurResponses);
	savefile->WriteInt(m_ApplyTimer);
	savefile->WriteInt(m_ApplyTimerVal);
}

void CStim::Restore(idRestoreGame *savefile)
{
	CStimResponse::Restore(savefile);

	m_Timer.Restore(savefile);

	int num;
	savefile->ReadInt(num);
	m_ResponseIgnore.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_ResponseIgnore[i].Restore(savefile);
	}

	savefile->ReadBool(m_bUseEntBounds);
	savefile->ReadBool(m_bCollisionBased);
	savefile->ReadBool(m_bCollisionFired);

	// Don't restore collision ents (probably not required)
	
	savefile->ReadInt(m_TimeInterleave);
	savefile->ReadInt(m_TimeInterleaveStamp);
	savefile->ReadInt(m_MaxFireCount);
	savefile->ReadFloat(m_Radius);
	savefile->ReadBounds(m_Bounds);
	savefile->ReadVec3(m_Velocity);
	savefile->ReadFloat(m_Magnitude);
	savefile->ReadInt(m_FallOffExponent);
	savefile->ReadInt(m_MaxResponses);
	savefile->ReadInt(m_CurResponses);
	savefile->ReadInt(m_ApplyTimer);
	savefile->ReadInt(m_ApplyTimerVal);
}

void CStim::AddResponseIgnore(idEntity *e)
{
	if(CheckResponseIgnore(e) != true)
	{
		idEntityPtr<idEntity> entPtr;
		entPtr = e;
		m_ResponseIgnore.Append(entPtr);
	}
}

void CStim::RemoveResponseIgnore(idEntity *e)
{
	for (int i = 0; i < m_ResponseIgnore.Num(); i++)
	{
		if (m_ResponseIgnore[i].GetEntity() == e)
		{
			m_ResponseIgnore.RemoveIndex(i);
			break;
		}
	}
}

bool CStim::CheckResponseIgnore(idEntity *e)
{
	bool rc = false;
	int i, n;

	n = m_ResponseIgnore.Num();
	for(i = 0; i < n; i++)
	{
		if(m_ResponseIgnore[i].GetEntity() == e)
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
	m_Timer.SetTicks(sys->ClockTicksPerSecond()/1000);

	return(&m_Timer);
}

void CStim::RemoveTimerFromGame(void)
{
	gameLocal.m_StimTimer.Remove(this);
}

void CStim::PostFired (int numResponses)
{
}
