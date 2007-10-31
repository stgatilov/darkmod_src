/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-30 18:53:28 +0200 (Di, 30 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: State.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "State.h"
#include "../Memory.h"
#include "../Tasks/SingleBarkTask.h"
#include "../../AIComm_Message.h"
#include "SearchingState.h"

namespace ai
{

void State::Init(idAI* owner)
{
	_owner = owner;
}

// Save/Restore methods
void State::Save(idSaveGame* savefile) const
{
	_owner.Save(savefile);
}

void State::Restore(idRestoreGame* savefile)
{
	_owner.Restore(savefile);
}

void State::OnAICommMessage(CAIComm_Message* message)
{
	assert(message); // Don't accept NULL messages

	// Get the message parameters
	CAIComm_Message::TCommType commType = message->getCommunicationType();
	
	idEntity* issuingEntity = message->getIssuingEntity();
	idEntity* recipientEntity = message->getRecipientEntity();
	idEntity* directObjectEntity = message->getDirectObjectEntity();
	idVec3 directObjectLocation = message->getDirectObjectLocation();

	if (issuingEntity != NULL)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("Got incoming message from %s\r", issuingEntity->name.c_str());
	}

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMind()->GetMemory();

	switch (commType)
	{
		case CAIComm_Message::Greeting_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: Greeting_CommType\r");
			// Have seen a friend
			memory.lastTimeFriendlyAISeen = gameLocal.time;

			// If not too upset, look at them
			if (owner->AI_AlertNum < owner->thresh_2)
			{
				owner->Event_LookAtEntity(issuingEntity, 3.0); // 3 seconds
			}
			break;
		case CAIComm_Message::FriendlyJoke_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: FriendlyJoke_CommType\r");
			// Have seen a friend
			memory.lastTimeFriendlyAISeen = gameLocal.time;

			if (directObjectEntity == owner)
			{
				gameLocal.Printf("Hah, yer no better!\n");
			}
			else
			{
				gameLocal.Printf("Ha, yer right, they be an ass\n");
			}
			break;
		case CAIComm_Message::Insult_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: Insult_CommType\r");
			if (directObjectEntity == owner)
			{
				gameLocal.Printf("Same to you, buddy\n");
			}
			else if (owner->IsEnemy(directObjectEntity))
			{
				gameLocal.Printf("Hah!\n");
			}
			else
			{
				gameLocal.Printf("I'm not gettin' involved\n");
			}
			break;
		case CAIComm_Message::RequestForHelp_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForHelp_CommType\r");
			if (owner->IsFriend(issuingEntity))
			{
				// Do we already have a target we are dealing with?
				if (owner->GetEnemy() != NULL)
				{
					gameLocal.Printf("I'm too busy, I have a target!\n");
					break;
				}

				if (directObjectEntity->IsType(idActor::Type))
				{
					// Bark
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend"))
					);

					gameLocal.Printf("Ok, I'm helping you.\n");

					owner->SetEnemy(static_cast<idActor*>(directObjectEntity));
					owner->GetMind()->PerformCombatCheck();
				}
			}
			else if (owner->AI_AlertNum < owner->thresh_1*0.5f)
			{
				owner->Event_SetAlertLevel(owner->thresh_1*0.5f);
			}
			break;
		case CAIComm_Message::RequestForMissileHelp_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForMissileHelp_CommType\r");
			// Respond if they are a friend and we have a ranged weapon
			if (owner->IsFriend(issuingEntity) && owner->GetNumRangedWeapons() > 0)
			{
				// Do we already have a target we are dealing with?
				if (owner->GetEnemy() != NULL)
				{
					gameLocal.Printf("I'm too busy, I have a target!\n");
					break;
				}

				if (directObjectEntity->IsType(idActor::Type))
				{
					gameLocal.Printf("I'll attack it with my ranged weapon!\n");

					// Bark
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend"))
					);
					
					owner->SetEnemy(static_cast<idActor*>(directObjectEntity));
					owner->GetMind()->PerformCombatCheck();
				}
			}
			else 
			{
				gameLocal.Printf("I don't have a ranged weapon or I am not getting involved.\n");
				if (owner->AI_AlertNum < owner->thresh_1*0.5f)
				{
					owner->Event_SetAlertLevel(owner->thresh_1*0.5f);
				}
			}
			break;
		case CAIComm_Message::RequestForMeleeHelp_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForMeleeHelp_CommType\r");
			// Respond if they are a friend and we have a ranged weapon
			if (owner->IsFriend(issuingEntity) && owner->GetNumMeleeWeapons() > 0)
			{
				// Do we already have a target we are dealing with?
				if (owner->GetEnemy() != NULL)
				{
					gameLocal.Printf("I'm too busy, I have a target!\n");
					break;
				}

				if (directObjectEntity->IsType(idActor::Type))
				{
					gameLocal.Printf("I'll attack it with my melee weapon!\n");

					// Bark
					owner->GetSubsystem(SubsysCommunication)->PushTask(
						SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend"))
					);
					
					owner->SetEnemy(static_cast<idActor*>(directObjectEntity));
					owner->GetMind()->PerformCombatCheck();
				}
			}
			else 
			{
				gameLocal.Printf("I don't have a melee weapon or I am not getting involved.\n");
				if (owner->AI_AlertNum < owner->thresh_1*0.5f)
				{
					owner->Event_SetAlertLevel(owner->thresh_1*0.5f);
				}
			}
			break;
		case CAIComm_Message::RequestForLight_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForLight_CommType\r");
			gameLocal.Printf("I don't know how to bring light!\n");
			break;
		case CAIComm_Message::DetectedSomethingSuspicious_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: DetectedSomethingSuspicious_CommType\r");
			OnMessageDetectedSomethingSuspicious(message);
			break;
		case CAIComm_Message::DetectedEnemy_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: DetectedEnemy_CommType\r");
			break;
		case CAIComm_Message::FollowOrder_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: FollowOrder_CommType\r");
			break;
		case CAIComm_Message::GuardLocationOrder_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: GuardLocationOrder_CommType\r");
			break;
		case CAIComm_Message::GuardEntityOrder_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: GuardEntityOrder_CommType\r");
			break;
		case CAIComm_Message::PatrolOrder_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: PatrolOrder_CommType\r");
			break;
		case CAIComm_Message::SearchOrder_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: SearchOrder_CommType\r");
			break;
		case CAIComm_Message::AttackOrder_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: AttackOrder_CommType\r");
			break;
		case CAIComm_Message::GetOutOfTheWayOrder_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: GetOutOfTheWayOrder_CommType\r");
			break;
		case CAIComm_Message::ConveyWarning_EvidenceOfIntruders_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: ConveyWarning_EvidenceOfIntruders_CommType\r");
			break;
		case CAIComm_Message::ConveyWarning_ItemsHaveBeenStolen_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: ConveyWarning_ItemsHaveBeenStolen_CommType\r");
			break;
		case CAIComm_Message::ConveyWarning_EnemiesHaveBeenSeen_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: ConveyWarning_EnemiesHaveBeenSeen_CommType\r");
			break;
	}
}

void State::OnMessageDetectedSomethingSuspicious(CAIComm_Message* message)
{
	idEntity* issuingEntity = message->getIssuingEntity();
	idEntity* recipientEntity = message->getRecipientEntity();
	idEntity* directObjectEntity = message->getDirectObjectEntity();
	idVec3 directObjectLocation = message->getDirectObjectLocation();

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMind()->GetMemory();

	gameLocal.Printf("Somebody else noticed something suspicious...\n");

	if (owner->GetEnemy() != NULL)
	{
		gameLocal.Printf ("I'm too busy with my own target!");
		return;
	}

	if (owner->IsFriend(issuingEntity))
	{
		// If not already searching something else
		if (GetName() == STATE_SEARCHING)
		{
			gameLocal.Printf ("I'm too busy searching something else\n");
			return;
		}
		
		gameLocal.Printf ("They're my friend, I'll look too!\n");
		
		// Get some search points from them.
		int numSpots = owner->GetSomeOfOtherEntitiesHidingSpotList(issuingEntity);

		if (numSpots > 0)
		{
			// What is the distance to the friend.  If it is greater than a certain amount, shout intention
			// to come help
			float distanceToIssuer = (issuingEntity->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthFast();
			if (distanceToIssuer >= MIN_DISTANCE_TO_ISSUER_TO_SHOUT_COMING_TO_ASSISTANCE)
			{
				// Bark
				owner->GetSubsystem(SubsysCommunication)->PushTask(
					SingleBarkTaskPtr(new SingleBarkTask("snd_assistFriend"))
				);
			}
			
			// If AI that called out has a higher alert num, raise ours
			// to match theres due to urgency in their voice
			float otherAlertNum = 0.0f;
			
			if (issuingEntity->IsType(idAI::Type))
			{
				otherAlertNum = static_cast<idAI*>(issuingEntity)->AI_AlertNum;
			}

			gameLocal.Printf("The AI who noticed something has an alert num of %d\n", otherAlertNum);
			if (otherAlertNum > owner->AI_AlertNum)
			{
				owner->Event_SetAlertLevel(otherAlertNum);
			}
			
			memory.searchingDueToCommunication = true;
			owner->GetMind()->PushStateIfHigherPriority(STATE_SEARCHING, PRIORITY_SEARCHING);

			/*// Set time of search
			subFrameTask_determineSearchDuration();
			
			// Set time search is starting
			currentHidingSpotListSearchStartTime = sys.getTime();
					
			float spotIndex = 0; 

			// Remember which hiding spot we have chosen at start
			firstChosenHidingSpotIndex = spotIndex;
			
			// Note currently chosen hiding spot
			currentChosenHidingSpotIndex = spotIndex;
		
			// Get location
			chosenHidingSpot = getNthHidingSpotLocation (spotIndex);
			numPossibleHidingSpotsSearched = 0;
		
			waitFrame();
			b_searchingDueToCommunication = true;
			pushStateIfHigherPriority("task_SearchingHidingSpotList", PRIORITY_SEARCH_THINKING);*/
			return;
		}
		else
		{
			gameLocal.Printf("Hmpfh, no spots to help them with :(\n");
		}
		
	}
	else if (owner->AI_AlertNum < owner->thresh_1*0.5f)
	{
		owner->Event_SetAlertLevel(owner->thresh_1*0.5f);
	}
}

} // namespace ai
