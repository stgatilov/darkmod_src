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

#ifndef __GAME_PLAYERVIEW_H__
#define __GAME_PLAYERVIEW_H__





/*
===============================================================================

  Player view.

===============================================================================
*/

// screenBlob_t are for the on-screen damage claw marks, etc
typedef struct {
	const idMaterial *	material;
	float				x, y, w, h;
	float				s1, t1, s2, t2;
	int					finishTime;
	int					startFadeTime;
	float				driftAmount;
} screenBlob_t;

#define	MAX_SCREEN_BLOBS	8

class idPlayerView {
public:
						idPlayerView();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				SetPlayerEntity( class idPlayer *playerEnt );

	void				ClearEffects( void );

	void				DamageImpulse( idVec3 localKickDir, const idDict *damageDef );

	void				WeaponFireFeedback( const idDict *weaponDef );

	idAngles			AngleOffset( void ) const;			// returns the current kick angle

	idMat3				ShakeAxis( void ) const;			// returns the current shake angle

	void				CalculateShake( void );

	// this may involve rendering to a texture and displaying
	// that with a warp model or in double vision mode
	void				RenderPlayerView( idUserInterface *hud );

	void				Fade( idVec4 color, int time );

	void				Flash( idVec4 color, int time );

	void				AddBloodSpray( float duration );

	// Events invoked by the engine on reloadImages or vid_restart
	void				OnReloadImages();
	void				OnVidRestart();

private:
	void				SingleView( idUserInterface *hud, const renderView_t *view, bool drawHUD = true);
	void				DoubleVision( idUserInterface *hud, const renderView_t *view, int offset );
	void				BerserkVision( idUserInterface *hud, const renderView_t *view );
	void				InfluenceVision( idUserInterface *hud, const renderView_t *view );
	void				ScreenFade();

	// Updates the ambient light settings
	void				UpdateAmbientLight();

	screenBlob_t *		GetScreenBlob();

	screenBlob_t		screenBlobs[MAX_SCREEN_BLOBS];

	int					dvFinishTime;		// double vision will be stopped at this time
	const idMaterial *	dvMaterial;			// material to take the double vision screen shot

	int					kickFinishTime;		// view kick will be stopped at this time
	idAngles			kickAngles;			

	class dnImageWrapper
	{
	private:	
		// Changed const idStr to idStr, so that compiler can provide a default implementation for the assignment operator. 
		// E.g. copying contents of idPlayerView object to another would be impossible otherwise.
		idStr m_strImage;
		const idMaterial *m_matImage;

	public:
		dnImageWrapper( const char *a_strImage ) : 
		m_strImage			( a_strImage ),
		m_matImage			( declManager->FindMaterial(a_strImage) )
		{
		}
		ID_INLINE operator const char * () const
		{
			return m_strImage.c_str();
		}
		ID_INLINE operator const idMaterial *() const
		{
			return m_matImage;
		}
	};

	class dnPostProcessManager
	{
	private:
		int					m_iScreenHeight;
		int					m_iScreenWidth;
		int					m_iScreenHeightPowOf2;
		int					m_iScreenWidthPowOf2;
		float				m_fShiftScale_x;
		float				m_fShiftScale_y;

		int					m_nFramesToUpdateCookedData; // After these number of frames Cooked data will be updated. 0 means no update.

		bool				m_bForceUpdateOnCookedData;

		dnImageWrapper m_imageCurrentRender;
		dnImageWrapper m_imageBloom;
		
		// Every channel of this image will have a cooked mathematical data. 
		dnImageWrapper		m_imageCookedMath;
		const idMaterial*	m_matCookMath_pass1;
		const idMaterial*	m_matCookMath_pass2;

		const idMaterial *m_matBrightPass;
		const idMaterial *m_matGaussBlurX;
		const idMaterial *m_matGaussBlurY;

		const idMaterial *m_matFinalScenePass;

		int					m_ImageAnisotropyHandle;

	public:
		dnPostProcessManager();
		~dnPostProcessManager();

		// Methods
		void Initialize	();						// This method should be invoked when idPlayerView::Restore is called.
		void Update		();						// Called Every Frame. 

		// Lets the cooked data update the next frame (if activated)
		void ScheduleCookedDataUpdate	();

	private:
		// Following methods should not be called by any other object, but itself.
		void UpdateBackBufferParameters	();		
		void RenderDebugTextures		();		
		void UpdateCookedData			();
		void UpdateInteractionShader	(); 	// Chooses between the various VFP files according to the CVAR settings. Only call this if settings got changed.
		void Hook_BufferCommandText( cmdExecution_t a_eType, const char *a_pcText );	// Source Hook for idCmdSystem::BufferCommandText - JC.

		void OnImageAnisotropyChanged	();	// gets called when the image_Anisotropy CVAR changes
	};

	dnPostProcessManager m_postProcessManager;

	const idMaterial *	tunnelMaterial;		// health tunnel vision
	const idMaterial *	bloodSprayMaterial; // blood spray
	const idMaterial *	lagoMaterial;		// lagometer drawing
	float				lastDamageTime;		// accentuate the tunnel effect for a while

	idVec4				fadeColor;			// fade color
	idVec4				fadeToColor;		// color to fade to
	idVec4				fadeFromColor;		// color to fade from
	float				fadeRate;			// fade rate
	int					fadeTime;			// fade time

	idAngles			shakeAng;			// from the sound sources

	idPlayer *			player;
	renderView_t		view;
	idVec2 shiftScale;

// sikk---> PostProcess Effects
	void				DoPostFX( void );
	void				PostFX_SoftShadows( void );
	void				PostFX_EdgeAA( void );
	void				PostFX_HDR( void );
	void				PostFX_Bloom( void );
	void				PostFX_MotionBlur( void );
	void				PostFX_DoF( void );
	void				PostFX_SSIL( void );
	void				PostFX_SSAO( void );
	void				PostFX_SunShafts( void );
	void				PostFX_LensFlare( void );
	void				PostFX_ColorGrading( void );
	void				PostFX_ExplosionFX( void );
	void				PostFX_IRGoggles( void );
	void				PostFX_ScreenFrost( void );
	void				PostFX_CelShading( void );
	void				PostFX_Filmgrain( void );
	void				PostFX_Vignetting( void );

	void				RenderDepth( bool bCrop );
	void				RenderNormals( bool bFace );
	void				ToggleShadows( bool noShadows );
	void				ResetShadows( void );
	bool				DoFConditionCheck( void );
	bool				MBConditionCheck( void );


	const idMaterial *	blackMaterial;			// Black material (for general use) 
	const idMaterial *	whiteMaterial;			// White material (for general use) 
	const idMaterial *	currentRenderMaterial;	// Current Render material (for general use) 
	const idMaterial *	scratchMaterial;		// Scratch material (for general use) 
	const idMaterial *	depthMaterial;			// Depth material (for general use) 
	const idMaterial *	normalsMaterial;		// View Space Normals material (for general use) 
	const idMaterial *	softShadowsMaterial;	// Soft Shadow material
	const idMaterial *	edgeAAMaterial;			// Edge AA material
	const idMaterial *	hdrLumBaseMaterial;		// HDR Luminance Base material
	const idMaterial *	hdrLumAverageMaterial;	// HDR Luminance Average material
	const idMaterial *	hdrLumAdaptedMaterial;	// HDR Luminance Adapted material
	const idMaterial *	hdrBrightPass1Material;	// HDR Bright Pass Filter material (Reinhard RGB)
	const idMaterial *	hdrBrightPass2Material;	// HDR Bright Pass Filter material (Reinhard Yxy)
	const idMaterial *	hdrBrightPass3Material;	// HDR Bright Pass Filter material (Exp)
	const idMaterial *	hdrBrightPass4Material;	// HDR Bright Pass Filter material (Filmic simple)
	const idMaterial *	hdrBrightPass5Material;	// HDR Bright Pass Filter material (Filmic complex)
	const idMaterial *	hdrBloomMaterial;		// HDR Bloom material
	const idMaterial *	hdrFlareMaterial;		// HDR Lens Flare material
	const idMaterial *	hdrGlareMaterial;		// HDR Glare material
	const idMaterial *	hdrFinalMaterial;		// HDR Final Tone Mapping material
	const idMaterial *	bloomMaterial;			// Bloom material
	const idMaterial *	ssilMaterial;			// SSIL material
	const idMaterial *	ssaoMaterial;			// SSAO material
	const idMaterial *	sunShaftsMaterial;		// Sun Shafts material
	const idMaterial *	lensFlareMaterial;		// Lens Flare material
	const idMaterial *	dofMaterial;			// DoF material
	const idMaterial *	motionBlurMaterial;		// Motion Blur material
	const idMaterial *	colorGradingMaterial;	// Color Grading material
	const idMaterial *	screenFrostMaterial;	// Screen Frost material
	const idMaterial *	celShadingMaterial;		// Cel Shading material
	const idMaterial *	filmgrainMaterial;		// Filmgrain material
	const idMaterial *	vignettingMaterial;		// Vignetting material
	const idMaterial *	tunnel2Material;		// health tunnel vision for Health Management System (Health Regen)
	const idMaterial *	adrenalineMaterial;		// Adrenaline Vision material
	const idMaterial *	explosionFXMaterial;	// Explosion FX material

	idAngles			prevViewAngles;			// Holds previous frame's player view angles
	int					prevTime;				// Holds previous frame's time
	bool				bDepthRendered;			// Holds whether the depth map has been rendered for the current frame
	renderView_t		hackedView;
	pvsHandle_t			playerPVS;				// Holds player's current pvs (for soft shadows)
	bool				bSoftShadows;			// a soft shadows toggle used so ResetShadows() is only run once when r_useSoftShadows = 0
// <---sikk
	bool				isRendering;
	int					interleaved_dist_check_period;
	bool				lastNoShadows;
};

#endif /* !__GAME_PLAYERVIEW_H__ */
