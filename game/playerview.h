/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

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

	// temp for view testing
	void				EnableBFGVision( bool b ) { bfgVision = b; };

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

	bool				bfgVision;			// 

	// HDR related - J.C.Denton : Start

	class dnImageWrapper
	{
	private:	
		// Changed const idStr to idStr, so that compiler can provide a default assignment operator. 
		// E.g. copying contents of idPlayerView object to another would be impossible otherwise.
		idStr m_strImage;
		const idMaterial *m_matImage;

	public:
		dnImageWrapper( const char *a_strImage ) : 
		m_strImage			( a_strImage ),
		m_matImage			( declManager->FindMaterial(a_strImage) )
		{
		}
		operator const char * () const
		{
			return m_strImage.c_str();
		}
		operator const idMaterial *() const
		{
			return m_matImage;
		}
	};

	class dnPostProcessManager
	{
	private:
		int					m_iScreenHeight;
		int					m_iScreenWidth;
		float				m_fShiftScale_x;
		float				m_fShiftScale_y;

	dnImageWrapper m_imageCurrentRender;
	dnImageWrapper m_imageCurrentRender8x8DownScaled;
	dnImageWrapper m_imageLuminance64x64;
	dnImageWrapper m_imageluminance4x4;
	dnImageWrapper m_imageAdaptedLuminance1x1;
	dnImageWrapper m_imageBloom;
	dnImageWrapper m_imageHalo;
	
		// Every channel of this image will have a cooked mathematical data. 
		// Since we might need more of these textures, I am numbering them.
		dnImageWrapper		m_imageCookedMath0;
		const idMaterial*	m_matCookMath0;

	const idMaterial *m_matAvgLuminance64x;
	const idMaterial *m_matAvgLumSample4x4;
	const idMaterial *m_matAdaptLuminance;
	const idMaterial *m_matBrightPass;
	const idMaterial *m_matGaussBlurX;
	const idMaterial *m_matGaussBlurY;
	const idMaterial *m_matHalo;
	const idMaterial *m_matGaussBlurXHalo;
	const idMaterial *m_matGaussBlurYHalo;
	const idMaterial *m_matFinalScenePass;

	// For debug renders
	const idMaterial *m_matDecodedLumTexture64x64;
	const idMaterial *m_matDecodedLumTexture4x4;
	const idMaterial *m_matDecodedAdaptLuminance;

	public:
		dnPostProcessManager();
		// 		~dnPostProcessManager();

		// Methods
		void Initialize	();						// This method should be invoked when idPlayerView::Restore is called.
		void Update		();						// Called Every Frame. 

	private:
		// Following methods should not be called by any other object, but itself.
		void UpdateBackBufferParameters	();		
		void RenderDebugTextures		();		
	};

	dnPostProcessManager m_postProcessManager;
	// HDR related - J.C.Denton : End

	const idMaterial *	tunnelMaterial;		// health tunnel vision
	const idMaterial *	armorMaterial;		// armor damage view effect
	const idMaterial *	berserkMaterial;	// berserk effect
	const idMaterial *	irGogglesMaterial;	// ir effect
	const idMaterial *	bloodSprayMaterial; // blood spray
	const idMaterial *	bfgMaterial;		// when targeted with BFG
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

	// TDM Related
	bool				cur_amb_method;		// Current ambient method. Used for checking whether ambient method grpahics option has changed. By Dram
};

#endif /* !__GAME_PLAYERVIEW_H__ */
