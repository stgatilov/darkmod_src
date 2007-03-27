/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mär 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef SR_RESPONSE__H
#define SR_RESPONSE__H

#include "StimResponse.h"

#include "ResponseEffect.h"

class CResponse : public CStimResponse {
friend CStimResponseCollection;

public:
	/**
	* This method is called when the response should
	* make its script callback. It is virtual
	* so that the container can reach overriden
	* versions from a CStimResponse base pointer.
	*/
	virtual void TriggerResponse(idEntity *Stim);

	/**
	 * Set the response script action.
	 */
	void SetResponseAction(idStr const &ActionScriptName);

	/**
	* Adds a response effect and returns the pointer to the local list
	* and returns the pointer to the new Effect object, so that the
	* data can be pumped into it.
	*/
	CResponseEffect* addResponseEffect(const idStr& effectEntityDef, const idStr& effectPostfix);

protected:
	CResponse(idEntity *Owner, int Type);
	virtual ~CResponse(void);

protected:
	/**
	 * Once this stimuls is finished, another one can be fired. So you
	 * can chain multiple stimulis one after the other. If NULL this is
	 * a single event.
	 */
	CResponse			*m_FollowUp;

	/**
	 * Scriptfunction that is to be executed when this response 
	 * is triggered.
	 */
	idStr				m_ScriptFunction;

	/**
	 * How much damage must be applied for this response?
	 */
	float				m_MinDamage;

	/**
	 * No more than this.
	 */
	float				m_MaxDamage;

	/**
	 * Defines the chance that this response is applied to the stimulus. Default should 
	 * be 1. A value of 0 would mean that this is never applied, but then it doesn't make
	 * sense to use this in most cases. 
	 */

	float				m_Chance;

	/**
	* The list of ResponseEffects
	*/
	idList<CResponseEffect*> m_ResponseEffects;
};

#endif /* SR_RESPONSE__H */
