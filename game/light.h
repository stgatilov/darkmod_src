/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.7  2005/03/21 23:09:13  sparhawk
 * Implemented projected and ellipsoid lights
 *
 * Revision 1.6  2005/01/24 00:17:16  sparhawk
 * Lightgem shadow problem fixed.
 *
 * Revision 1.5  2005/01/20 19:37:49  sparhawk
 * Lightgem now calculates projected lights as well as parallel lights.
 *
 * Revision 1.4  2005/01/19 23:22:04  sparhawk
 * Bug fixed for ambient lights
 *
 * Revision 1.3  2005/01/19 23:01:48  sparhawk
 * Lightgem updated to do proper projected lights with occlusion.
 *
 * Revision 1.2  2005/01/07 02:10:35  sparhawk
 * Lightgem updates
 *
 * Revision 1.1.1.1  2004/10/30 15:52:30  sparhawk
 * Initial release
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_LIGHT_H__
#define __GAME_LIGHT_H__

/*
===============================================================================

  Generic light.

===============================================================================
*/
class CLightMaterial;

extern const idEventDef EV_Light_GetLightParm;
extern const idEventDef EV_Light_SetLightParm;
extern const idEventDef EV_Light_SetLightParms;

class idLight : public idEntity {
public:
	CLASS_PROTOTYPE( idLight );

					idLight();
					~idLight();

	void			Spawn( void );

	void			Save( idSaveGame *savefile ) const;					// archives object for save game file
	void			Restore( idRestoreGame *savefile );					// unarchives object from save game file

	virtual void	UpdateChangeableSpawnArgs( const idDict *source );
	virtual void	Think( void );
	virtual void	FreeLightDef( void );
	virtual bool	GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
	void			Present( void );

	void			SaveState( idDict *args );
	virtual void	SetColor( float red, float green, float blue );
	virtual void	SetColor( const idVec4 &color );
	virtual void	GetColor( idVec3 &out ) const;
	virtual void	GetColor( idVec4 &out ) const;
	const idVec3 &	GetBaseColor( void ) const { return baseColor; }
	void			SetShader( const char *shadername );
	void			SetLightParm( int parmnum, float value );
	void			SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			SetRadiusXYZ( float x, float y, float z );
	void			SetRadius( float radius );
	void			On( void );
	void			Off( void );
	void			Fade( const idVec4 &to, float fadeTime );
	void			FadeOut( float time );
	void			FadeIn( float time );
	void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void			BecomeBroken( idEntity *activator );
	qhandle_t		GetLightDefHandle( void ) const { return lightDefHandle; }
	void			SetLightParent( idEntity *lparent ) { lightParent = lparent; }
	void			SetLightLevel( void );

	virtual void	ShowEditingDialog( void );

	enum {
		EVENT_BECOMEBROKEN = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual void	ClientPredictionThink( void );
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool	ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	/**
	 * This will return a grayscale value dependent on the value from the light.
	 * X and Y are the coordinates returned by calculating the position from the 
	 * player related to the light. The Z coordinate can be ignored. The distance
	 * is required when the light has no textures to calculate a falloff.
	 */
	double			GetDistanceColor(double distance, double x, double y);

	/**
	 * GetTextureIndex calculates the index into the texture based on the x/y coordinates
	 * given and returns the index.
	 */
	int				GetTextureIndex(double x, double y, int TextureWidth, int TextureHeight, int BytesPerPixel);

	/**
	 * Returns true if the light is a parallel light.
	 */
	inline bool		IsParallel(void) { return renderLight.parallel; };
	inline bool		IsPointlight(void) { return renderLight.pointLight; };
	bool			CastsShadow(void);

	/**
	 * GetLightCone returns the lightdata.
	 * If the light is a pointlight it will return an ellipsoid defining the light.
	 * In case of a projected light, the returned data is a cone.
	 * If the light is a projected light and uses the additional vectors for
	 * cut off cones, it will return true.
	 */
	bool GetLightCone(idVec3 &Origin, idVec3 &Axis, idVec3 &Center);
	bool GetLightCone(idVec3 &Origin, idVec3 &Target, idVec3 &Right, idVec3 &Up, idVec3 &Start, idVec3 &End);

private:
	renderLight_t	renderLight;				// light presented to the renderer
	idVec3			localLightOrigin;			// light origin relative to the physics origin
	idMat3			localLightAxis;				// light axis relative to physics axis
	qhandle_t		lightDefHandle;				// handle to renderer light def
	idStr			brokenModel;
	int				levels;
	int				currentLevel;
	idVec3			baseColor;
	bool			breakOnTrigger;
	int				count;
	int				triggercount;
	idEntity *		lightParent;
	idVec4			fadeFrom;
	idVec4			fadeTo;
	int				fadeStart;
	int				fadeEnd;
	bool			soundWasPlaying;

private:
	void			PresentLightDefChange( void );
	void			PresentModelDefChange( void );

	void			Event_SetShader( const char *shadername );
	void			Event_GetLightParm( int parmnum );
	void			Event_SetLightParm( int parmnum, float value );
	void			Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 );
	void			Event_SetRadiusXYZ( float x, float y, float z );
	void			Event_SetRadius( float radius );
	void			Event_Hide( void );
	void			Event_Show( void );
	void			Event_On( void );
	void			Event_Off( void );
	void			Event_ToggleOnOff( idEntity *activator );
	void			Event_SetSoundHandles( void );
	void			Event_FadeOut( float time );
	void			Event_FadeIn( float time );

	/**
	 * Texturename for the falloff image
	 */
	const char		*m_MaterialName;

	/**
	 * Pointer to the material that is used for this light. This pointer
	 * is only loaded once. If the material needs to change dynamically
	 * for a light, the m_FalloffImage must be set to the new material name
	 * and m_LightMaterial must be set to NULL, to force the reload, next
	 * time the light should use a new material.
	 */
	CLightMaterial	*m_LightMaterial;

public:
	/**
	 * Each light also gets the maxlightradius, which determines which value
	 * is the maximum radius for that particular light,
	 */
	double			m_MaxLightRadius;

};

#endif /* !__GAME_LIGHT_H__ */
