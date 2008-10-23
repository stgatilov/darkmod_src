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
 * Update: Players can now control them without being bound to them,
 * e.g., controlling them from a horse-drawn coach
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
	* Returns the player that is controlling this AI's movement
	**/
	idPlayer *				GetController( void ) { return m_Controller.GetEntity(); };
	/**
	* Starts reading control input from the player and stops thinking independently
	* If player argument is NULL, returns control back to AI mind
	* This does not handle immobilizing the player, that is done elsewhere.
	**/
	void					SetController( idPlayer *player );

	// Script events
	void					Event_SetController( idPlayer *player );
	/**
	* This needs to be a separate script event since scripting didn't like
	* passing in $null_entity for some reason.
	**/
	void					Event_ClearController( void );
	void					Event_FrobRidable(idPlayer *player);

protected:
	idEntityPtr<idPlayer>	m_Controller;
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
