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

	
}

} // namespace ai
