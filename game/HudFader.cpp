#include "precompiled.h"
#include "HudFader.h"



HudFader::HudFader(const FadeParams& fadeIn, const FadeParams& fadeOut)
    : m_fadeIn(fadeIn)
	, m_fadeOut(fadeOut)
{
}


void HudFader::UpdateParams(const FadeParams& fadeIn, const FadeParams& fadeOut)
{
	m_fadeIn  = fadeIn;
	m_fadeOut = fadeOut;
}


void HudFader::Show(bool autoToggleBack)
{
	if (IsIgnored(autoToggleBack, true, false))
		return;
    
    m_shouldBeShown = true;
    m_lastStateChangeTime = gameLocal.time;
    m_lastStateChangeAlpha = m_currentAlpha;
    m_fadeFinished = false;
    m_autoToggleBack = autoToggleBack;
}


void HudFader::ShowInstantly(bool autoToggleBack)
{
	if (IsIgnored(autoToggleBack, true, true))
		return;
    
    m_currentAlpha = 1.0f;
    m_lastStateChangeAlpha = 1.0f;
    m_shouldBeShown = true;
    m_fadeFinished = true;
    m_autoToggleBack = autoToggleBack;
}


void HudFader::Hide(bool autoToggleBack)
{
	if (IsIgnored(autoToggleBack, false, false))
		return;

    m_shouldBeShown = false;
    m_lastStateChangeTime = gameLocal.time;
    m_lastStateChangeAlpha = m_currentAlpha;
    m_fadeFinished = false;
    m_autoToggleBack = autoToggleBack;
}


void HudFader::HideInstantly(bool autoToggleBack)
{
	if (IsIgnored(autoToggleBack, false, true))
		return;
    
    m_currentAlpha = 0.0f;
    m_lastStateChangeAlpha = 0.0f;
    m_shouldBeShown = false;
    m_fadeFinished = true;
    m_autoToggleBack = autoToggleBack;
}


bool HudFader::ShouldBeShown() const
{
	return m_shouldBeShown;
}


bool HudFader::WillAutoToggleBack() const
{
	return m_autoToggleBack;
}


bool HudFader::ShouldBeShownStatic() const
{
	return m_shouldBeShown && !m_autoToggleBack;
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

bool HudFader::IsIgnored(bool autoToggleBack, bool fadingIn, bool instantStateChange) const
{
	if (autoToggleBack && !m_autoToggleBack)
	{
		// Indefinite state has priority over finite state changes
		// Meaning: A Show() without autoToggleBack has to be manually resolved by a Hide(). Until then all Show() with autoToggleBack will be ignored.
		// > Allows us to set a static state without having to worry some other call might overwrite it
		return fadingIn == m_shouldBeShown;
	}

	const bool nothingToDo = fadingIn == m_shouldBeShown && !autoToggleBack && !instantStateChange;
	return nothingToDo;
}


