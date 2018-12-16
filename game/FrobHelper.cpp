#include "precompiled.h"
#include "FrobHelper.h"
#include "Game_local.h"
#include "FrobLock.h"
#include "FrobLockHandle.h"
#include "FrobDoorHandle.h"
#include <limits>

#pragma hdrstop

extern idGameLocal			gameLocal;

CFrobHelper::CFrobHelper()
: m_bShouldBeDisplayed(false)
, m_bActive(false)
, m_fCurrentAlpha(0.0f)
, m_fLastStateChangeAlpha(0.0f)
, m_iLastStateChangeTime(0)
, m_bReachedTargetAlpha(true)
, m_iFadeDelay(-1)
, m_iFadeInDuration(-1)
, m_iFadeOutDuration(-1)
, m_fMaxAlpha(-1.0)
{
}

CFrobHelper::~CFrobHelper()
{
}


void CFrobHelper::HideInstantly()
{
	if (!IsActive())
		return;

	if (m_fCurrentAlpha > 0.0f)
	{
		m_fCurrentAlpha = 0.0f;
		m_fLastStateChangeAlpha = 0.0f;
		m_bShouldBeDisplayed = false;
		m_bReachedTargetAlpha = true;
	}	
}


void CFrobHelper::Hide()
{
	if (!IsActive())
		return;

	if (m_bShouldBeDisplayed)
	{
		m_bShouldBeDisplayed	= false;
		m_iLastStateChangeTime	= gameLocal.time;
		m_fLastStateChangeAlpha = m_fCurrentAlpha;
		m_bReachedTargetAlpha	= false;
	}
}


void CFrobHelper::Show()
{
	if (!IsActive())
		return;

	if (!m_bShouldBeDisplayed)
	{
		m_bShouldBeDisplayed	= true;
		m_iLastStateChangeTime	= gameLocal.time;
		m_fLastStateChangeAlpha = m_fCurrentAlpha;
		m_bReachedTargetAlpha	= false;
	}
}


const bool CFrobHelper::IsEntityIgnored(idEntity* pEntity)
{
	if (!pEntity)
		return true;

	if (cv_frobhelper_ignore_size.GetFloat() < std::numeric_limits<float>::epsilon())
		// Ignore size is disabled
		return false;

	bool bIsTeamMember = pEntity->GetTeamMaster() != NULL;
	if (bIsTeamMember)
	{
		// Check all members of the team. Start with master
		for (idEntity* pEntityIt = pEntity->GetTeamMaster(); pEntityIt != NULL; pEntityIt = pEntityIt->GetNextTeamEntity())
		{
			if (IsEntityTooBig(pEntityIt))
				return true;
		}
		return false;
	}

	// Entity is not a team member. Just check its size
	return IsEntityTooBig(pEntity);
}


const float CFrobHelper::GetAlpha()
{
	if (!IsActive())
		return 0.0f;

	const int&		iFadeDelay			= GetFadeDelay();
	const int&		iFadeInDuration		= GetFadeInDuration();
	const int&		iFadeOutDuration	= GetFadeOutDuration();
	const float&	fMaxAlpha			= GetMaxAlpha();

	if (m_bReachedTargetAlpha)
		// Early return for higher performance
		return m_fCurrentAlpha;

	const int iTime = gameLocal.time;

	// Calculate current FrobHelper alpha based on delay and fade-in / fade-out
	if (m_bShouldBeDisplayed) 
	{		
		// Skip the fade delay if a fade was already active
		const bool bPreviousFadeoutNotCompleted = m_fLastStateChangeAlpha > 0.0f;
		const int iFadeStart = m_iLastStateChangeTime 			
			+ (bPreviousFadeoutNotCompleted ? 0 : iFadeDelay);
		if (iTime < iFadeStart)
			return 0.0f;

		// Calculate fade end time
		// > If there was a previous unfinished fade, reduce the fade end time to the needed value
		const float fFadeDurationFactor =
			fabs(fMaxAlpha - m_fLastStateChangeAlpha) / fMaxAlpha;
		const int iFadeEnd = iFadeStart 
			+ static_cast<int>(fFadeDurationFactor * static_cast<float>(iFadeInDuration));

		if (iTime < iFadeEnd)
		{
			const float fFadeTimeTotal = static_cast<float>(iFadeEnd - iFadeStart);
			m_fCurrentAlpha = m_fLastStateChangeAlpha + abs(fMaxAlpha - m_fLastStateChangeAlpha)
				* static_cast<float>(iTime - iFadeStart)
				/ fFadeTimeTotal;
		}
		else if (!m_bReachedTargetAlpha)
		{
			m_fCurrentAlpha = fMaxAlpha;
			m_bReachedTargetAlpha = true;
		}
	} else // Should not be displayed
	{	
		// Calculate fade ent time
		// > If there was a previous unfinished fade, reduce the fade end time to the needed value
		const float fFadeDurationFactor =
			m_fLastStateChangeAlpha / fMaxAlpha;
		const int iFadeEnd = m_iLastStateChangeTime 
			+ static_cast<int>(fFadeDurationFactor*static_cast<float>(iFadeOutDuration));

		// Calculate FrobHelper alpha based on fade out time
		if (iTime < iFadeEnd)
		{
			m_fCurrentAlpha = m_fLastStateChangeAlpha *
				(1.0f - static_cast<float>(iTime - m_iLastStateChangeTime)
					    / static_cast<float>(iFadeOutDuration) );
		}
		else if (!m_bReachedTargetAlpha)
		{
			m_fCurrentAlpha = 0.0;
			m_bReachedTargetAlpha = true;
		}
	}

	return m_fCurrentAlpha;
}
