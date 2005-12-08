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
collection, and all stim/responses are added to this collection.
******************************************************************************/

/**
 * CStimResponseTimer handles all timing aspects of stimuli.
 * Each of the values as a copy, which is used to store the actual value at runtime.
 * The original value is used for initialisation but is never changed during runtime
 * as only the Val counterparts will be used to track the current state.
 */
class CStimResponseTimer {
public:
	typedef enum {
		SRTT_SINGLESHOT,			// Stimuls can be used exactly once
		SRTT_RELOAD,				// Stimulus can be reused
		SRTT_DEFAULT
	} TimerType;

	typedef enum {
		SRTS_TRIGGERED,				// Stimuls just became active
		SRTS_RUNNING,				// Stimulus is progressing (only relevant for durations)
		SRTS_EXPIRED,				// Stimulus has been expired (can be reloaded of reload is set)
		SRTS_DEFAULT
	} TimerState;

public:
	CStimResponseTimer(void);
	~CStimResponseTimer(void);

protected:
	TimerType		m_Type;
	TimerState		m_State;

	/**
	 * How long does it take until this stimulus can be used again.
	 */
	int				m_Reload;
	int				m_ReloadVal;

	/**
	 * How often can the stimulus be reused.
	 */
	int				m_ReloadTimer;
	int				m_ReloadTimerVal;

	/**
	 * How long does the stimulus need to be applied, before the action is
	 * triggered.
	 * i.E. When a candle is hold to a book, it will take some time until
	 * it catches fire.
	 */
	int				m_Apply;
	int				m_ApplyVal;

	/**
	 * How often can the stimulus be applied before it is depleted.
	 */
	int				m_ApplyCounter;
	int				m_ApplyCounterVal;

	/**
	 * How long is the stimulus performing it's action
	 */
	int				m_Duration;
	int				m_DurationVal;
};


typedef enum {
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
	ST_SIT,				// can be used to sit down
	ST_READ,			// Can be read
	ST_RANDOM,			// Random response is selected
	ST_DEFAULT
} StimType;

typedef enum {
	SS_DISABLED,
	SS_ENABLED,
	SS_DEFAULT
} StimState;

/**
 * CResponse is a base class for the stimulis.
 */
class CStimulus {
public:
	CStimulus(void);
	virtual ~CStimulus(void);

protected:
	/**
	 * Timer for the stimulus. If no timer is set, then it is assumed
	 * that this stimulus is always working whenever it is applied.
	 */
	CStimResponseTimer	*m_Timer;

	/**
	 * Id for the stimulus that uniquely identifies a stim, so they can
	 * be associated to each other.
	 */
	int					m_StimTypeId;

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
	 * the actual stimulus. For example you could create a stimulus
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
};


class CResponse {
public:
	CResponse(void);
	virtual ~CResponse(void);

protected:
	/**
	 * Once this stimuls is finished, another one can be fired. So you
	 * can chain multiple stimulis one after the other. If NULL this is
	 * a single event.
	 */
	CResponse		*m_FollowUp;

	/**
	 * Scriptfunction that is to be executed when this response 
	 * is triggered.
	 */
	char			*m_ScriptFunction;

	/**
	 * Id for the stimulus that uniquely identifies a stim, so they can
	 * be associated to each other.
	 */
	int					m_StimTypeId;

	/**
	 * How much damage must be applied for this response?
	 */
	float				m_MinDamage;

	/**
	 * No more than this.
	 */
	float				m_MaxDamage;
};

/**
 * CStimResponseCollection is the collection to handle all interactions within
 * stimulations and their responses.
 */
class CStimResponseCollection {
public:
	CStimResponseCollection(void);
	~CStimResponseCollection(void);

protected:
	/**
	 * MaxRadius is the biggest radius for any given stimulus, currently in 
	 * the collection.
	 */
	float			m_MaxRadius;
};


#endif
