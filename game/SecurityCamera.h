/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#ifndef __GAME_SECURITYCAMERA_H__
#define __GAME_SECURITYCAMERA_H__

/*
===================================================================================

	Security camera

===================================================================================
*/


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

	enum { SCANNING, LOSINGINTEREST, ALERT, ACTIVATED };

	float					angle;
	float					sweepAngle;
	int						modelAxis;
	bool					flipAxis;
	float					scanDist;
	float					scanFov;
							
	float					sweepStart;
	float					sweepEnd;
	bool					negativeSweep;
	bool					sweeping;
	int						alertMode;
	float					stopSweeping;
	float					scanFovCos;

	idVec3					viewOffset;
							
	int						pvsArea;
	idPhysics_RigidBody		physicsObj;
	idTraceModel			trm;

	void					StartSweep( void );
	bool					CanSeePlayer( void );
	void					SetAlertMode( int status );
	void					DrawFov( void );
	const idVec3			GetAxis( void ) const;
	float					SweepSpeed( void ) const;

	void					Event_ReverseSweep( void );
	void					Event_ContinueSweep( void );
	void					Event_Pause( void );
	void					Event_Alert( void );
	void					Event_AddLight( void );
};

#endif /* !__GAME_SECURITYCAMERA_H__ */
