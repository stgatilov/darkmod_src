/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-04-22 18:53:28 +0200 (Di, 22 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: Memory.cpp 1435 2008-04-22 16:53:28Z greebo $", init_version);

#include "Memory.h"
#include "../../game/ai/ai.h"

namespace ai
{

Memory::Memory(idAI* owningAI) :
	owner(owningAI),
	alertState(ERelaxed),
	lastAlertRiseTime(-1),
	lastPatrolChatTime(-1),
	lastTimeFriendlyAISeen(-1000),
	lastTimeEnemySeen(-1),
	lastTimeVisualStimBark(-1),
	countEvidenceOfIntruders(0),
	nextHeadTurnCheckTime(0),
	currentlyHeadTurning(false),
	headTurnEndTime(0),
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
	alertSearchCenter(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
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
void Memory::Save(idSaveGame* savefile) const
{
	savefile->WriteObject(owner);
	savefile->WriteInt(static_cast<int>(alertState));
	currentPath.Save(savefile);
	savefile->WriteInt(lastAlertRiseTime);
	savefile->WriteInt(lastPatrolChatTime);
	savefile->WriteInt(lastTimeFriendlyAISeen);
	savefile->WriteInt(lastTimeEnemySeen);
	savefile->WriteInt(lastTimeVisualStimBark);
	savefile->WriteInt(countEvidenceOfIntruders);
	savefile->WriteInt(nextHeadTurnCheckTime);
	savefile->WriteBool(currentlyHeadTurning);
	savefile->WriteInt(headTurnEndTime);
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
	savefile->WriteVec3(alertSearchCenter);
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

	doorRelated.currentDoor.Save(savefile);

	savefile->WriteInt(doorRelated.doorInfo.size());
	for (DoorInfoMap::const_iterator i = doorRelated.doorInfo.begin();
		 i != doorRelated.doorInfo.end(); i++)
	{
		savefile->WriteObject(i->first);
		i->second->Save(savefile);
	}

	// greebo: Don't save the AAS areaNum => DoorInfo mapping (can be re-calculated at restore time)
}

void Memory::Restore(idRestoreGame* savefile)
{
	savefile->ReadObject(reinterpret_cast<idClass*&>(owner));

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
	savefile->ReadInt(nextHeadTurnCheckTime);
	savefile->ReadBool(currentlyHeadTurning);
	savefile->ReadInt(headTurnEndTime);
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
	savefile->ReadVec3(alertSearchCenter);
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

	doorRelated.currentDoor.Restore(savefile);
	// Clear the containers before restoring them
	doorRelated.doorInfo.clear();
	doorRelated.areaDoorInfoMap.clear();

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

	// greebo: Reconstruct the AAS areaNum => DoorInfo mapping
	for (DoorInfoMap::iterator i = doorRelated.doorInfo.begin();
		 i != doorRelated.doorInfo.end(); i++)
	{
		// Use the areanumber as index and insert the pointer into the map
		doorRelated.areaDoorInfoMap.insert(
			AreaToDoorInfoMap::value_type(i->second->areaNum, i->second)
		);
	}
}

DoorInfo& Memory::GetDoorInfo(CFrobDoor* door)
{
	DoorInfoMap::iterator i = doorRelated.doorInfo.find(door);

	if (i != doorRelated.doorInfo.end())
	{
		return *(i->second);
	}
	else
	{
		DoorInfoPtr info(new DoorInfo);
		// Set the area number
		info->areaNum = door->GetFrobMoverAasArea(owner->GetAAS());
		// Insert into the map
		std::pair<DoorInfoMap::iterator, bool> result =
			doorRelated.doorInfo.insert(DoorInfoMap::value_type(door, info));

		// Add the areaNum => info mapping for faster lookup using area numbers
		doorRelated.areaDoorInfoMap.insert(
			AreaToDoorInfoMap::value_type(door->GetFrobMoverAasArea(owner->GetAAS()), info)
		);

		return *(result.first->second);
	}
}

DoorInfoPtr Memory::GetDoorInfo(int areaNum)
{
	AreaToDoorInfoMap::iterator i = doorRelated.areaDoorInfoMap.find(areaNum);
	return (i != doorRelated.areaDoorInfoMap.end()) ? i->second : DoorInfoPtr();
}

} // namespace ai
