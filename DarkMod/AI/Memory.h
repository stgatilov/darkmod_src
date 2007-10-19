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

// Considered cause radius around a tactile event
#define TACTILE_ALERT_RADIUS 10.0f
#define TACTILE_SEARCH_VOLUME idVec3(40,40,40)

// Considered cause radius around a visual event
#define VISUAL_ALERT_RADIUS 25.0f
#define VISUAL_SEARCH_VOLUME idVec3(100,100,100)

// Considered cause radius around an audio event
#define AUDIO_ALERT_RADIUS 50.0f
#define AUDIO_SEARCH_VOLUME idVec3(200,200,200)


enum EAlertType {
	EAlertVisual,
	EAlertTactile,
	EAlertAudio,
	EAlertTypeCount
};

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

	// position of alert causing stimulus
	idVec3 alertPos;

	// Type of alert (visual, tactile, audio)
	EAlertType alertType;

	// radius of alert causing stimulus (depends on the type and distance)
	float alertRadius;

	// This is true if the original alert position is to be searched
	bool stimulusLocationItselfShouldBeSearched;

	// This flag indicates if the search is due to a communication
	bool searchingDueToCommunication;

	// Position of the last alert causing stimulus which was searched.
    // This is used to compare new stimuli to the previous stimuli searched
    // to determine if a new search is necessary
	idVec3 lastAlertPosSearched;

	// A search area vector that is m_alertRadius on each side
	idVec3 alertSearchVolume;

	// An area within the search volume that is to be ignored. It is used for expanding
	// radius searches that don't re-search the inner points.
	idVec3 alertSearchExclusionVolume;

	Memory() :
		alertState(ERelaxed),
		lastPatrolChatTime(-1),
		countEvidenceOfIntruders(0),
		lastRandomHeadTurnCheckTime(-1),
		enemiesHaveBeenSeen(false),
		itemsHaveBeenStolen(false),
		alertPos(0,0,0),
		alertType(EAlertTypeCount),
		alertRadius(-1),
		stimulusLocationItselfShouldBeSearched(false),
		searchingDueToCommunication(false),
		lastAlertPosSearched(0,0,0),
		alertSearchVolume(0,0,0),
		alertSearchExclusionVolume(0,0,0)
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
		savefile->WriteVec3(alertPos);
		savefile->WriteInt(static_cast<int>(alertType));
		savefile->WriteFloat(alertRadius);
		savefile->WriteBool(stimulusLocationItselfShouldBeSearched);
		savefile->WriteBool(searchingDueToCommunication);
		savefile->WriteVec3(lastAlertPosSearched);
		savefile->WriteVec3(alertSearchVolume);
		savefile->WriteVec3(alertSearchExclusionVolume);
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
		savefile->ReadVec3(alertPos);

		savefile->ReadInt(temp);
		alertType = static_cast<EAlertType>(temp);

		savefile->ReadFloat(alertRadius);
		savefile->ReadBool(stimulusLocationItselfShouldBeSearched);
		savefile->ReadBool(searchingDueToCommunication);
		savefile->ReadVec3(lastAlertPosSearched);
		savefile->ReadVec3(alertSearchVolume);
		savefile->ReadVec3(alertSearchExclusionVolume);
	}
};

} // namespace ai

#endif /*__AI_MEMORY_H__*/
