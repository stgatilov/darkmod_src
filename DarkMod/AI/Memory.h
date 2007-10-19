/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_MEMORY_H__
#define __AI_MEMORY_H__

#include "../idlib/precompiled.h"

namespace ai
{

// SZ: Minimum count of evidence of intruders to communicate suspicion to others
#define MIN_EVIDENCE_OF_INTRUDERS_TO_COMMUNICATE_SUSPICION 3

/**
 * greebo: This class acts as container for all kinds of state variables.
 */
class Memory
{
public:
	// The alert state we're currently in
	EAlertState	alertState;

	// The path entity we're supposed to be heading to
	idEntityPtr<idPathCorner> currentPath;

	// The last time the AI has been barking when patrolling
	int lastPatrolChatTime;

	/*!
	* This variable indicates the number of out of place things that the
	* AI has witness, such as sounds, missing items, open doors, torches gone
	* out etc..
	*/
	int countEvidenceOfIntruders;

	// The last time a check for randomly turning the head was done
	int lastRandomHeadTurnCheckTime;

	// TRUE if enemies have been seen
	bool enemiesHaveBeenSeen;

	// TRUE if the AI knows that items have been stolen
	bool itemsHaveBeenStolen;

	Memory() :
		alertState(ERelaxed),
		lastPatrolChatTime(-1),
		countEvidenceOfIntruders(0),
		lastRandomHeadTurnCheckTime(-1),
		enemiesHaveBeenSeen(false),
		itemsHaveBeenStolen(false)
	{}

	// Save/Restore routines
	void Save(idSaveGame* savefile) const
	{
		savefile->WriteInt(static_cast<int>(alertState));
		currentPath.Save(savefile);
		savefile->WriteInt(lastPatrolChatTime);
		savefile->WriteInt(countEvidenceOfIntruders);
		savefile->WriteInt(lastRandomHeadTurnCheckTime);
		savefile->WriteBool(enemiesHaveBeenSeen);
		savefile->WriteBool(itemsHaveBeenStolen);
	}

	void Restore(idRestoreGame* savefile)
	{
		int temp;
		savefile->ReadInt(temp);
		alertState = static_cast<EAlertState>(temp);

		currentPath.Restore(savefile);
		savefile->ReadInt(lastPatrolChatTime);
		savefile->ReadInt(countEvidenceOfIntruders);
		savefile->ReadInt(lastRandomHeadTurnCheckTime);
		savefile->ReadBool(enemiesHaveBeenSeen);
		savefile->ReadBool(itemsHaveBeenStolen);
	}
};

} // namespace ai

#endif /*__AI_MEMORY_H__*/
