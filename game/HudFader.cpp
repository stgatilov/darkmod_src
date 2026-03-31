/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#include "HudFader.h"



HudFader::HudFader(const FadeParams& fadeIn, const FadeParams& fadeOut)
    : m_fadeIn(fadeIn)
	, m_fadeOut(fadeOut)
{
}


void HudFader::Reset()
{
	m_currentTargetState      = EState::Hidden;
	m_restingState            = EState::Hidden;
	m_fadeFinished            = true;
	m_currentAlpha            = 0.0f;
	m_lastStateChangeAlpha    = 0.0f;
	m_lastStateChangeTime     = 0;
	m_lastUpdatedTime         = 0;
}


void HudFader::UpdateParams(const FadeParams& fadeIn, const FadeParams& fadeOut)
{
	m_fadeIn  = fadeIn;
	m_fadeOut = fadeOut;
}


void HudFader::Show(bool autoToggleBack)
{
	InitFade(EState::Shown, autoToggleBack, false);
}


void HudFader::ShowInstantly(bool autoToggleBack)
{
	InitFade(EState::Shown, autoToggleBack, true);
}


void HudFader::Hide(bool autoToggleBack)
{
	InitFade(EState::Hidden, autoToggleBack, false);
}


void HudFader::HideInstantly(bool autoToggleBack)
{
	InitFade(EState::Hidden, autoToggleBack, true);
}


bool HudFader::ShouldBeShown() const
{
	return m_currentTargetState == EState::Shown;
}


bool HudFader::WillAutoToggleBack() const
{
	return m_currentTargetState != m_restingState;
}


bool HudFader::ShouldBeShownIndefinitely() const
{
	return m_restingState == EState::Shown;
}


bool HudFader::ShouldBeHiddenIndefinitely() const
{
	return m_restingState == EState::Hidden;
}


float HudFader::GetAlpha()
{
    if (m_fadeFinished || m_lastUpdatedTime == gameLocal.time)
        return m_currentAlpha;

    if (m_currentTargetState == EState::Shown)
        return ExecuteFade<true>(m_fadeIn);
    else
        return ExecuteFade<false>(m_fadeOut);
}


void HudFader::InitFade(EState desiredState, bool autoToggleBack, bool instantStateChange)
{
	const float targetAlpha = desiredState == EState::Shown ? 1.0f : 0.0f;
	m_currentTargetState    = desiredState;
	if (!autoToggleBack)
	{
		m_restingState = desiredState;
	}
	if (instantStateChange)
	{
		m_lastStateChangeAlpha = targetAlpha;
		m_currentAlpha         = targetAlpha;
		m_fadeFinished         = true;
	}
	else
	{
		m_lastStateChangeAlpha = m_currentAlpha;
		m_lastStateChangeTime  = gameLocal.time;
		m_fadeFinished         = false;
	}
}


template <bool fadingIn>
float HudFader::ExecuteFade(const FadeParams& params)
{
    const float targetAlpha = fadingIn ? 1.0f : 0.0f;
	m_lastUpdatedTime = gameLocal.time;

    // Fade delay: We skip delay if a fade was already running before
    const bool skipDelay = params.delay_ms <= 0 
        || fadingIn && m_lastStateChangeAlpha > 0.0f
        || !fadingIn && m_lastStateChangeAlpha < 1.0f;
    const int fadeStart = m_lastStateChangeTime + (skipDelay ? 0 : params.delay_ms);
	if (gameLocal.time < fadeStart)
		return fadingIn ? 0.0f : 1.0f;

    // Fade 
    const float remainingFade = targetAlpha - m_lastStateChangeAlpha;
    const int   fadeEnd = params.duration_ms <= 0 ? 0 : fadeStart + (targetAlpha ? remainingFade : -remainingFade) * params.duration_ms;
    if (gameLocal.time < fadeEnd)
    {
        const int actualFadeDuration = fadeEnd - fadeStart;
        const int elapseSinceStart = gameLocal.time - fadeStart;
        m_currentAlpha = m_lastStateChangeAlpha + remainingFade * elapseSinceStart / actualFadeDuration;
        return m_currentAlpha;
    }
    else
    {
        m_currentAlpha = targetAlpha;
        if (m_restingState != m_currentTargetState)
        {
			m_currentTargetState   = m_restingState;
			m_lastStateChangeAlpha = targetAlpha;
			m_lastStateChangeTime  = gameLocal.time;
        }
        else
        {
            m_fadeFinished = true;
        }
        return targetAlpha;
    }
}



