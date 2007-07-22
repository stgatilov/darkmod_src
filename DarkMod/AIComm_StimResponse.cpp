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

#define MAX_COMMUNICATION_RADIUS 5000.0

/*
**************************************************************************
* CAIComm_Response
**************************************************************************
*/

/*----------------------------------------------------------------*/

CAIComm_Response::CAIComm_Response(idEntity* Owner, int Type) : CResponse (Owner, Type)
{
}

/*----------------------------------------------------------------*/

CAIComm_Response::~CAIComm_Response(void)
{
}

/*----------------------------------------------------------------*/

void CAIComm_Response::TriggerResponse(idEntity *StimEnt, CStim* stim)
{
	// Can't respond if we are unconscious or dead
	if (m_Owner.GetEntity() != NULL)
	{
		// If we are descended from AI ...
		if (m_Owner.GetEntity()->IsType(idAI::Type))
		{
			// check if dead or knocked out
			idAI* p_AIOwner = static_cast< idAI * >( m_Owner.GetEntity() );
			if (p_AIOwner->IsKnockedOut() )
			{
				return;
			}
		}
	
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
		CStim* p_stim = p_StimResponseCollection->GetStim (ST_COMMUNICATION);
		if (p_stim == NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Source entity has no communications stim\r");
			return;
		}
		p_CommStim = (CAIComm_Stim*) p_stim;
	}

	// Get the number of messages
//	unsigned long numMessages = p_CommStim->getNumMessages();

	// Get the script function specified in this response
	const function_t *pScriptFkt = m_Owner.GetEntity()->scriptObject.GetFunction(m_ScriptFunction.c_str());
	if(pScriptFkt == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global.\r", m_ScriptFunction.c_str());
		pScriptFkt = gameLocal.program.FindFunction(m_ScriptFunction.c_str());
		if (pScriptFkt == NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Action: %s not found in global space\r", m_ScriptFunction.c_str());
			return;

		}
	}

	// If there is a script function in the response
	if(pScriptFkt)
	{
		unsigned long iterationHandle;
		CAIComm_Message* p_message = NULL;
		p_message = p_CommStim->getFirstMessage (iterationHandle);

		// Call the script function for each of the messages
		while (p_message != NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Got message to respond\r");
			
			// Get the message parameters
			CAIComm_Message::TCommType commType = p_message->getCommunicationType();
			float maxRadiusForResponse = p_message->getMaximumRadiusInWorldCoords();
			idEntity* p_issuingEntity = p_message->getIssuingEntity();
			idEntity* p_recipientEntity = p_message->getRecipientEntity();
			idEntity* p_directObjectEntity = p_message->getDirectObjectEntity();
			idVec3 directObjectLocation = p_message->getDirectObjectLocation();
			idVec3 positionOfIssuance = p_message->getPositionOfIssuance();

			// Calculate distance of the owner of the response from the position of the message at issuance
			float distanceFromIssuance = maxRadiusForResponse + 1.0;
			
			idPhysics* p_phys = m_Owner.GetEntity()->GetPhysics();
			if (p_phys != NULL)
			{
				idVec3 ownerOrigin = p_phys->GetOrigin();
				distanceFromIssuance = (ownerOrigin - positionOfIssuance).Length();
			}

			// Only respond if response owner is within maximum radius of issuance position
			if (distanceFromIssuance <= maxRadiusForResponse)
			{
				idThread *pThread = new idThread(pScriptFkt);
				int n = pThread->GetThreadNum();

				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Running AIComm_StimResponse ResponseScript %s, threadID = %d, commType = %d \r", m_ScriptFunction.c_str(), n, commType);

				pThread->CallFunctionArgs
				(
					pScriptFkt, 
					true, 
					"eeffeeev", 
					m_Owner.GetEntity(), 
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

				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Done running AIComm_StimResponse ResponseScript %s\r", m_ScriptFunction.c_str());

			} // Responder was within maximum radius of message

			// Get next message
			p_message = p_CommStim->getNextMessage (iterationHandle);

		} // Loop until no more messages in Stim
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("ResponseActionScript not found! [%s]\r", m_ScriptFunction.c_str());
	}

	// Continue the chain if we have a followup response to be triggered.
	if(m_FollowUp != NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Followup: %08lX\r", m_FollowUp);
		m_FollowUp->TriggerResponse(StimEnt, stim);
	}
}

/*
**************************************************************************
* CAIComm_Stim
**************************************************************************
*/

CAIComm_Stim::CAIComm_Stim (idEntity* Owner, int Type) : CStim (Owner, Type)
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

void CAIComm_Stim::PostFired (int numResponses)
{
	// Clear all the messages
	clearMessages();

	// We are no longer active until a message is added
	m_State = SS_DISABLED;

}
