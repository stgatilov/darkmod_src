#include "precompiled.h"
#include "HudFader.h"



HudFader::HudFader(const FadeParams& fadeIn, const FadeParams& fadeOut)
    : m_fadeIn(fadeIn)
	, m_fadeOut(fadeOut)
{
}


void HudFader::Reset()
{
	m_autoToggleBack       = false;
	m_shouldBeShown        = false;
	m_fadeFinished         = false;
	m_currentAlpha         = 0.0f;
	m_lastStateChangeAlpha = 0.0f;
	m_lastStateChangeTime  = 0;
	m_lastUpdatedTime      = 0;
}


void HudFader::UpdateParams(const FadeParams& fadeIn, const FadeParams& fadeOut)
{
	m_fadeIn  = fadeIn;
	m_fadeOut = fadeOut;
}


void HudFader::Show(bool autoToggleBack)
{
	InitFade(autoToggleBack, true, false);
}


void HudFader::ShowInstantly(bool autoToggleBack)
{
	InitFade(autoToggleBack, true, true);
}


void HudFader::Hide(bool autoToggleBack)
{
	InitFade(autoToggleBack, false, false);
}


void HudFader::HideInstantly(bool autoToggleBack)
{
	InitFade(autoToggleBack, false, true);
}


bool HudFader::ShouldBeShown() const
{
	return m_shouldBeShown;
}


bool HudFader::WillAutoToggleBack() const
{
	return m_autoToggleBack;
}


bool HudFader::ShouldBeShownIndefinitely() const
{
	return m_shouldBeShown && !m_autoToggleBack;
}


bool HudFader::ShouldBeHiddenIndefinitely() const
{
	return !m_shouldBeShown && !m_autoToggleBack;
}


float HudFader::GetAlpha()
{
    if (m_fadeFinished || m_lastUpdatedTime == gameLocal.time)
        return m_currentAlpha;

    if (m_shouldBeShown)
        return ExecuteFade<true>(m_fadeIn);
    else
        return ExecuteFade<false>(m_fadeOut);
}


void HudFader::InitFade(bool autoToggleBack, bool fadingIn, bool instantStateChange)
{
	// Indefinite state (e.g. LO->HI) has priority over momentary state (e.g. LO->HI->LO)
	// > Allows us to Show/Hide without having to worry some other momentary state change might overwrite it
	if (!m_autoToggleBack)
	{		
		if (fadingIn == m_shouldBeShown)
			return;
	}
	else if (!autoToggleBack && m_autoToggleBack)
	{
		// Let this call override the state
	}
	else if (fadingIn == m_shouldBeShown && !autoToggleBack && !instantStateChange)
	{
		// We are already doing this fade
		return;
	}

	const float targetAlpha = fadingIn ? 1.0f : 0.0f;
	m_shouldBeShown         = fadingIn;
	m_autoToggleBack        = autoToggleBack;
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
        if (m_autoToggleBack)
        {
			m_autoToggleBack = false;
			if (m_shouldBeShown)
				Hide();
			else
				Show();
        }
        else
        {
            m_fadeFinished = true;
        }
        return targetAlpha;
    }
}



