/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4870 $
 * $Date: 2011-05-31 13:59:19 -0400 (Tue, 31 May 2011) $
 * $Author: grayman $
 *
 ***************************************************************************/

#ifndef __AI_EXAMINE_ROPE_H__
#define __AI_EXAMINE_ROPE_H__

#include "State.h"

namespace ai
{

#define STATE_EXAMINE_ROPE "ExamineRope"

class ExamineRopeState :
	public State
{
private:
	// Default constructor
	ExamineRopeState();

	idEntityPtr<idAFEntity_Generic> _rope;
	idVec3 _point; // the interesting point on the rope

	// time to wait before proceeding with a state
	int _waitEndTime;

	idVec3 _examineSpot;	// where to stand to examine the rope

	enum EExamineRopeState
	{
		EStateSitting,
		EStateStarting,
		EStateApproaching,
		EStateTurningToward,
		EStateExamineTop,
		EStateExamineBottom,
		EStateFinal
	} _examineRopeState;

public:
	// Constructor using rope and examination point as input
	ExamineRopeState(idAFEntity_Generic* rope, idVec3 point);

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

#endif /* __AI_EXAMINE_ROPE_H__ */
