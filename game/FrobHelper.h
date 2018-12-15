#pragma once

typedef struct trace_s trace_t;

class CFrobHelper
{
public:
	CFrobHelper();
	~CFrobHelper();

	/** @brief	  	Hide the frob helper, skipping the fade-out
	  * @author		STiFU */
	void HideInstantly();
	
	/** @brief	  	Hide the FrobHelper with fade-out
	  * @author		STiFU */
	void Hide();
	
	/** @brief	  	Show the FrobHelper with fade-in and delay
	  * @author		STiFU */
	void Show();

	// TODO: change method to just do the increased distance check
	/** @brief	  	Check whether the Frob Helper should be displayed or not
	  * @param 		eyePos	The position of the eye of the player
	  * @param		frobTraceEnd	The end position of the frob trace
	  * @author		STiFU */
	//void UpdateState(const idVec3& eyePos, const idVec3& frobTraceEnd);

	/** @brief	  	Retrieve the current visibility of the frob helper
	  * @return   	The visibility of the frob helper in the range [0,1]
	  * @author		STiFU */
	const float GetAlpha();

	/** @brief	  	Check whether the FrobHelper is supposed to used
	  * @remark		If the state changes, it will also reset all internal members
	  * @return   	True, if it is active
	  * @author		STiFU */
	inline const bool IsActive()
	{
		// TODO: poll cvar
		static const bool bActive = true;
		if (bActive != m_bActive)
		{
			m_bActive = bActive;
			Reset();
		}
		return m_bActive;
	}

private:
	// TODO: add separate fade out delay

	inline const float GetFadeDelay()
	{
		// TODO: poll cvar
		static const int iDelay = 500;
		if (m_iFadeDelay != iDelay)
		{
			m_iFadeDelay = iDelay;
			Reset();
		}
		return m_iFadeDelay;
	}

	inline const int GetFadeDuration()
	{
		// TODO: poll cvar
		static const int iFadeInOutDuration = 1000;
		if (m_iFadeInOutDuration != iFadeInOutDuration)
		{
			m_iFadeInOutDuration = iFadeInOutDuration;
			Reset();
		}
		return m_iFadeInOutDuration;
	}

	inline const float GetMaxAlpha()
	{
		// TODO: poll cvar
		static const float fMaxAlpha = 1.0f;
		if (fMaxAlpha != m_fMaxAlpha)
		{
			m_fMaxAlpha = fMaxAlpha;
			Reset();
		}
		return m_fMaxAlpha;
	}

	inline void Reset()
	{
		m_bShouldBeDisplayed = false;
		m_bReachedTargetAlpha = false;
		m_fCurrentAlpha = 0.0f;
		m_fLastStateChangeAlpha = 0.0f;
		m_iLastStateChangeTime = 0;
	}


private:
	// Configuration
	bool	m_bActive;
	int		m_iFadeDelay;
	int		m_iFadeInOutDuration;
	float	m_fMaxAlpha;

	// State members
	bool	m_bShouldBeDisplayed;
	bool	m_bReachedTargetAlpha;
	float	m_fCurrentAlpha;
	float	m_fLastStateChangeAlpha;
	int		m_iLastStateChangeTime;
};