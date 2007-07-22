/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 M�r 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef SR_RESPONSE__H
#define SR_RESPONSE__H

#include "StimResponse.h"

#include "ResponseEffect.h"
class CStim;

class CResponse : public CStimResponse {
	
	friend class CStimResponseCollection;

public:
	virtual void Save(idSaveGame *savefile) const;
	virtual void Restore(idRestoreGame *savefile);

	/**
	* This method is called when the response should
	* make its script callback. It is virtual
	* so that the container can reach overriden
	* versions from a CStimResponse base pointer.
	*
	* @sourceEntity: This is the entity carrying the stim
	* @stim: This is the stim to retrieve stim properties like magnitude, etc.
	*		 This is an optional argument, pass NULL to fire responses without
	*		 a "real" stim (e.g. frobbing)
	*/
	virtual void TriggerResponse(idEntity *sourceEntity, CStim* stim = NULL);

	/**
	 * Set the response script action.
	 */
	void SetResponseAction(idStr const &ActionScriptName);

	/**
	* Adds a response effect and returns the pointer to the new Effect object.
	*
	* @effectEntityDef: The entity definition where the target script is stored.
	*					The effect entity "effect_script" is treated specially.
	*
	* @effectPostfix:	The string that gets passed to the effect script (e.g. "1_2")
	*
	* @args:	The entity's spawnargs needed to query the script argument for the
	*			aforementioned special case of "effect_script".
	*/
	CResponseEffect* addResponseEffect(const idStr& effectEntityDef, 
									   const idStr& effectPostfix,
									   const idDict *args);

protected:
	CResponse(idEntity *Owner, int Type);
	virtual ~CResponse(void);

protected:
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
	* If non-zero, this specifies the number of effects
	* that get fired on response. If this is set to 2 and 
	* 5 response effects are available, exactly 2 random
	* effects are fired. If only one effect is available,
	* this effect would get fired twice.
	*/
	int					m_NumRandomEffects;

	/**
	* The list of ResponseEffects
	*/
	idList<CResponseEffect*> m_ResponseEffects;
};

#endif /* SR_RESPONSE__H */
