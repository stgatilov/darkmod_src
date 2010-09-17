// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4069 $
 * $Date: 2010-07-18 13:07:48 +0200 (Sun, 18 Jul 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_LIGHTCONTROLLER_H__
#define __DARKMOD_LIGHTCONTROLLER_H__

/*
===============================================================================

  Light Controller -  control local ambient lights depending on which lights
  are in the same area/close to them.

  This class is a singleton and initiated/destroyed from gameLocal.

===============================================================================
*/

// Defines data for each ambient light the light controller controls:
typedef struct {
	idVec3				origin;
	idVec3				color;			// current color of the light
	idVec3				target_color;	// the target coloer
} light_controller_ambient_t;

// Defines data for each light the light controller uses to control the ambients:
typedef struct {
	idVec3				origin;
	idVec3				color;			// current color of the light
	float				radius;			// average of the radius
} light_controller_light_t;


class CLightController {
public:
	//CLASS_PROTOTYPE( CModelGenerator );

						CLightController( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	* Called by gameLocal.
	*/
	void				Init ( void );
	void				Shutdown ( void );
	void				Clear ( void );

	/**
	* Register a light with the controller.
	*/
	void				RegisterLight ( void );

	/**
	* De-register a light with the controller.
	*/
	void				UnregisterLight ( void );

	/**
	* Register a local ambient light with the controller.
	*/
	void				RegisterAmbient ( void );

	/**
	* Unregister a local ambient light with the controller.
	*/
	void				UnregisterAmbient ( void );

	/**
	* Update the local ambient lights because the light has changed.
	*/
	void				LightChanged( const int entityNum );

private:

	idList< light_controller_ambient_t >	m_Ambients;
	idList< light_controller_light_t >		m_Lights;

	bool				m_bActive;
};

#endif /* !__DARKMOD_LIGHTCONTROLLER_H__ */

