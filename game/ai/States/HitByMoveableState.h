/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision: 5121 $ (Revision of last commit) 
 $Date: 2011-12-11 14:12:26 -0500 (Sun, 11 Dec 2011) $ (Date of last commit)
 $Author: greebo $ (Author of last commit)
 
******************************************************************************/

#ifndef __AI_HIT_BY_MOVEABLE_H__
#define __AI_HIT_BY_MOVEABLE_H__

#include "State.h"

namespace ai
{

#define STATE_HIT_BY_MOVEABLE "HitByMoveable"

// grayman #2816 - constants for looking at objects that strike the AI
const int HIT_DELAY	     = 1000; // ms - when getting hit by something, wait this long before turning toward it
const int HIT_DURATION	 = 2000; // ms - and look at it for this long (+/- a random variation)
const int HIT_VARIATION  =  400; // ms - max variation
const int HIT_DIST		 =  150; // pick a point this far away, back where the object came from and look at it
const int HIT_FIND_THROWER_HORZ = 300; // how far out to look for a friendly/neutral AI
const int HIT_FIND_THROWER_VERT = 150; // how far up/down to look for a friendly/neutral AI

class HitByMoveableState :
	public State
{
private:
	idVec3 _pos;							// a position to look back at
	idEntityPtr<idActor> _responsibleActor;	// who threw the object

	// time to wait before proceeding with a state
	int _waitEndTime;

	enum EHitByMoveableState
	{
		EStateSittingSleeping,
		EStateStarting,
		EStateTurnToward,
		EStateLookAt,
		EStateTurnBack,
		EStateLookBack,
		EStateFinal
	} _hitByMoveableState;

public:
	// Default constructor
	HitByMoveableState();

	// Get the name of this state
	virtual const idStr& GetName() const;

	virtual void Wrapup(idAI* owner);

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner);

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner);

	// Look at top of rope
	void StartExaminingTop(idAI* owner);

	// Look at bottom of rope
	void StartExaminingBottom(idAI* owner);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	static StatePtr CreateInstance();
};

} // namespace ai

#endif /* __AI_HIT_BY_MOVEABLE_H__ */
