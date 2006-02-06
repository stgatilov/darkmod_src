/******************************************************************************/
/*                                                                            */
/*         StimResponse (C) by Gerhard W. Gruber in Germany 2005              */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.6  2006/02/06 22:13:51  sparhawk
 * Added ignore list for responses.
 *
 * Revision 1.5  2006/02/04 23:51:39  sparhawk
 * Finished the Stim/Response for radius types.
 *
 * Revision 1.4  2006/01/31 22:34:44  sparhawk
 * StimReponse first working version
 *
 * Revision 1.3  2006/01/25 22:05:51  sparhawk
 * Added additional entries to support stims on projectiles.
 *
 * Revision 1.2  2006/01/24 22:03:24  sparhawk
 * Stim/Response implementation preliminary
 *
 * Revision 1.1  2005/12/08 21:34:28  sparhawk
 * Intitial release
 *
 *
 *
 * DESCRIPTION: Headerfile for the Stim/Response system.
 *
 *****************************************************************************/

#ifndef STIMRESPONSE_H
#define STIMRESPONSE_H

/******************************************************************************
The Stim/Response system consists of a collection class, which handles the
actuall stimulations and responses. Each entity can have exactly one such 
collection, and all stim/responses are added to this collection. All 
primary interactions with stims and responses are done via this collection.
******************************************************************************/
class CStimResponseCollection;
class CStim;

extern char *cStimType[];


/**
 * CStimResponseTimer handles all timing aspects of stimuli.
 * Each of the values as a copy, which is used to store the actual value at runtime.
 * The original value is used for initialisation but is never changed during runtime
 * as only the Val counterparts will be used to track the current state.
 */
class CStimResponseTimer {
friend CStim;
friend CStimResponseCollection;

public:
	typedef enum {
		SRTT_SINGLESHOT,			// Stimuls can be used exactly once
		SRTT_RELOAD,				// Stimulus can be reused
		SRTT_DEFAULT
	} TimerType;

	typedef enum {
		SRTS_DISABLED,				// Timer is not active
		SRTS_TRIGGERED,				// Stimuls just became active
		SRTS_RUNNING,				// Stimulus is progressing (only relevant for durations)
		SRTS_EXPIRED,				// Stimulus has been expired (can be reloaded of reload is set)
		SRTS_DEFAULT
	} TimerState;

protected:
	CStimResponseTimer(void);
	virtual ~CStimResponseTimer(void);

protected:
	TimerType		m_Type;
	TimerState		m_State;

	/**
	 * How often can the stimulus be reused. 0 = unlimited
	 */
	int				m_Reload;
	int				m_ReloadVal;

	/**
	 * How long does it take until this stimulus can be used again.
	 * 0 = immediately
	 */
	int				m_ReloadTimer;
	int				m_ReloadTimerVal;

	/**
	 * How long does the stimulus need to be applied, before the action is
	 * triggered.
	 * i.E. When a candle is hold to a book, it will take some time until
	 * it catches fire.
	 * 0 = no limit
	 */
	int				m_Apply;
	int				m_ApplyVal;

	/**
	 * How often can the stimulus be applied before it is depleted.
	 * -1 = unlimited
	 */
	int				m_ApplyCounter;
	int				m_ApplyCounterVal;

	/**
	 * How long is the stim performing it's action.
	 * 0 = unlimited
	 */
	int				m_Duration;
	int				m_DurationVal;
};


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
	ST_SIT,				// can be used to sit down
	ST_READ,			// Can be read
	ST_RANDOM,			// Random response is selected
	ST_TIMER,			// Timer trigger
	ST_USER				= 1000,	// User defined types should use this as it's base
	ST_DEFAULT			= -1
} StimType;

typedef enum {
	SS_DISABLED,		// Stim is disabled and can not be triggered
	SS_ENABLED,			// Stim is enabled and waits for activation
	SS_ACTIVE,			// Stim has been activated
	SS_DEFAULT
} StimState;

/**
 * CStimResponse is the baseclass for stims and responses
 */
class CStimResponse {
friend CStimResponseCollection;
protected:
	CStimResponse(idEntity *Owner, int Type);
	virtual ~CStimResponse(void);

public:
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
	 * Default means that this is a stim which has been added as default to this entity.
	 * Thiw would also mainly be used for an editor.
	 */
	bool				m_Default;

	idEntity			*m_Owner;
};

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
	void EnableStim(bool Enable = true);
	void ActivateStim(void);

	/**
	 * Add a responseentity to the ignore list. If the response is already
	 * in the list, it is not entered again.
	 */
	void AddResponseIgnore(idEntity *);
	void RemoveResponseIgnore(idEntity *);
	bool CheckResponseIgnore(idEntity *);

protected:
	/**
	 * Timer for the stimulus. If no timer is set, then it is assumed
	 * that this stimulus is always working whenever it is applied.
	 */
	CStimResponseTimer	*m_Timer;

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
	 * State of this stimuls.
	 */
	StimState			m_State;

	/**
	 * Radius defines the radius the action can reach out
	 */
	float				m_Radius;

	/**
	 * Damage that can be dealt with this stimulus. Note that this
	 * is not neccessarily a real damage always, because it depends on
	 * the actual stimulus. For example you could create a stim
	 * that deals damage (like a lava pit), but you can also create one
	 * that heals as long as you are in the vicinity (holy shrine) while at
	 * the same time deals damage to undead.
	 */

	/**
	 * TriggerDamage defines the amount of damage the stimulus will
	 * deal when it is triggered.
	 */
	float				m_TriggerDamage;

	/**
	 * DurationDamage defines the amount of damage this stimulus can deal
	 * per time. Only relevant for stimulis that are running for some time.
	 */
	float				m_DurationDamage;

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
};


class CResponse : public CStimResponse {
friend CStimResponseCollection;

public:
	void TriggerResponse(idEntity *Stim);

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
};

/**
 * CStimResponseCollection is the collection to handle all interactions within
 * stimulations and their responses. For each stim of a given type only one
 * stim/response may exist in a given collection.
 * Stim/responses should always be identified by their type instead of their pointer.
 * Handling for the objects is done via the collection class.
 */
class CStimResponseCollection {
public:
	CStimResponseCollection(void);
	~CStimResponseCollection(void);

	/**
	 * AddStim/Response creates a new stim of the given type and returns the pointer to
	 * the new object. If the stim already existed, it is not created again but the 
	 * pointer still is returned to the existing object.
	 * The returned pointer may be used for configuring or activating the stim, but it
	 * may NEVER be used to delete the object, and it should not be passed around
	 * extensively, because it may become invalid.
	 */
	CStim				*AddStim(idEntity *Owner, int Type, float Radius = 0.0f, bool Removable = true, bool Default = false);
	CResponse			*AddResponse(idEntity *Owner, int Type, bool Removable = true, bool Default = false);

	/**
	 * AddStim/Response with already configured objects. If the type already exists, the new object is not added 
	 * and the pointer to the existing object is returned, otherwise the added pointer is returned.
	 */
	CStim				*AddStim(CStim *);
	CResponse			*AddResponse(CResponse *);

	/**
	 * RemoveStim will remove the stim of the given type and the object is destroyed.
	 * Any pointer that still exists will become invalid after that.
	 * The number of remaining stims are returned.
	 */
	int				RemoveStim(int Type);
	int				RemoveResponse(int Type);
	int				RemoveStim(CStim *);
	int				RemoveResponse(CResponse *);

	/**
	 * AddEntityToList will add the given entity to the list exactly once. If the entity
	 * is already in the list, then nothing will happen and the entity stays in it.
	 */
	void				AddEntityToList(idList<idEntity *> &List, idEntity *);

	idList<CStim *>		&GetStimList(void) { return m_Stim; };
	idList<CResponse *>	&GetResponseList(void) { return m_Response; };

	CStimResponse		*GetStimResponse(int StimType, bool Stim);
	CStim				*GetStim(int StimType);
	CResponse			*GetResponse(int StimType);

	void				ParseSpawnArgsToStimResponse(const idDict *args, idEntity *Owner);
	bool				ParseSpawnArg(const idDict *args, idEntity *Owner, const char Class, int Counter);

protected:
	idList<CStim *>		m_Stim;
	idList<CResponse *>	m_Response;
};

#endif
