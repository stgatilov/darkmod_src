#include "precompiled.h"
#include "FrobHelper.h"
#include "Game_local.h"

// TODO: is this needed?
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
, m_iFadeInOutDuration(-1)
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


// void CFrobHelper::UpdateState(const idVec3& eyePos, const idVec3& frobTraceEnd)
// {
// 	if (!IsActive())
// 		return;
// 
// 	// Check whether multiple entities are in close proximity to the frob end point
// 	// If so, the frob helper should be displayed
// 	idBounds frobBounds(frobTraceEnd);
// 	static const float fFrobHelperDisplayRange = 10; // TODO: make cvar
// 	frobBounds.ExpandSelf(fFrobHelperDisplayRange);
// 
// 	// TODO optimize: share with player.cpp to save some memory
// 	static idEntity* frobRangeEnts[MAX_GENTITIES];
// 	const int numFrobEnt = gameLocal.clip.EntitiesTouchingBounds(
// 		frobBounds, -1, frobRangeEnts, MAX_GENTITIES);
// 
// 	// TODO: test for direct hit needed
// 
// 	// Count frobable entites in range
// 	int iFrobableEntities = 0;
// 	for (int i = 0; i < numFrobEnt; i++)
// 	{
// 		// Skip irrelevant entites
// 		idEntity *ent = frobRangeEnts[i];
// 		if (!ent || !ent->m_bFrobable || ent->IsHidden() || !ent->m_FrobDistance)
// 			continue;
// 		
// 		// Check if entity is within frob distance
// 		static const float fExtraSpace = 8;
// 		const float frobDist = ent->m_FrobDistance + fExtraSpace;
// 		const idVec3 delta = ent->GetPhysics()->GetOrigin() - eyePos;
// 		const float entDistance = delta.LengthFast();
// 		if (entDistance > frobDist)
// 			continue;
// 		
// 		iFrobableEntities++;
// 	}
// 
// 	// Change state
// 	const bool bShouldBeDisplayed = iFrobableEntities > 0;
// 	if (bShouldBeDisplayed != m_bShouldBeDisplayed)
// 	{
// 		// TODO: Add mutex
// 		m_bShouldBeDisplayed = bShouldBeDisplayed;
// 		m_iLastStateChangeTime = gameLocal.time;
// 		m_fLastStateChangeAlpha = m_fCurrentAlpha;
// 		m_bReachedTargetAlpha = false;		
// 	}
// }

const float CFrobHelper::GetAlpha()
{
	if (!IsActive())
		return 0.0f;

	// TODO: activate mutex here

	const int	iFadeDelay		= GetFadeDelay();
	const int	iFadeDuration	= GetFadeDuration();
	const float	fMaxAlpha		= GetMaxAlpha();

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
			+ static_cast<int>(fFadeDurationFactor * static_cast<float>(iFadeDuration));

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
			+ static_cast<int>(fFadeDurationFactor*static_cast<float>(iFadeDuration));

		// Calculate FrobHelper alpha based on fade out time
		if (iTime < iFadeEnd)
		{
			m_fCurrentAlpha = m_fLastStateChangeAlpha *
				(1.0f - static_cast<float>(iTime - m_iLastStateChangeTime)
					    / static_cast<float>(iFadeDuration) );
		}
		else if (!m_bReachedTargetAlpha)
		{
			m_fCurrentAlpha = 0.0;
			m_bReachedTargetAlpha = true;
		}
	}

	return m_fCurrentAlpha;
}
