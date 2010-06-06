/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/
#ifndef _STIMTYPE_H_
#define _STIMTYPE_H_

// If default stims are to be added here, the static array in the StimResponse.cpp file
// also must be updated. USER and UNDEFINED are not to be added though, as
// they have special meanings.
enum StimType
{
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
};

#endif /* _STIMTYPE_H_ */
