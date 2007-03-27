/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
/******************************************************************************/
/*                                                                            */
/*         StimResponse (C) by Gerhard W. Gruber in Germany 2005              */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

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

/*
#define GetHours(x)		((x >> 24) & 0xff)
#define GetMinutes(x)	((x >> 16) & 0xff)
#define GetSeconds(x)	((x >> 8) & 0xff)
#define GetMSeconds(x)	(x & 0xff)

#define SetHours(x)		(x << 24)
#define SetMinutes(x)	(x << 16)
#define SetSeconds(x)	(x << 8)
#define SetMSeconds(x)	(x)
*/

#define TIMER_UNDEFINED		-1

#pragma warning( push )
#pragma warning( disable : 4201 ) // non standard extension nameless struct/union
typedef union {
public:
	struct {
		signed char Flags;
		signed char Hour;
		signed char Minute;
		signed char Second;
		signed short Millisecond;
	};
	struct {
        signed long TimerVal;
		signed short Millisecond;
	};
} TimerValue;
#pragma warning( pop )

/**
 * CStimResponseTimer handles all timing aspects of stimuli.
 * Each of the values as a copy, which is used to store the actual value at runtime (<X>Val).
 * The original value is used for initialisation but is never changed during runtime
 * as only the Val counterparts will be used to track the current state.
 *
 * m_Timer is the actual timer that determines how long it takes before the stim
 * is firing. After this timer is expired, the stim will start to fire. If a duration
 * timer is also set, it will fire as long as the duration lasts. If no duration is
 * specified, the timer will fire exactly once, and then reset itself if it is a
 * restartable timer (SRTT_RELOAD) and the cycle will begin again. If a reload value
 * is set, it will only be restarted until the reloadcounter has been depleted (0).
 * If the reloadvalue is -1, it means that it will reset itself infinitely (or until
 * manually stopped).
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

public:
	/**
	 * If the stim contains information for a timed event, this function parses the string
	 * and returns a timervalue.
	 *
	 * The timer is initialized by a string on the entity which reads like this:
	 * HH:MM:SS
	 *
	 * HH are the hours in 24 hour format 0-23.
	 * MM are the minutes 0-59.
	 * SS are the seconds 0-59.
	 */
	static TimerValue ParseTimeString(idStr &s);

	virtual void SetTicks(double const &TicksPerSecond);
	virtual void SetTimer(int Hour, int Minute, int Seconds, int Milisecond);
	virtual void SetReload(int Reload);

	/**
	 * Start the timer again, after it has been stopped. If the timer 
	 * has been stopped before, but has not yet bee expired, it will
	 * just continue where it stopped which is different to Restart().
	 */
	virtual void Start(double const &t);

	/**
	 * Stop will simply stop the timer without any changes
	 */
	virtual void Stop(void);

	/**
	 * Restart will restart the timer with the next cycle. If a reload
	 * is specified it will be decreased, which means that if no more
	 * reloads are possible, restart will have no effect.
	 */
	virtual void Restart(double const &t);
	
	/**
	 * Reset will reset the timer. This means that also the reload
	 * value will be reset as well.
	 */
	virtual void Reset(void);

	void SetState(TimerState State);
	inline TimerState GetState(void) { return m_State; };

	/**
	 * The timer returns -1 if it is not working. Otherwise it will
	 * return the number of times it was triggered since the last
	 * time it was avanced. Usually this should be 1. If the number is
	 * consequently higher, it could mean that the machine is to slow
	 * to handle this timer frequency.
	 */
	virtual int Tick(double const &Ticks);

	/**
	 * Calculate the difference between two timervalues. This is usefull
	 * if you want a countdown instead of a normal clock. In this case, you
	 * can use the timer just like normal, but if you want to know the 
	 * state of the countdown just calculate the difference to your
	 * original value.
	 *
	 * The parameters are:
	 * A - B = Result
	 */
	void GetTimerValueDiff(TimerValue const &A, TimerValue const &B, TimerValue &Result) const;

protected:
	CStimResponseTimer();
	virtual ~CStimResponseTimer(void);

protected:
	double			m_LastTick;
	double			m_Ticker;
	double			m_TicksPerSecond;
	double			m_TicksPerMilliSecond;

	TimerType		m_Type;
	TimerState		m_State;

	/**
	 * How often can the stimulus be reused. -1 = unlimited
	 */
	int				m_Reload;
	int				m_ReloadVal;

	/**
	 * Timer
	 */
	TimerValue		m_Timer;
	TimerValue		m_TimerVal;
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
	ST_INVITE,			// can be used to trigger special behaviour (like a stool can invite an AI to sit down)
	ST_READ,			// Can be read
	ST_RANDOM,			// Random response is selected
	ST_TIMER,			// Timer trigger
	ST_COMMUNICATION,	// A communication stimulus (see CommunicationStim.h)
	ST_GAS,				// triggered by gas arrows
	ST_USER				= 1000,	// User defined types should use this as it's base
	ST_DEFAULT			= -1
} StimType;

typedef enum {
	SS_DISABLED,		// Stim is disabled and can not be triggered
	SS_ENABLED,			// Stim is enabled and waits for activation
	SS_DEFAULT
} StimState;

class CResponseEffect
{
	const idDict* _effectEntityDef;

	const function_t* _scriptFunction;

	// The effect postfix, "1_1" for example
	// This is passed to the script along with the "owner" entity,
	// so that the script can lookup any arguments it might need.
	idStr _effectPostfix;

public:
	// Pass the effect entity to this structure
	CResponseEffect(const idDict* effectEntityDef, 
					const function_t* scriptFunction,
					const idStr& effectPostfix) :
		_effectEntityDef(effectEntityDef),
		_scriptFunction(scriptFunction),
		_effectPostfix(effectPostfix)
	{}

	/**
	* Runs the attached response effect script 
	* (does nothing if the scriptfunc pointer is NULL)
	*
	* @owner: The entity this script is affecting
	* @stimEntity: The entity that triggered this response
	*/
	void runScript(idEntity* owner, idEntity* stimEntity);
};

/**
 * CStimResponse is the baseclass for stims and responses
 */
class CStimResponse {
friend CStimResponseCollection;
protected:
	CStimResponse(idEntity *Owner, int Type);
	virtual ~CStimResponse(void);

public:
	void EnableSR(bool Enable = true);

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
	 * State for the stim/response.
	 */
	StimState			m_State;

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
// TODO: Add additional parameters to AddStim: Magnitude, Interleave duration, duration
	CStim			*AddStim(idEntity *Owner, int Type, float Radius = 0.0f, bool Removable = true, bool Default = false);
	CResponse		*AddResponse(idEntity *Owner, int Type, bool Removable = true, bool Default = false);

	/**
	 * AddStim/Response with already configured objects. If the type already exists, the new object is not added 
	 * and the pointer to the existing object is returned, otherwise the added pointer is returned.
	 */
	CStim			*AddStim(CStim *);
	CResponse		*AddResponse(CResponse *);

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
	* Returns true if the stim response collection has any stims or responses
	**/
	bool			HasStim( void );
	bool			HasResponse( void );

	/**
	 * AddEntityToList will add the given entity to the list exactly once. If the entity
	 * is already in the list, then nothing will happen and the entity stays in it.
	 */
	void			AddEntityToList(idList<void *> &List, void *);
 
	/**
	 * If the stim contains information for a timed event, this function parses the string
	 * and creates the appropriate timer structure.
	 *
	 * The timer is comfigured by several strings on the entity:
	 * Key: sr_timer_duration
	 * Value: TimeString
	 * 
	 * HH:MM:SS
	 *
	 * HH are the hours in 24 hour format 0-23.
	 * MM are the minutes 0-59.
	 * SS are the seconds 0-59.
	 *
	 *
	 * Key: sr_timer_reload
	 * Value: N
	 *
	 * N = 0-N for the number of times it should be reloaded.
	 * A value of -1 means that it is infinitely reloaded (until disabled).
	 *
	 * Key: sr_timer_type
	 * Value: { RELOAD | SINGLESHOT }
	 *
	 * Key: sr_timer_reload_duration
	 * Value: TimeString
	 *
	 * Key: sr_timer_apply_duration
	 * Value: TimeString
	 *
	 * Key: sr_timer_time
	 * Value: Time in the format: HOURS:MINUTES:SECONDS:MILLISECONDS
	 *
	 * Key: sr_timer_waitforstart
	 * Value: { 0 | 1 } - Set true if timer should wait for StartTimer to start
	 * Otherwise starts on spawn.
	 */
	void			CreateTimer(const idDict *args, CStim *Owner, int Counter);
	void			CreateTimer(CStim *Owner);

 	idList<CStim *>	&GetStimList(void) { return m_Stim; };
	idList<CResponse *>	&GetResponseList(void) { return m_Response; };

	CStimResponse	*GetStimResponse(int StimType, bool Stim);
	CStim			*GetStim(int StimType);
	CResponse		*GetResponse(int StimType);

	void			ParseSpawnArgsToStimResponse(const idDict *args, idEntity *Owner);
	bool			ParseSpawnArg(const idDict *args, idEntity *Owner, const char Class, int Counter);

	/*
	* This static method is used to allocate, on the heap, a stim of a given type.
	* Some stim types create descended classes with virtual overrides of some stim methods.
	* It is important to always uses this instead of allocating a CStim object yourself so
	* that the correct descended class is created.
	*
	* @param p_owner A pointer to the entity which owns this stim
	*
	* @param type The enumerated stim type value
	*/
	static			CStim *createStim(idEntity* p_Owner, StimType type);

	/*
	* This static method is used to allocate, on the heap, a response of a given type.
	* Some response types create descended classes with virtual overrides of some response methods.
	* It is important to always uses this instead of allocating a CResponse object yourself so
	* that the correct descended class is created.
	*
	* @param p_owner A pointer to the entity which owns this response
	*
	* @param type The enumerated stim type value for the response
	*/
	static			CResponse *createResponse (idEntity* p_owner, StimType type);

protected:
	idList<CStim *>		m_Stim;
	idList<CResponse *>	m_Response;
};

#endif
