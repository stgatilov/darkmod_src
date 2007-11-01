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

// SZ: Minimum count evidence of intruders to turn on all lights encountered
#define MIN_EVIDENCE_OF_INTRUDERS_TO_TURN_ON_ALL_LIGHTS 2
// SZ: Minimum count of evidence of intruders to communicate suspicion to others
#define MIN_EVIDENCE_OF_INTRUDERS_TO_COMMUNICATE_SUSPICION 3

// SZ: Someone hearing a distress call won't bother to shout that it is coming to their assisitance unless
// it is at least this far away. This is to simulate more natural human behaivior.
#define MIN_DISTANCE_TO_ISSUER_TO_SHOUT_COMING_TO_ASSISTANCE 200

// Considered cause radius around a tactile event
#define TACTILE_ALERT_RADIUS 10.0f
#define TACTILE_SEARCH_VOLUME idVec3(40,40,40)

// Considered cause radius around a visual event
#define VISUAL_ALERT_RADIUS 25.0f
#define VISUAL_SEARCH_VOLUME idVec3(100,100,100)

// Considered cause radius around an audio event
#define AUDIO_ALERT_RADIUS 50.0f
#define AUDIO_SEARCH_VOLUME idVec3(200,200,200)

// Area searched around last sighting after losing an enemy
#define LOST_ENEMY_ALERT_RADIUS 200.0
#define LOST_ENEMY_SEARCH_VOLUME idVec3(200, 200, 200.0)

// Stim radii for various communication styles
#define YELL_STIM_RADIUS 400
#define TALK_STIM_RADIUS 200

enum EAlertType {
	EAlertVisual,
	EAlertTactile,
	EAlertAudio,
	EAlertTypeCount
};

#define MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS 15000 // milliseconds

// SZ: Maximum amount of time since last visual or audio contact with a friendly person to use
// group stimulous barks, in seconds
#define MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK 10.0f

// TODO: Parameterize these as darkmod globals
#define HIDING_OBJECT_HEIGHT 0.35f
#define MAX_SPOTS_PER_SEARCH_CALL 100

// The maximum time the AI is able to follow the enemy although it's visible
#define MAX_BLIND_CHASE_TIME 1500

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

	int	lastTimeFriendlyAISeen;

	// This is the last time the enemy was visible
	int	lastTimeEnemySeen;

	// The last time a visual stim made the AI bark
	int lastTimeVisualStimBark;

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

	// The last position the enemy was seen
	// greebo: Note: Currently this is filled in before fleeing only.
	idVec3 lastEnemyPos;

	// This is set to TRUE by the sensory routines to indicate whether
	// the AI is in the position to damage the player.
	// This flag is mostly for caching purposes so that the subsystem tasks
	// don't need to query idAI::CanHitEnemy() independently.
	bool canHitEnemy;

	/*!
	* This is the time at which the current hiding spot
	* list search started
	* @author SophisticatedZombie
	*/
	int currentHidingSpotListSearchStartTime;

	/*!
	* This is the maximum duration of the current
	* hiding spot list search in msec.
	* @author SophisticatedZombie
	*/
	int currentHidingSpotListSearchMaxDuration;
	
	/*!
	* This is the number of hiding spots from the current
	* hiding spot list which have been searched
	* @author SophisticatedZombie
	*/
	int numPossibleHidingSpotsSearched;

	/*!
	* These hold the current spot search target, regardless of whether
	* or not it is a hiding spot search or some other sort of spot search
	*/
	idVec3 currentSearchSpot;

	/*!
	* This flag indicates if a hiding spot test was started
	* @author SophisticatedZombie
	*/
	bool hidingSpotTestStarted;

	/*!
	* This flag idnicates if a hiding spot was chosen
	*/
	bool hidingSpotSearchDone;

	// This counts the number of frames we have been thinking, in case
	// we have a problem with hiding spot searches not returning
	int hidingSpotThinkFrameCount;

	int firstChosenHidingSpotIndex;
	int currentChosenHidingSpotIndex;
	idVec3 chosenHidingSpot;

	// True if the AI is currently investigating a hiding spot (walking to it, for instance).
	bool hidingSpotInvestigationInProgress;

	// True if fleeing is done, false if fleeing is in progress
	bool fleeingDone;

	Memory() :
		alertState(ERelaxed),
		lastPatrolChatTime(-1),
		lastTimeFriendlyAISeen(-1000),
		lastTimeVisualStimBark(-1),
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
		alertSearchExclusionVolume(0,0,0),
		lastEnemyPos(0,0,0),
		canHitEnemy(false),
		currentHidingSpotListSearchStartTime(-1),
		currentHidingSpotListSearchMaxDuration(-1),
		numPossibleHidingSpotsSearched(0),
		currentSearchSpot(0,0,0),
		hidingSpotTestStarted(false),
		hidingSpotSearchDone(true),
		hidingSpotThinkFrameCount(0),
		firstChosenHidingSpotIndex(0),
		currentChosenHidingSpotIndex(0),
		chosenHidingSpot(0,0,0),
		hidingSpotInvestigationInProgress(false),
		fleeingDone(true)
	{}

	// Save/Restore routines
	void Save(idSaveGame* savefile) const
	{
		savefile->WriteInt(static_cast<int>(alertState));
		currentPath.Save(savefile);
		savefile->WriteInt(lastPatrolChatTime);
		savefile->WriteInt(countEvidenceOfIntruders);
		savefile->WriteInt(lastRandomHeadTurnCheckTime);
		savefile->WriteInt(lastTimeFriendlyAISeen);
		savefile->WriteInt(lastTimeVisualStimBark);
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
		savefile->WriteVec3(lastEnemyPos);
		savefile->WriteBool(canHitEnemy);
		savefile->WriteInt(currentHidingSpotListSearchStartTime);
		savefile->WriteInt(currentHidingSpotListSearchMaxDuration);
		savefile->WriteInt(numPossibleHidingSpotsSearched);
		savefile->WriteVec3(currentSearchSpot);
		savefile->WriteBool(hidingSpotTestStarted);
		savefile->WriteBool(hidingSpotSearchDone);
		savefile->WriteInt(hidingSpotThinkFrameCount);
		savefile->WriteInt(firstChosenHidingSpotIndex);
		savefile->WriteInt(currentChosenHidingSpotIndex);
		savefile->WriteVec3(chosenHidingSpot);
		savefile->WriteBool(hidingSpotInvestigationInProgress);
		savefile->WriteBool(fleeingDone);
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
		savefile->ReadInt(lastTimeFriendlyAISeen);
		savefile->ReadInt(lastTimeVisualStimBark);
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
		savefile->ReadVec3(lastEnemyPos);
		savefile->ReadBool(canHitEnemy);
		savefile->ReadInt(currentHidingSpotListSearchStartTime);
		savefile->ReadInt(currentHidingSpotListSearchMaxDuration);
		savefile->ReadInt(numPossibleHidingSpotsSearched);
		savefile->ReadVec3(currentSearchSpot);
		savefile->ReadBool(hidingSpotTestStarted);
		savefile->ReadBool(hidingSpotSearchDone);
		savefile->ReadInt(hidingSpotThinkFrameCount);
		savefile->ReadInt(firstChosenHidingSpotIndex);
		savefile->ReadInt(currentChosenHidingSpotIndex);
		savefile->ReadVec3(chosenHidingSpot);
		savefile->ReadBool(hidingSpotInvestigationInProgress);
		savefile->ReadBool(fleeingDone);
	}
};

} // namespace ai

#endif /*__AI_MEMORY_H__*/
