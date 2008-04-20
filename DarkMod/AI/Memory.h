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
#include "../BinaryFrobMover.h"
#include "../FrobDoor.h"
#include "DoorInfo.h"

namespace ai
{

#define AIUSE_WEAPON			"AIUSE_WEAPON"
#define AIUSE_LIGHTSOURCE		"AIUSE_LIGHTSOURCE"
#define AIUSE_BLOOD_EVIDENCE	"AIUSE_BLOOD_EVIDENCE"
#define AIUSE_SEAT				"AIUSE_SEAT"
#define AIUSE_COOK				"AIUSE_COOK"
#define AIUSE_EAT				"AIUSE_EAT"
#define AIUSE_PET				"AIUSE_PET"
#define AIUSE_MONSTER			"AIUSE_MONSTER"  // a random or caged monster, not a pet
#define AIUSE_UNDEAD			"AIUSE_UNDEAD" // An undead creature
#define AIUSE_CATTLE			"AIUSE_CATTLE"
#define AIUSE_PERSON			"AIUSE_PERSON"
#define AIUSE_PEST				"AIUSE_PEST"
#define AIUSE_DRINK			"AIUSE_DRINK"
#define AIUSE_DOOR				"AIUSE_DOOR"
#define AIUSE_ELEVATOR			"AIUSE_ELEVATOR"
#define AIUSE_MISSING_ITEM_MARKER "AIUSE_MISSING_ITEM_MARKER"

//----------------------------------------------------------------------------------------
// The following key and values are used for identifying types of lights
#define AIUSE_LIGHTTYPE_KEY		"lightType"
#define AIUSE_LIGHTTYPE_TORCH	"AIUSE_LIGHTTYPE_TORCH"
#define AIUSE_LIGHTTYPE_GASLAMP	 "AIUSE_LIGHTTYPE_GASLAMP"
#define AIUSE_LIGHTTYPE_ELECTRIC "AIUSE_LIGHTTYPE_ELECTRIC"
#define AIUSE_LIGHTTYPE_MAGIC	 "AIUSE_LIGHTTYPE_MAGIC"
#define AIUSE_LIGHTTYPE_AMBIENT	 "AIUSE_LIGHTTYPE_AMBIENT"

//----------------------------------------------------------------------------------------
// The following key is used to identify the name of the switch entity used to turn on
// a AIUSE_LIGHTTYPE_ELECTRIC light.
#define AIUSE_LIGHTSWITCH_NAME_KEY	"switchName"

//----------------------------------------------------------------------------------------
// The following defines a key that should be non-0 if the device should be on
#define AIUSE_SHOULDBEON_KEY		"shouldBeOn"


// SZ: Minimum count evidence of intruders to turn on all lights encountered
#define MIN_EVIDENCE_OF_INTRUDERS_TO_TURN_ON_ALL_LIGHTS 5
// angua: The AI starts searching after encountering a switched off light 
// only if it is already suspicious
#define MIN_EVIDENCE_OF_INTRUDERS_TO_SEARCH_ON_LIGHT_OFF 3
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
#define AUDIO_ALERT_FUZZINESS 100.0f
#define AUDIO_SEARCH_VOLUME idVec3(300,300,200)

// Area searched around last sighting after losing an enemy
#define LOST_ENEMY_ALERT_RADIUS 200.0
#define LOST_ENEMY_SEARCH_VOLUME idVec3(200, 200, 200.0)

// Stim radii for various communication styles
#define YELL_STIM_RADIUS 400
#define TALK_STIM_RADIUS 200

enum EAlertClass 
{
	EAlertNone,
	EAlertVisual,
	EAlertTactile,
	EAlertAudio,
	EAlertClassCount
};

enum EAlertType
{
	EAlertTypeNone,
	EAlertTypeEnemy,
	EAlertTypeWeapon,
	EAlertTypeDeadPerson,
	EAlertTypeUnconsciousPerson,
	EAlertTypeBlood,
	EAlertTypeLightSource,
	EAlertTypeMissingItem,
	EAlertTypeDoor,
	EAlertTypeDamage,
	EAlertTypeCount
};

// The alert index the AI is in
enum EAlertState {
	ERelaxed = 0,
	EObservant,
	ESuspicious,
	EInvestigating,
	EAgitatedSearching,
	ECombat,
	EAlertStateNum
};

#define MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS 15000 // milliseconds

// SZ: Maximum amount of time since last visual or audio contact with a friendly person to use
// group stimulous barks, in seconds
#define MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK 10.0f

// TODO: Parameterize these as darkmod globals
#define HIDING_OBJECT_HEIGHT 0.35f
#define MAX_SPOTS_PER_SEARCH_CALL 100

// The maximum time the AI is able to follow the enemy although it's invisible
#define MAX_BLIND_CHASE_TIME 3000

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

	// The game time, the AlertLevel was last increased.
	int lastAlertRiseTime;

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

	// TRUE if the AI has found a dead or unconscious person
	bool unconsciousPeopleHaveBeenFound;
	bool deadPeopleHaveBeenFound;

	// position of alert causing stimulus
	idVec3 alertPos;

	// Type of alert (visual, tactile, audio)
	EAlertClass alertClass;

	// Source of the alert (enemy, weapon, blood, dead person, etc.)
	EAlertType alertType;

	// radius of alert causing stimulus (depends on the type and distance)
	float alertRadius;

	// This is true if the original alert position is to be searched
	bool stimulusLocationItselfShouldBeSearched;

	// Set this to TRUE if stimulus location itself should be closely investigated (kneel down)
	bool investigateStimulusLocationClosely;

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

	/**
	 * greebo: This is queried by the SearchStates and indicates a new
	 *         stimulus to be considered.
	 */
	bool restartSearchForHidingSpots;

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

	// angua: The last position of the AI before it takes cover, so it can return to it later.
	idVec3 positionBeforeTakingCover;

	typedef std::map<CFrobDoor*, DoorInfoPtr> DoorInfoMap;
	// Variables related to door opening/closing process
	struct DoorRelatedVariables
	{
		idEntityPtr<CBinaryFrobMover> currentFrobMover;

		DoorInfoMap doorInfo;
	} doorRelated;

	Memory() :
		alertState(ERelaxed),
		lastAlertRiseTime(-1),
		lastPatrolChatTime(-1),
		lastTimeFriendlyAISeen(-1000),
		lastTimeEnemySeen(-1),
		lastTimeVisualStimBark(-1),
		countEvidenceOfIntruders(0),
		lastRandomHeadTurnCheckTime(-1),
		enemiesHaveBeenSeen(false),
		itemsHaveBeenStolen(false),
		unconsciousPeopleHaveBeenFound(false),
		deadPeopleHaveBeenFound(false),
		alertPos(0,0,0),
		alertClass(EAlertClassCount),
		alertType(EAlertTypeCount),
		alertRadius(-1),
		stimulusLocationItselfShouldBeSearched(false),
		investigateStimulusLocationClosely(false),
		searchingDueToCommunication(false),
		lastAlertPosSearched(0,0,0),
		alertSearchVolume(0,0,0),
		alertSearchExclusionVolume(0,0,0),
		lastEnemyPos(0,0,0),
		canHitEnemy(false),
		numPossibleHidingSpotsSearched(0),
		currentSearchSpot(0,0,0),
		hidingSpotTestStarted(false),
		hidingSpotSearchDone(true),
		restartSearchForHidingSpots(false),
		hidingSpotThinkFrameCount(0),
		firstChosenHidingSpotIndex(0),
		currentChosenHidingSpotIndex(0),
		chosenHidingSpot(0,0,0),
		hidingSpotInvestigationInProgress(false),
		fleeingDone(true),
		positionBeforeTakingCover(0,0,0)
	{}

	// Save/Restore routines
	void Save(idSaveGame* savefile) const
	{
		savefile->WriteInt(static_cast<int>(alertState));
		currentPath.Save(savefile);
		savefile->WriteInt(lastAlertRiseTime);
		savefile->WriteInt(lastPatrolChatTime);
		savefile->WriteInt(lastTimeFriendlyAISeen);
		savefile->WriteInt(lastTimeEnemySeen);
		savefile->WriteInt(lastTimeVisualStimBark);
		savefile->WriteInt(countEvidenceOfIntruders);
		savefile->WriteInt(lastRandomHeadTurnCheckTime);
		savefile->WriteBool(enemiesHaveBeenSeen);
		savefile->WriteBool(itemsHaveBeenStolen);
		savefile->WriteBool(unconsciousPeopleHaveBeenFound);
		savefile->WriteBool(deadPeopleHaveBeenFound);
		savefile->WriteVec3(alertPos);
		savefile->WriteInt(static_cast<int>(alertClass));
		savefile->WriteInt(static_cast<int>(alertType));
		savefile->WriteFloat(alertRadius);
		savefile->WriteBool(stimulusLocationItselfShouldBeSearched);
		savefile->WriteBool(investigateStimulusLocationClosely);
		savefile->WriteBool(searchingDueToCommunication);
		savefile->WriteVec3(lastAlertPosSearched);
		savefile->WriteVec3(alertSearchVolume);
		savefile->WriteVec3(alertSearchExclusionVolume);
		savefile->WriteVec3(lastEnemyPos);
		savefile->WriteBool(canHitEnemy);
		savefile->WriteInt(numPossibleHidingSpotsSearched);
		savefile->WriteVec3(currentSearchSpot);
		savefile->WriteBool(hidingSpotTestStarted);
		savefile->WriteBool(hidingSpotSearchDone);
		savefile->WriteBool(restartSearchForHidingSpots);
		savefile->WriteInt(hidingSpotThinkFrameCount);
		savefile->WriteInt(firstChosenHidingSpotIndex);
		savefile->WriteInt(currentChosenHidingSpotIndex);
		savefile->WriteVec3(chosenHidingSpot);
		savefile->WriteBool(hidingSpotInvestigationInProgress);
		savefile->WriteBool(fleeingDone);
		savefile->WriteVec3(positionBeforeTakingCover);

		doorRelated.currentFrobMover.Save(savefile);

		savefile->WriteInt(doorRelated.doorInfo.size());
		for (DoorInfoMap::const_iterator i = doorRelated.doorInfo.begin();
			 i != doorRelated.doorInfo.end(); i++)
		{
			savefile->WriteObject(i->first);
			i->second->Save(savefile);
		}
	}

	void Restore(idRestoreGame* savefile)
	{
		int temp;
		savefile->ReadInt(temp);
		alertState = static_cast<EAlertState>(temp);

		currentPath.Restore(savefile);
		savefile->ReadInt(lastAlertRiseTime);
		savefile->ReadInt(lastPatrolChatTime);
		savefile->ReadInt(lastTimeFriendlyAISeen);
		savefile->ReadInt(lastTimeEnemySeen);
		savefile->ReadInt(lastTimeVisualStimBark);
		savefile->ReadInt(countEvidenceOfIntruders);
		savefile->ReadInt(lastRandomHeadTurnCheckTime);
		savefile->ReadBool(enemiesHaveBeenSeen);
		savefile->ReadBool(itemsHaveBeenStolen);
		savefile->ReadBool(unconsciousPeopleHaveBeenFound);
		savefile->ReadBool(deadPeopleHaveBeenFound);
		savefile->ReadVec3(alertPos);

		savefile->ReadInt(temp);
		alertClass = static_cast<EAlertClass>(temp);

		savefile->ReadInt(temp);
		alertType = static_cast<EAlertType>(temp);

		savefile->ReadFloat(alertRadius);
		savefile->ReadBool(stimulusLocationItselfShouldBeSearched);
		savefile->ReadBool(investigateStimulusLocationClosely);
		savefile->ReadBool(searchingDueToCommunication);
		savefile->ReadVec3(lastAlertPosSearched);
		savefile->ReadVec3(alertSearchVolume);
		savefile->ReadVec3(alertSearchExclusionVolume);
		savefile->ReadVec3(lastEnemyPos);
		savefile->ReadBool(canHitEnemy);
		savefile->ReadInt(numPossibleHidingSpotsSearched);
		savefile->ReadVec3(currentSearchSpot);
		savefile->ReadBool(hidingSpotTestStarted);
		savefile->ReadBool(hidingSpotSearchDone);
		savefile->ReadBool(restartSearchForHidingSpots);
		savefile->ReadInt(hidingSpotThinkFrameCount);
		savefile->ReadInt(firstChosenHidingSpotIndex);
		savefile->ReadInt(currentChosenHidingSpotIndex);
		savefile->ReadVec3(chosenHidingSpot);
		savefile->ReadBool(hidingSpotInvestigationInProgress);
		savefile->ReadBool(fleeingDone);
		savefile->ReadVec3(positionBeforeTakingCover);

		doorRelated.currentFrobMover.Restore(savefile);
		doorRelated.doorInfo.clear();

		int num;
		savefile->ReadInt(num);
		for (int i = 0; i < num; i++)
		{
			CFrobDoor* door;
			savefile->ReadObject(reinterpret_cast<idClass*&>(door));

			// Allocate a new doorinfo structure and insert it into the map
			DoorInfoPtr info(new DoorInfo);

			std::pair<DoorInfoMap::iterator, bool> result = 
				doorRelated.doorInfo.insert(DoorInfoMap::value_type(door, info));

			// The insertion must succeed (unique pointers in the map!), otherwise we have inconsistent data
			assert(result.second == true);

			info->Restore(savefile);
		}
	}

	DoorInfo& GetDoorInfo(CFrobDoor* door)
	{
		DoorInfoMap::iterator i = doorRelated.doorInfo.find(door);

		if (i != doorRelated.doorInfo.end())
		{
			return *(i->second);
		}
		else
		{
			DoorInfoPtr info(new DoorInfo);
			std::pair<DoorInfoMap::iterator, bool> result =
				doorRelated.doorInfo.insert(DoorInfoMap::value_type(door, info));
			return *(result.first->second);
		}
	}
	
};

} // namespace ai

#endif /*__AI_MEMORY_H__*/
