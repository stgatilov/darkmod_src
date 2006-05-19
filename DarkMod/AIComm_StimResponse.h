/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.2  2006/05/19 19:56:50  sparhawk
 * CVSHeader added
 *
 *
 ***************************************************************************/
#pragma once

#include "stimresponse.h"
#include "AIComm_Message.h"

/**
* This class manifests an AI's reponse link to 
* a response to a AI Communications Stim
*/
class CAIComm_Response:
	public CResponse
{
public:
	CAIComm_Response(idEntity* Owner, int Type);
	virtual ~CAIComm_Response(void);

public:

	/**
	* This override of the base class version
	* calls a script function of a different format
	* than the default stim response, and passes
	* in the communication stim parameters.
	*/
	virtual void TriggerResponse(idEntity *Stim);



};

/*------------------------------------------------------------------*/

typedef struct tagTAICommMessageNode
{
	// The message
	CAIComm_Message* p_message;

	// Node links
	tagTAICommMessageNode* p_prev;
	tagTAICommMessageNode* p_next;

} TAICommMessageNode;

/*------------------------------------------------------------------*/

/**
* This class manifests an AI's outbound communications
* stim and contains their outbound communication messages.
*/
class CAIComm_Stim: 
	public CStim
{
protected:

	// The number of comm messages in the list
	unsigned long messageCount;

	// List header and trailer pointers
	TAICommMessageNode* p_firstMessage;
	TAICommMessageNode* p_lastMessage;

public:
	/*!
	* Constructor
	*/
	CAIComm_Stim (idEntity* Owner, int Type);

	/*!
	* Destructor
	*/
	virtual ~CAIComm_Stim(void);

	/*!
	* This method clears all the messages from the list and destroys
	* them.
	*/
	void clearMessages();

	/*!
	* This method adds a communication message to this stim
	*
	* @param in_commType The type of the communication message
	*
	* @param in_maximumRadiusOfResponse The maximum radius away that
	*	the message should be responded to (due to audability)
	* 
	* @param in_p_issuingEntity The entity speaking the communication
	*
	* @param in_p_recipientEntity The entity intended to hear the communication
	*
	* @param in_p_directObjectEntity The entity that the communication is about
	*
	* @param in_directObjectLocation The location that the communication is about
	*
	*/
	bool addMessage
	(
		CAIComm_Message::TCommType in_commType,
		float in_maximumRadiusOfResponse,
		idEntity* in_p_issuingEntity,
		idEntity* in_p_recipientEntity,
		idEntity* in_p_directObjectEntity,
		const idVec3& in_directObjectLocation
	);

	/**
	* Get the number of messages in the stim
	*/
	unsigned long getNumMessages();

	/**
	* Gets the first message in the stim
	*
	* @return pointer to the message
	* @return NULL if no messages
	*/
	CAIComm_Message* getFirstMessage (unsigned long& out_iterationHandle);

	/**
	* Gets the next message in the stim.
	*
	* @return pointer to the message
	* @return NULL if no more messages
	*/
	CAIComm_Message* getNextMessage (unsigned long& inout_iterationHandle);

	/**
	* BASE CLASS VIRTUAL OVERRIDE
	*
	* This version removes any messages in the list of messages as they
	* have been distributed.
	*/
	virtual void PostFired (int numResponses);


};

