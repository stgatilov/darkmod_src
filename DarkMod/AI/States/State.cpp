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
#include "../../AIComm_Message.h"

namespace ai
{

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

	switch (commType)
	{
		case CAIComm_Message::Greeting_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: Greeting_CommType\r");
			break;
		case CAIComm_Message::FriendlyJoke_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: FriendlyJoke_CommType\r");
			break;
		case CAIComm_Message::Insult_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: Insult_CommType\r");
			break;
		case CAIComm_Message::RequestForHelp_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForHelp_CommType\r");
			break;
		case CAIComm_Message::RequestForMissileHelp_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForMissileHelp_CommType\r");
			break;
		case CAIComm_Message::RequestForMeleeHelp_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForMeleeHelp_CommType\r");
			break;
		case CAIComm_Message::RequestForLight_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: RequestForLight_CommType\r");
			break;
		case CAIComm_Message::DetectedSomethingSuspicious_CommType:
			DM_LOG(LC_AI, LT_INFO).LogString("Message Type: DetectedSomethingSuspicious_CommType\r");
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

	/*if (MessageType == Greeting_MessageType)
	{
		responseTo_Greeting (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == FriendlyJoke_MessageType)
	{
		responseTo_FriendlyJoke (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == ConveyWarning_EvidenceOfIntruders_MessageType)
	{
		responseTo_conveyWarningEvidenceOfIntruders (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == ConveyWarning_ItemsHaveBeenStolen_MessageType)
	{
		responseTo_conveyWarningItemsStolen (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == ConveyWarning_EnemiesHaveBeenSeen_MessageType)
	{
		responseTo_conveyWarningEnemiesSeen (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == Insult_MessageType)
	{
		responseTo_Insult (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == RequestForHelp_MessageType)
	{
		responseTo_RequestForHelp (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == RequestForMissileHelp_MessageType)
	{
		responseTo_RequestForMissileHelp (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == RequestForMeleeHelp_MessageType)
	{
		responseTo_RequestForMeleeHelp (issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == RequestForLight_MessageType)
	{
		responseTo_RequestForLight(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == DetectedSomethingSuspicious_MessageType)
	{
		responseTo_DetectedSomethingSuspicious(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == DetectedEnemy_MessageType)
	{
		responseTo_DetectedEnemy(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == FollowOrder_MessageType)
	{
		responseTo_FollowOrder(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == GuardLocationOrder_MessageType)
	{
		responseTo_GuardLocationOrder(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == GuardEntityOrder_MessageType)
	{
		responseTo_GuardEntityOrder(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == PatrolOrder_MessageType)
	{
		responseTo_PatrolOrder(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == SearchOrder_MessageType)
	{
		responseTo_SearchOrder(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}
	else if (MessageType == AttackOrder_MessageType)
	{
		responseTo_AttackOrder(issuingEntity, intendedRecipientEntity, directObjectEntity, directObjectLocation);
	}*/
}

} // namespace ai
