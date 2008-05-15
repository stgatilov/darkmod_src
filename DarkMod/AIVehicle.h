/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2007 The Dark Mod Authors
//

#ifndef AIVEHICLE_H
#define AIVEHICLE_H

/**
 * AIVehicle is a derived class of idAI meant for AI that can be ridden around
 * by players as a vehicle, but can also act independently.
 * Players must be on the same team and frob them to start riding
 */
class CAIVehicle : public idAI {
public:
	CLASS_PROTOTYPE( CAIVehicle );

							CAIVehicle( void );
							~CAIVehicle( void );
	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Think( void );

	/**
	* Updates the facing angle
	**/
	void					UpdateSteering( void );

	/**
	* Updates requested speed.
	* For now, returns true if the player is pressing forward and movement is requested
	* Otherwise, returns false and only turning is requested
	**/
	bool					UpdateSpeed( void );

	/**
	* Executed when frobbed by the player: toggle mount/dismount
	**/
	void					PlayerFrob(idPlayer *player);

	/**
	* Overloads default frob action
	**/
	virtual void			FrobAction(bool bMaster, bool bPeer = false);

	idPlayer *				GetRider( void ) { return m_Rider.GetEntity(); };
	void					SetRider( idPlayer *player ) { m_Rider = player; };

protected:
	idEntityPtr<idPlayer>	m_Rider;
	/**
	* Joint to which the player is attached
	**/
	jointHandle_t			m_RideJoint;
	idVec3					m_RideOffset;
	idAngles				m_RideAngles;

	/**
	* Current world yaw we are pointing along [deg?]
	**/
	float					m_CurAngle;
	/**
	* Requested speed, as a fraction of max speed
	**/
	float					m_SpeedFrac;
	/**
	* Speed at which the pointing angle can be changed (should be speed dependent)
	**/
	float					m_SteerSpeed;
	/**
	* Assuming a constant acceleration, how many seconds does it take to get to max speed?
	**/
	float					m_SpeedTimeToMax;
	/**
	* Max reverse speed as a fraction of max forward speed
	**/
	float					m_MaxReverseSpeed;
};


#endif /* !AIVEHICLE_H */
