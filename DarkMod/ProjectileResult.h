/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2006/01/20 08:47:45  ishtvan
 * initial version
 *
 *
 * Revision 1.1  2006/01/06 20:19:12  ishtvan
 * Initial Release
 *
 *
 ***************************************************************************/

// Copyright (C) 2006 Chris Sarantos <csarantos@gmail.com>

#ifndef PROJECTILE_RESULT_H
#define PROJECTILE_RESULT_H

/**
*	CProjectileResult is a dummy object that may be spawned when a projectile hits
* a surface.  This object handles things like determining whether the projectile
* sticks in to the surface and may be retrieved, or if it breaks.  This object
* is also used to store any stim/response information on the projectile (e.g., 
* water arrow puts out torches when it detonates), and run other scripts if desired.
*
* NOTE: This object MUST be destroyed/removed in the scripts that are run
**/


class CProjectileResult : public idEntity {
public:
	CLASS_PROTOTYPE( CProjectileResult );

	CProjectileResult( void );

/**
* Initialize the projectile result, called by the projectile
**/
	void Init
		(
			SFinalProjData *pData, const trace_t &collision,
			idProjectile *pProj, bool bActivate
		);

protected:
	/**
	* Chooses whether to run the "active" or "dud" script, based on the material
	* type hit and the activating material types.
	**/
	void RunResultScript( void );

protected:
	/**
	* Collision data from the impacted projectile
	**/
	trace_t			m_Collision;

	/**
	* Other data from the impacted projectile
	**/
	SFinalProjData	m_ProjData;

	/**
	* True => projectile activated, false => projectile is a dud
	**/
	bool			m_bActivated;

	/**
	* Getter events for scripting
	**/
	void Event_GetFinalVel( void );

	void Event_GetFinalAngVel( void );

	void Event_GetAxialDir( void );

	void Event_GetProjMass( void );

	void Event_GetSurfType( void );

	void Event_GetSurfNormal( void );

	void Event_GetStruckEnt( void );

	void Event_GetIncidenceAngle( void );

};

#endif