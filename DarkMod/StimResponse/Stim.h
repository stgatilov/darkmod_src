/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef SR_STIM__H
#define SR_STIM__H

#include "StimResponse.h"
#include "StimResponseTimer.h"

/**
 * CStim is a base class for the stims. The constructor and destructors
 * are declared protected so that only the collection can actually create
 * destroy them.
 */
class CStim : public CStimResponse {
friend CStimResponseCollection;

protected:
	CStim(idEntity *, int Type);
	virtual ~CStim(void);

public:
	/**
	 * Add a responseentity to the ignore list. If the response is already
	 * in the list, it is not entered again.
	 */
	void AddResponseIgnore(idEntity *);
	void RemoveResponseIgnore(idEntity *);
	bool CheckResponseIgnore(idEntity *);

	CStimResponseTimer *CreateTimer(void);
	void RemoveTimer(void);
	CStimResponseTimer *GetTimer(void) { return &m_Timer; };

protected:
	/**
	 * Timer for the stimulus. If no timer is set, then it is assumed
	 * that this stimulus is always working whenever it is applied.
	 */
	CStimResponseTimer	m_Timer;

public:
	/**
	 * This is the list of all responses that should be ignored for this stim.
	 * This is required for stims, which are having a livespan during which they
	 * can fire. Each response would fire on each frame as long as the stim is
	 * enabled. This is not really usefull in most cases, so we can add a response,
	 * which already has fired, to the ignorelist. Until the response is removed
	 * it will no longer fire.
	 */
	idList<idEntity *>		m_ResponseIgnore;

	/**
	* If set to true, the stim uses the entity's bounds in the stim intersection test.
	* This makes it possible to get accurate stims with non-cube objects.
	* Note that radius further expands on these bounds.
	**/
	bool					m_bUseEntBounds;

	/**
	* Milliseconds between interleaving for use with frame-based timer check (not StimTimer)
	**/
	int						m_TimeInterleave;

	/**
	* Timestamp used with time interleaving code.
	**/
	int						m_TimeInterleaveStamp;

	/**
	 * Radius defines the radius the action can reach out
	 */
	float				m_Radius;

	/**
	* Magnitude of stim, per stim firing.  This can be damage if the stim does damage,
	*	or healing if it's a healing stim, or more abstract things like amount of water,
	*	amount of energy transferred by heat, etc.
	**/
	float				m_Magnitude;

	/**
	* The Falloff shape of the magnitude in dependence of the distance.
	* 0 = constant (homogeneous) - magnitude is the same for all
	* 1 = linear
	* 2 = quadratic
	* 3 = etc..
	*/
	int					m_FallOffExponent;

	/**
	 * Defines the chance that this stimulus works. Whenever the stim is activated
	 * the chance determines wether it will ignore the activation and stay enabled.
	 */
	float				m_Chance;

	/**
	 * Whenever the chance test failed, and the stim has a timer before it can be 
	 * reused again, the ChanceTimer determines wether the timer should be used (true)
	 * or not (false = default).
	 * This can be used to create a stim that has a chance of failure but needs time 
	 * to recharge, bevore it can be used again, but the reuse timer may not always 
	 * be desirable to be applied.
	 */
	bool				m_ChanceTimer;

	/**
	 * Defines the maximum number responses that can be applied to this particular
	 * stimulus at any given time. 0 means unlimited and is the default.
	 */
	int					m_MaxResponses;
	int					m_CurResponses;		// Already active responses

	/**
	 * Applytimer defines the number of times the stim can be used before the stim starts
	 * it's timer. For example a machinegun can be used for forty rounds before
	 * it has to cool down a certain time. 0 is unlimited.
	 */
	int					m_ApplyTimer;
	int					m_ApplyTimerVal;

	/**
	* This virtual member is called after the stimulus has been fired to allow the stim
	* to adjust itself according to any stim class specific logic.
	*
	* @param numResponses The number of responses triggered by the stim.  It may be 0 to
	* indicate there were no active responders present.
	*/
	virtual void PostFired (int numResponses);

};

#endif /* SR_STIM__H */
