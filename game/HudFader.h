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
#pragma once


class HudFader
{
public:

    struct FadeParams
    {
        int  delay_ms{ 0 };
        int  duration_ms{ 0 };
    };

	HudFader() = default;

    HudFader(const FadeParams& fadeIn, const FadeParams& fadeOut);

	void Reset();

    void UpdateParams(const FadeParams& fadeIn, const FadeParams& fadeOut);

    void Show(bool autoToggleBack = false);

    void ShowInstantly(bool autoToggleBack = false);

    void Hide(bool autoToggleBack = false);

    void HideInstantly(bool autoToggleBack = false);

public: // Getters

	bool ShouldBeShown() const;

	bool WillAutoToggleBack() const;

	bool ShouldBeShownIndefinitely() const;

	bool ShouldBeHiddenIndefinitely() const;

    float GetAlpha();

private:

	enum class EState : bool { Shown = true, Hidden = false };

	void InitFade(EState desiredState, bool autoToggleBack, bool instantStateChange);

	template <bool fadingIn>
    float ExecuteFade(const FadeParams& params);

private:

	FadeParams m_fadeIn;  
	FadeParams m_fadeOut;

	
	EState m_currentTargetState{ EState::Hidden };
	EState m_restingState{ EState::Hidden };

    bool m_fadeFinished{ true };

    float m_currentAlpha{ 0.0f };
    float m_lastStateChangeAlpha{ 0.0f };
    int m_lastStateChangeTime{ 0 };
	int m_lastUpdatedTime{ 0 };
};

