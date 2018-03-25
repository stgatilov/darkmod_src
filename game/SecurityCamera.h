/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

#ifndef __GAME_SECURITYCAMERA_H__
#define __GAME_SECURITYCAMERA_H__

/*
===================================================================================

	Security camera

===================================================================================
*/

// grayman #4615 - Refactored for 2.06

class idSecurityCamera : public idEntity {
public:
	CLASS_PROTOTYPE( idSecurityCamera );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	virtual renderView_t *	GetRenderView();
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			Present( void );


private:

	enum
	{
		MODE_SCANNING,
		MODE_LOSINGINTEREST,
		MODE_SIGHTED,
		MODE_ALERT
	};

	enum
	{
		STATE_SWEEPING,
		STATE_PLAYERSIGHTED,
		STATE_ALERTED,
		STATE_LOSTINTEREST,
		STATE_POWERRETURNS_SWEEPING,
		STATE_POWERRETURNS_PAUSED,
		STATE_PAUSED,
		STATE_DEAD
	};

	float					angle;
	float					sweepAngle;
	int						modelAxis;
	bool					flipAxis;
	float					scanDist;
	float					scanFov;
							
	int						sweepStartTime;
	int						sweepEndTime;
	int						nextSparkTime;
	int						removeSparkTime;
	bool					negativeSweep;
	bool					sweeping;
	int						alertMode;
	float					scanFovCos;

	idVec3					viewOffset;
							
	int						pvsArea;
	idPhysics_RigidBody		physicsObj;
	idTraceModel			trm;

	bool					rotate;
	bool					stationary;
	int						nextAlertTime;
	int						state;
	int						startAlertTime;
	bool					emitPauseSound;
	int						emitPauseSoundTime;
	int						pauseEndTime;
	int						endAlertTime;
	int						lostInterestEndTime;
	float					percentSwept;
	idEntityPtr<idLight>	spotLight;
	idEntityPtr<idEntity>	sparks;
	idEntityPtr<idEntity>	cameraDisplay;
	bool					powerOn;
	bool					spotlightPowerOn;

	void					StartSweep( void );
	bool					CanSeePlayer( void );
	void					SetAlertMode( int status );
	void					DrawFov( void );
	const idVec3			GetAxis( void ) const;
	float					SweepTime( void ) const;

	void					ReverseSweep( void );
	void					ContinueSweep( void );
	void					Event_AddLight( void );
	void					Event_SpotLight_Toggle( void );
	void					Event_Sweep_Toggle( void );
	// Obsttorte
	void					Event_GetSpotLight(void);

	void					PostSpawn( void );
	void					AddSparks(void);

	void					Activate( idEntity* activator );
	float					GetCalibratedLightgemValue(idPlayer* player);
	bool					IsEntityHiddenByDarkness(idPlayer* player, const float sightThreshold);

};

#endif /* !__GAME_SECURITYCAMERA_H__ */
