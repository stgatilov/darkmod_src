/******************************************************************************/
/*                                                                            */
/*         CommunicationStim									              */
/*         Copyright 2006 Damon Hill (sophisticatedZombie)  2006              */
/*																			  */
/******************************************************************************/


/*
*--------------------------------------------------------------------------
* Header wrapper
*--------------------------------------------------------------------------
*/
#ifndef COMMUNICATION_MESSAGE_HEADER
#define COMMUNICATION_MESSAGE_HEADER


/*
*--------------------------------------------------------------------------
* Required includes
*--------------------------------------------------------------------------
*/

#include "StimResponse.h"


/*
*--------------------------------------------------------------------------
* Classes
*--------------------------------------------------------------------------
*/

/*!
* This class defines a type of CStim that carries information
* between artificial intelligences.
*
*/
class CAIComm_Message
{
public:

	/*!
	* This enumeration defines the meaning of the communication.
	* Each enumeration value indicates 
	*
	*/
	typedef enum
	{
		/*!
		* A friendly greeting, such as "hello"
		*
		* recipientEntity: The entity being greeted
		*
		*/
		Greeting_CommType = 0x01,

		/*!
		* A friendly joke not meant to insult anyone.
		*
		* recipientEntity: The entity meant to hear it
		*
		* directObjectEntity; The butt of the joke
		*/
		FriendlyJoke_CommType,

		/*!
		* An insult directed at some entity.
		* "<X>'s mother was a hamster, and his father smelt of elderberries!"
		*
		* recipientEntity: The entity meant to hear it
		*
		* directObjectEntity: The entity that is the subject of the insult
		*/
		Insult_CommType,

		/*!
		* A request for help of any kind at issuing entity.
		*
		* directObjectEntity: The entity that caused the need for help
		*
		* directObjectLocation: The location of the enemy when the
		*	request was issued
		*	
		*/
		RequestForHelp_CommType,

		/*!
		* A request for long range combat help at issuing entity.
		*
		* directObjectEntity: The entity that caused the need for help
		*
		* directObjectLocation: The location of the enemy when the
		*	request was issued
		* 
		*/
		RequestForMissileHelp_CommType,

		/*!
		* A request for close range combat help at issuing entity.
		*
		* directObjectEntity: The entity that caused the need for help
		*
		* directObjectLocation: The location of the enemy when the
		*	request was issued
		*/
		RequestForMeleeHelp_CommType,

		/*!
		* A request for someone to bring a light to a given location.
		*
		* directObjectLocation: The location where the light
		*	is needed
		*
		*/
		RequestForLight_CommType,

		/*!
		* A general declaration that something suspicious has been 
		* detected.
		*
		* directObjectEntity: If not NULL, the entity that is suspicious
		*
		* directObjectLocation: The location of the detected suspicious
		*	stimulus
		*  
		*/
		DetectedSomethingSuspicious_CommType,

		/*!
		* A general declaration that an enemy has been detected.
		*
		* directObjectEntity: The enemy that was detected
		*
		* directObjectLocation: The location of the enemy at the time
		*	of detection
		*
		*/
		DetectedEnemy_CommType,

		/*!
		* An order to follow the issuer.
		*
		*/
		FollowOrder_CommType,	

		/*!
		* An order to guard a location.
		* 
		* directObjectLocation: The location to be guarded
		*
		*/
		GuardLocationOrder_CommType,

		/*!
		* An order to guard an entity
		*
		* directObjectEntity: The entity to be guarded
		*/
		GuardEntityOrder_CommType,

		/*!
		* An order to perform a certain patrol
		*
		* ???
		*/
		PatrolOrder_CommType,

		/*!
		* An order to search near a certain location
		*
		* directObjectLocation: The point near which the
		* search should be conducted.
		*
		*/
		SearchOrder_CommType,

		/*
		* An order to attack a certain entity
		*
		* directObjectEntity: The entity to be attacked
		*/
		AttackOrder_CommType

	} TCommType;


	/*!
	* Retrieves the type of communication
	*/
	TCommType getCommunicationType()
	{
		return m_commType;
	};

	/*!
	* Retrieves a pointer to the issuing entity
	*/
	idEntity* getIssuingEntity()
	{
		return m_p_issuingEntity;
	};

	/*!
	* Retrieves a pointer to the intended recipient
	* entity. If NULL, then there is no specific
	* recipient intended.
	*/
	idEntity* getRecipientEntity()
	{
		return m_p_recipientEntity;
	};

	/*!
	* Retrieves a pointer to the entity to which
	* the communication is referring.
	*/
	idEntity* getDirectObjectEntity()
	{
		return m_p_directObjectEntity;
	};

	/*!
	* Retrieves the location to which the 
	* communicatoin is referring
	*/
	idVec3 getDirectObjectLocation()
	{
		return m_directObjectLocation;
	};

	idVec3 getPositionOfIssuance()
	{
		return m_positionOfIssuance;
	}

	float getMaximumRadiusInWorldCoords()
	{
		return m_maximumRadiusInWorldCoords;
	}

public:

	/*!
	* public Constructor
	*/
	CAIComm_Message
	(
		TCommType in_commType,
		float in_maximumRadiusInWorldCoords,
		idEntity* in_p_issuingEntity,
		idEntity* in_p_recipientEntity,
		idEntity* in_p_directObjectEntity,
		const idVec3& in_directObjectLocation
	);

	/*!
	* public destructor
	*/
	~CAIComm_Message();


protected:

	/*!
	* This field indicates the type of communication. The value of this type defines
	* the meaning of the other member variables. (See typedef of TCommType for specs)
	*/
	TCommType m_commType;

	/*!
	* This field indicates the entity which issued the communication
	*/
	idEntity* m_p_issuingEntity;

	/*!
	* This field indicates the entity which is the intended recipient of the
	* communication.  A value of NULL indicates the communication has no
	* specific recipient in mind.
	*/
	idEntity* m_p_recipientEntity;

	/*!
	* This field indicates the entity which is the entity mentioned in the
	* communication.
	*
	*/
	idEntity* m_p_directObjectEntity;

	/*!
	* This field indicates the location which is the location mentioned in
	* the communication.
	*
	*/
	idVec3 m_directObjectLocation;

	/*!
	* This is the position of the issuing entity at the time of issuance
	*/
	idVec3 m_positionOfIssuance;

	/*!
	* This field indicates the maximum radius of the message for responders
	*/
	float m_maximumRadiusInWorldCoords;



};




/*
*--------------------------------------------------------------------------
* End header wrapper
*--------------------------------------------------------------------------
*/
#endif
