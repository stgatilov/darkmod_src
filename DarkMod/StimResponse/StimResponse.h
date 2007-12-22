/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 M�r 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#ifndef SR_STIMRESPONSE__H
#define SR_STIMRESPONSE__H

extern char *cStimType[];

// If default stims are to be added here, the static array in the CPP file
// also must be updated. USER and UNDEFINED are not to be added though, as
// they have special meanings.
typedef enum {
	ST_FROB,			// Frobbed
	ST_FIRE,			// Fire
	ST_WATER,			// Water
	ST_DAMAGE,			// damages target
	ST_SHIELD,			// protects against arrows or physical blows
	ST_HEALING,			// heals target
	ST_HOLY,			// holy is applied
	ST_MAGIC,			// Magic is being used
	ST_TOUCH,			// triggered if touched
	ST_KNOCKOUT,		// target is knocked out
	ST_KILL,			// target is killed
	ST_RESTORE,			// target is restored
	ST_LIGHT,			// triggered by light
	ST_SOUND,			// triggered by sound
	ST_VISUAL,			// visual contact
	ST_INVITE,			// can be used to trigger special behaviour (like a stool can invite an AI to sit down)
	ST_READ,			// Can be read
	ST_RANDOM,			// Random response is selected
	ST_TIMER,			// Timer trigger
	ST_COMMUNICATION,	// A communication stimulus (see CommunicationStim.h)
	ST_GAS,				// triggered by gas arrows
	ST_TRIGGER,			// Triggered by triggering :)
	ST_TARGET_REACHED,	// Emitted, if the AI has reached its target (induced by effect_moveToPosition)
	ST_PLAYER,			// The Stim emitted by the player
	ST_FLASH,			// Emitted by flashbombs
	ST_BLIND,			// A stim that immediately blinds the AI (no visibility is needed) - for use in flashmines
	ST_USER				= 1000,	// User defined types should use this as it's base
	ST_DEFAULT			= -1
} StimType;

typedef enum {
	SS_DISABLED,		// Stim is disabled and can not be triggered
	SS_ENABLED,			// Stim is enabled and waits for activation
	SS_DEFAULT
} StimState;

/**
 * CStimResponse is the baseclass for stims and responses
 */
class CStimResponse {

	friend class CStimResponseCollection;

protected:
	CStimResponse(idEntity *Owner, int Type, int uniqueId);
	virtual ~CStimResponse(void);

public:
	virtual void Save(idSaveGame *savefile) const;
	virtual void Restore(idRestoreGame *savefile);

	void EnableSR(bool Enable = true);

	/**
	 * greebo: Returns the unique ID used to identify this S/R after map load.
	 */
	int	getUniqueId() const;

	/** 
	* greebo: This evaluates the m_Chance member variable against a random float.
	*
	* @returns: TRUE, if the S/R passed the check and can be fired
	*/
	bool checkChance();

	/**
	* greebo: This retrieves the stim type id for the given stimName.
	*
	* @stimName: The name of the stim (e.g. "STIM_THIEF")
	* @returns: the according StimType (if the name is known), or ST_DEFAULT (== -1) if unknown
	*/
	static StimType getStimType(const idStr& stimName);

public:
	// A unique ID as assigned by the StimResponseCollection. Used to identify 
	// this stim after map load.
	int					m_UniqueId;

	/**
	 * Id for the stimulus that uniquely identifies a stim, so they can
	 * be associated to each other.
	 */
	int					m_StimTypeId;

	// This is only populated with the Id as used in the entity definition. We
	// store the name here to reference the script action key.
	idStr				m_StimTypeName;

	/**
	 * If set to true, then the stim can be removed from an entity. This is mistly needed
	 * for an external app lication later on, so that the defauls can not be accidently
	 * removed.
	 */
	bool				m_Removable;

	/**
	 * State for the stim/response.
	 */
	StimState			m_State;

	/**
	 * Defines the chance that this stim/response works. Whenever the S/R is activated
	 * the chance determines wether it will do its job.
	 */
	float				m_Chance;

	/**
	 * Whenever the chance test failed, and the stim has a timer before it can be 
	 * reused again, the ChanceTimer determines wether the timer should be used (true)
	 * or not (default = -1).
	 * This can be used to create a stim that has a chance of failure but needs time 
	 * to recharge, bevore it can be used again, but the reuse timer may not always 
	 * be desirable to be applied.
	 */
	int					m_ChanceTimer;

	/**
	* greebo: This is the earliest time a next chance can be evaluated.
	*		  If a previous chance test failed, the next chance time is calculated.
	*		  If a previous chance test was passed, the time is set to -1;
	*/
	int					m_NextChanceTime;

	/**
	 * Default means that this is a stim which has been added as default to this entity.
	 * Thiw would also mainly be used for an editor.
	 */
	bool				m_Default;

	/**
	* Timestamp for stims/responses with finite duration after they're enabled (milliseconds)
	**/
	int						m_EnabledTimeStamp;

	/**
	* Stim or response duration after being enabled (in milliseconds).  
	* SR will automatically disable itself after this time.
	**/
	int						m_Duration;

	idEntityPtr<idEntity>	m_Owner;
};

#endif /* SR_STIMRESPONSE__H */
