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

    void UpdateParams(const FadeParams& fadeIn, const FadeParams& fadeOut);

    void Show(bool autoToggleBack = false);

    void ShowInstantly(bool autoToggleBack = false);

    void Hide(bool autoToggleBack = false);

    void HideInstantly(bool autoToggleBack = false);

	bool ShouldBeShown() const;

	bool WillAutoToggleBack() const;

	bool ShouldBeShownStatic() const;

    float GetAlpha();

private:

	template <bool fadingIn>
    float ExecuteFade(const FadeParams& params);

	bool IsIgnored(bool autoToggleBack, bool fadingIn, bool instantStateChange) const;

private:

	FadeParams m_fadeIn;  
	FadeParams m_fadeOut;

    bool m_autoToggleBack{ false };

    bool m_shouldBeShown{ false };
    bool m_fadeFinished{ false };

    float m_currentAlpha{ 0.0f };
    float m_lastStateChangeAlpha{ 0.0f };
    int m_lastStateChangeTime{ 0 };
	int m_lastUpdatedTime{ 0 };
};

