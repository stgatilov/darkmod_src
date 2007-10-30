/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "DarkModGlobals.h"
#include "AIComm_StimResponse.h"
#include "StimResponse/StimResponseCollection.h"
#include "AI/States/State.h"

#define MAX_COMMUNICATION_RADIUS 5000.0

/*
**************************************************************************
* CAIComm_Response
**************************************************************************
*/

/*----------------------------------------------------------------*/

CAIComm_Response::CAIComm_Response(idEntity* Owner, int Type, int uniqueId) : 
	CResponse (Owner, Type, uniqueId)
{
}

/*----------------------------------------------------------------*/

void CAIComm_Response::TriggerResponse(idEntity *StimEnt, CStim* stim)
{
	idEntity* ownerEnt = m_Owner.GetEntity();

	if (ownerEnt == NULL || !ownerEnt->IsType(idAI::Type))
	{
		// Only process valid owners (must be AI type)
		return;
	}

	idAI* owner = static_cast<idAI*>(ownerEnt);

	// Can't respond if we are unconscious or dead
	if (owner->IsKnockedOut() || owner->AI_DEAD)
	{
		return;
	}

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Response for Id %s triggered (Action: %s)\r", m_StimTypeName.c_str(), m_ScriptFunction.c_str());

	// Get the communications stim
	CAIComm_Stim* p_CommStim = NULL;

	CStimResponseCollection* p_StimResponseCollection = StimEnt->GetStimResponseCollection();
	if (p_StimResponseCollection == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Entity has no stim response collection\r");
		return;
	}
	else
	{
		CStim* p_stim = p_StimResponseCollection->GetStim(ST_COMMUNICATION);
		if (p_stim == NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Source entity has no communications stim\r");
			return;
		}
		p_CommStim = static_cast<CAIComm_Stim*>(p_stim);
	}

	// Get the number of messages
//	unsigned long numMessages = p_CommStim->getNumMessages();

	// Get the script function specified in this response
	/*const function_t *pScriptFkt = owner->scriptObject.GetFunction(m_ScriptFunction.c_str());
	if (pScriptFkt == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global.\r", m_ScriptFunction.c_str());
		pScriptFkt = gameLocal.program.FindFunction(m_ScriptFunction.c_str());

		if (pScriptFkt == NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Action: %s not found in global space\r", m_ScriptFunction.c_str());
			return;
		}
	}*/

	// At this point, there is a script function in the response
	unsigned long iterationHandle;
	CAIComm_Message* p_message = NULL;
	p_message = p_CommStim->getFirstMessage(iterationHandle);

	// Call the script function for each of the messages
	while (p_message != NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Got message to respond\r");
		
		// Get the message parameters
		/*CAIComm_Message::TCommType commType = p_message->getCommunicationType();
		
		idEntity* p_issuingEntity = p_message->getIssuingEntity();
		idEntity* p_recipientEntity = p_message->getRecipientEntity();
		idEntity* p_directObjectEntity = p_message->getDirectObjectEntity();
		idVec3 directObjectLocation = p_message->getDirectObjectLocation();*/
		

		// Calculate distance of the owner of the response from the position of the message at issuance
		float maxRadiusForResponse = p_message->getMaximumRadiusInWorldCoords();
		idVec3 positionOfIssuance = p_message->getPositionOfIssuance();

		float distanceFromIssuance = maxRadiusForResponse + 1.0;
		
		idPhysics* p_phys = owner->GetPhysics();
		if (p_phys != NULL)
		{
			idVec3 ownerOrigin = p_phys->GetOrigin();
			distanceFromIssuance = (ownerOrigin - positionOfIssuance).Length();
		}

		// Only respond if response owner is within maximum radius of issuance position
		if (distanceFromIssuance <= maxRadiusForResponse)
		{
			// Pass the AIComm_Message object to the AI's Mind
			owner->GetMind()->GetState()->OnAICommMessage(p_message);

			/*idThread *pThread = new idThread(pScriptFkt);
			int n = pThread->GetThreadNum();

			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Running AIComm_StimResponse ResponseScript %s, threadID = %d, commType = %d \r", m_ScriptFunction.c_str(), n, commType);

			pThread->CallFunctionArgs
			(
				pScriptFkt, 
				true, 
				"eeffeeev", 
				owner, 
				StimEnt, 
				(float) n,
				(float) commType,
				p_issuingEntity,
				p_recipientEntity,
				p_directObjectEntity,
				&directObjectLocation
			);

			// Run the response script
			pThread->DelayedStart(0);

			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Done running AIComm_StimResponse ResponseScript %s\r", m_ScriptFunction.c_str());*/

		} // Responder was within maximum radius of message

		// Get next message
		p_message = p_CommStim->getNextMessage(iterationHandle);

	} // Loop until no more messages in Stim
}

/*
**************************************************************************
* CAIComm_Stim
**************************************************************************
*/

CAIComm_Stim::CAIComm_Stim (idEntity* Owner, int Type, int uniqueId) : 
	CStim (Owner, Type, uniqueId)
{
	messageCount = 0;
	p_firstMessage = NULL;
	p_lastMessage = NULL;

	// Set radius
	m_Radius = MAX_COMMUNICATION_RADIUS;

}

/*-------------------------------------------------------------------------*/

CAIComm_Stim::~CAIComm_Stim(void)
{
	clearMessages();
}

void CAIComm_Stim::Save(idSaveGame *savefile) const
{
	CStim::Save(savefile);

	savefile->WriteFloat(static_cast<float>(messageCount));
	
	TAICommMessageNode* cur = p_firstMessage;
	
	for (unsigned long i = 0; i < messageCount; i++)
	{
		if (cur == NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("MessageCount/list count discrepancy while saving!\r");
			break;
		}
		cur->p_message->Save(savefile);

		cur = cur->p_next;
	}

	// Check if any more nodes are available
	if (cur != NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("MessageCount/list count discrepancy after saving!\r");
	}
}

void CAIComm_Stim::Restore(idRestoreGame *savefile)
{
	CStim::Restore(savefile);

	float tempFloat;
	savefile->ReadFloat(tempFloat);
	messageCount = static_cast<unsigned long>(tempFloat);

	TAICommMessageNode* prev = NULL;
	p_firstMessage = NULL;

	// greebo: Restore the linked list, one by one
	for (unsigned long i = 0; i < messageCount; i++)
	{
		// Allocate a new node
		TAICommMessageNode* cur = new TAICommMessageNode;

		// See if the first message node is still NULL
		if (p_firstMessage == NULL)
		{
			// Set the first node to this one
			p_firstMessage = cur;
		}

		// Link back to the previous node
		cur->p_prev = prev;

		// Next is still NULL, will be set in the next loop
		cur->p_next = NULL;

		// Link the previous node to the current one
		if (prev != NULL)
		{
			prev->p_next = cur;
		}

		// Allocate an empty message class
		cur->p_message = new CAIComm_Message(
			static_cast<CAIComm_Message::TCommType>(0),
			0.0f,
			NULL,
			NULL,
			NULL,
			idVec3(0,0,0)
		);
		// Tell the message to load its values from the savefile
		cur->p_message->Restore(savefile);

		// Set the previous pointer for the next loop
		prev = cur;
	}

	// "prev" is pointing to the last allocated node
	p_lastMessage = prev;
}

/*-------------------------------------------------------------------------*/

void CAIComm_Stim::clearMessages()
{
	TAICommMessageNode* p_temp;
	TAICommMessageNode* p_cursor = p_firstMessage;

	// Ride the lightning
	while (p_cursor != NULL)
	{
		p_temp = p_cursor->p_next;

		if (p_cursor->p_message != NULL)
		{
			delete (p_cursor->p_message);
		}

		delete p_cursor;
		p_cursor = p_temp;
	}

	p_firstMessage = 0;
	p_lastMessage = 0;
	messageCount = 0;
}

/*-------------------------------------------------------------------------*/

unsigned long CAIComm_Stim::getNumMessages()
{
	return messageCount;
}

/*-------------------------------------------------------------------------*/

bool CAIComm_Stim::addMessage
(
	CAIComm_Message::TCommType in_commType,
	float in_maximumRadiusOfResponse,
	idEntity* in_p_issuingEntity,
	idEntity* in_p_recipientEntity,
	idEntity* in_p_directObjectEntity,
	const idVec3& in_directObjectLocation
)
{
	// Create new node
	TAICommMessageNode* p_node = new TAICommMessageNode;
	if (p_node == NULL)
	{
		// Out of memory
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Out of Memory! Failed to allocate communication message node\r");
		return false;
	}

	// Initialize the communication message
	p_node->p_message = new CAIComm_Message (in_commType, in_maximumRadiusOfResponse, in_p_issuingEntity, in_p_recipientEntity, in_p_directObjectEntity, in_directObjectLocation);
	if (p_node->p_message == NULL)
	{
		// Out of memory
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Out of Memory! Failed to allocate communication message\r");
		return false;
	}

	// Link node to front of list	
	p_node->p_next = p_firstMessage;
	p_node->p_prev = NULL;

	// Link list after node
	if (p_firstMessage == NULL)
	{
		// Was empty list
		p_firstMessage = p_node;
		p_lastMessage = p_node;

		// If we were disabled, we are now enabled
		if (m_State == SS_DISABLED)
		{
			m_State = SS_ENABLED;
		}
	}
	else
	{
		// Wasn't an empty list
		p_firstMessage->p_prev = p_node;
		p_firstMessage = p_node;
	}

	// One more message
	messageCount ++;

	// Success
	return true;

}

/*-------------------------------------------------------------------------*/

CAIComm_Message* CAIComm_Stim::getFirstMessage (unsigned long& out_iterationHandle)
{
	// Init iteration cursor
	out_iterationHandle = (unsigned long) p_firstMessage;

	// Return message
	if (p_firstMessage != NULL)
	{
		return p_firstMessage->p_message;
	}
	else
	{
		return NULL;
	}
}

/*-------------------------------------------------------------------------*/

CAIComm_Message* CAIComm_Stim::getNextMessage (unsigned long& inout_iterationHandle)
{
	// Advance iteration cursor
	TAICommMessageNode* p_cursor = (TAICommMessageNode*) inout_iterationHandle;
	if (p_cursor != NULL)
	{
		p_cursor = p_cursor->p_next;
	}
	inout_iterationHandle = (unsigned long) p_cursor;

	// Return message
	if (p_cursor != NULL)
	{
		return p_cursor->p_message;
	}
	else
	{
		return NULL;
	}
}

/*-------------------------------------------------------------------------*/

void CAIComm_Stim::PostFired(int numResponses)
{
	// Clear all the messages
	clearMessages();

	// We are no longer active until a message is added
	m_State = SS_DISABLED;
}
