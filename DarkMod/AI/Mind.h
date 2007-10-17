/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_MIND_H__
#define __AI_MIND_H__

#include "../idlib/precompiled.h"

#include <boost/shared_ptr.hpp>

namespace ai
{

enum EAlertState {
	ERelaxed,
	EAroused,
	EInvestigating,
	EAgitatedSearching,
	ECombat,
};

/**
 * greebo: This defines the ABC of an AI mind. It basically
 *         handles the incoming stimuli and emits signals to the 
 *         AI subsystems like movement, interaction and sensory stuff.
 */
class Mind
{
public:
	/**
	 * greebo: This should be called each frame to let the AI
	 *         think. This distributes the call to the various
	 *         subsystem's Think() methods, maybe in and interleaved way.
	 */
	virtual void Think() = 0;

	// Get the current alert state 
	virtual EAlertState GetAlertState() const = 0;

	// Set the current alert state
	virtual void SetAlertState(EAlertState newState) = 0;

	// Save/Restore routines
	virtual void Save(idSaveGame* savefile) const = 0;
	virtual void Restore(idRestoreGame* savefile) = 0;
};
typedef boost::shared_ptr<Mind> MindPtr;

} // namespace ai

#endif /*__AI_MIND_H__*/
