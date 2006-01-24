/******************************************************************************/
/*                                                                            */
/*         StimResponse (C) by Gerhard W. Gruber in Germany 2005              */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.2  2006/01/24 22:03:24  sparhawk
 * Stim/Response implementation preliminary
 *
 * Revision 1.1  2005/12/08 21:34:28  sparhawk
 * Intitial release
 *
 *
 * DESCRIPTION: Implementation for the Stim/Response system.
 *
 *****************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "StimResponse.h"

CStimResponseCollection::CStimResponseCollection(void)
{
	m_MaxRadius = 0.0f;
}

CStimResponseCollection::~CStimResponseCollection(void)
{
}

CStim *CStimResponseCollection::AddStim(idEntity *Owner, int Type, float fRadius, bool bRemovable, bool bDefault)
{
	CStim *pRet = NULL;
	int i, n;

	n = m_Stim.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Stim[i]->m_StimTypeId == Type)
		{
			pRet = m_Stim[i];
			break;
		}
	}

	if(pRet == NULL)
	{
		pRet = new CStim(Owner, Type);
		m_Stim.Append(pRet);
	}

	if(pRet != NULL)
	{
		pRet->m_Default = bDefault;
		pRet->m_Removable = bRemovable;
		pRet->m_Radius = fRadius;
		pRet->m_State = SS_DISABLED;

		AddEntityToList(gameLocal.m_StimEntity, Owner);
	}

	return pRet;
}

CResponse *CStimResponseCollection::AddResponse(idEntity *Owner, int Type, bool bRemovable, bool bDefault)
{
	CResponse *pRet = NULL;
	int i, n;

	n = m_Response.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Response[i]->m_StimTypeId == Type)
		{
			pRet = m_Response[i];
			break;
		}
	}

	if(pRet == NULL)
	{
		pRet = new CResponse(Owner, Type);
		m_Response.Append(pRet);
	}

	if(pRet != NULL)
	{
		pRet->m_Default = bDefault;
		pRet->m_Removable = bRemovable;

		AddEntityToList(gameLocal.m_RespEntity, Owner);
	}

	return pRet;
}

void CStimResponseCollection::RemoveStim(int Type)
{
	CStim *pRet = NULL;
	int i, n;

	n = m_Stim.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Stim[i]->m_StimTypeId == Type)
		{
			pRet = m_Stim[i];
			if(pRet->m_Removable == true)
			{
				m_Stim.RemoveIndex(i);
				delete pRet;
			}
			break;
		}
	}
}


void CStimResponseCollection::AddEntityToList(idList<idEntity *> &oList, idEntity *e)
{
	bool add = true;
	int i, n;

	n = oList.Num();
	for(i = 0; i < n; i++)
	{
		if(oList[i] == e)
		{
			add = false;
			break;
		}
	}

	if(add == true)
		oList.Append(e);
}



/********************************************************************/
/*                     CStim                                        */
/********************************************************************/
CStim::CStim(idEntity *e, int Type)
{
	m_Timer = NULL;
	m_StimTypeId = Type;
	m_State = SS_DISABLED;
	m_Radius = 0.0;
	m_TriggerDamage = 0.0;
	m_DurationDamage = 0.0;
	m_Chance = 1.0;
	m_ChanceTimer = false;
	m_MaxResponses = 0;
	m_CurResponses = 0;
	m_ApplyTimer = 0;
	m_ApplyTimerVal = 0;
	m_Removable = true;
	m_Default = false;
	m_Owner = e;
}

CStim::~CStim(void)
{
	if(m_Timer != NULL)
		delete m_Timer;
}

void CStim::EnableStim(bool bEnable)
{
	if(bEnable == true)
		m_State = SS_ENABLED;
	else
		m_State = SS_DISABLED;
}

void CStim::ActivateStim(void)
{
	m_State = SS_ACTIVE;
}


/********************************************************************/
/*                   CResponse                                      */
/********************************************************************/
CResponse::CResponse(idEntity *e, int Type)
{
	m_FollowUp = NULL;
	m_ScriptFunction = NULL;
	m_StimTypeId = Type;
	m_MinDamage = 0.0f;
	m_MaxDamage = 0;
	m_Chance = 1.0f;
	m_Removable = true;
	m_Default = false;
	m_Owner = e;
}

CResponse::~CResponse(void)
{
}

/********************************************************************/
/*                 CStimResponseTimer                               */
/********************************************************************/
CStimResponseTimer::CStimResponseTimer(void)
{
	m_Type = SRTT_RELOAD;
	m_State = SRTS_DISABLED;
	m_Reload = 0;
	m_ReloadVal = 0;
	m_ReloadTimer = 0;
	m_ReloadTimerVal = 0;
	m_Apply = 0;
	m_ApplyVal = 0;
	m_ApplyCounter = -1;
	m_ApplyCounterVal = -1;
	m_Duration = 0;
	m_DurationVal = 0;
}

CStimResponseTimer::~CStimResponseTimer(void)
{
}

