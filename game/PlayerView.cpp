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

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "Game_local.h"

#include <boost/bind.hpp>

static int MakePowerOfTwo( int num ) {
	int		pot;

	for (pot = 1 ; pot < num ; pot<<=1) {}

	return pot;
}

const int IMPULSE_DELAY = 150;

/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView() :
m_postProcessManager()			// Invoke the postprocess Manager Constructor - J.C.Denton
{
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = NULL;
	dvMaterial = declManager->FindMaterial( "_scratch" );
	tunnelMaterial = declManager->FindMaterial( "textures/darkmod/decals/tunnel" );	// damage overlay
	bloodSprayMaterial = declManager->FindMaterial( "textures/decals/bloodspray" );
	lagoMaterial = declManager->FindMaterial( LAGO_MATERIAL, false );

	dvFinishTime = 0;
	kickFinishTime = 0;
	kickAngles.Zero();
	lastDamageTime = 0.0f;
	fadeTime = 0;
	fadeRate = 0.0;
	fadeFromColor.Zero();
	fadeToColor.Zero();
	fadeColor.Zero();
	shakeAng.Zero();

	/*
	fxManager = NULL;

	if ( !fxManager ) {
	fxManager = new FullscreenFXManager;
	fxManager->Initialize( this );
	}
	*/

	ClearEffects();

	// JC: Just set the flag so that we know that the update is needed.
	cv_ambient_method.SetModified();
	cv_interaction_vfp_type.SetModified();	// Always update interaction shader the first time. J.C.Denton
}

/*
==============
idPlayerView::Save
==============
*/
void idPlayerView::Save( idSaveGame *savefile ) const {
	int i;
	const screenBlob_t *blob;

	blob = &screenBlobs[ 0 ];
	for( i = 0; i < MAX_SCREEN_BLOBS; i++, blob++ ) {
		savefile->WriteMaterial( blob->material );
		savefile->WriteFloat( blob->x );
		savefile->WriteFloat( blob->y );
		savefile->WriteFloat( blob->w );
		savefile->WriteFloat( blob->h );
		savefile->WriteFloat( blob->s1 );
		savefile->WriteFloat( blob->t1 );
		savefile->WriteFloat( blob->s2 );
		savefile->WriteFloat( blob->t2 );
		savefile->WriteInt( blob->finishTime );
		savefile->WriteInt( blob->startFadeTime );
		savefile->WriteFloat( blob->driftAmount );
	}

	savefile->WriteInt( dvFinishTime );
	savefile->WriteMaterial( dvMaterial );
	savefile->WriteInt( kickFinishTime );
	savefile->WriteAngles( kickAngles );

	savefile->WriteMaterial( tunnelMaterial );
	savefile->WriteMaterial( bloodSprayMaterial );
	savefile->WriteFloat( lastDamageTime );

	savefile->WriteVec4( fadeColor );
	savefile->WriteVec4( fadeToColor );
	savefile->WriteVec4( fadeFromColor );
	savefile->WriteFloat( fadeRate );
	savefile->WriteInt( fadeTime );

	savefile->WriteAngles( shakeAng );

	savefile->WriteObject( player );
	savefile->WriteRenderView( view );
}

/*
==============
idPlayerView::Restore
==============
*/
void idPlayerView::Restore( idRestoreGame *savefile ) {
	int i;
	screenBlob_t *blob;

	blob = &screenBlobs[ 0 ];
	for( i = 0; i < MAX_SCREEN_BLOBS; i++, blob++ ) {
		savefile->ReadMaterial( blob->material );
		savefile->ReadFloat( blob->x );
		savefile->ReadFloat( blob->y );
		savefile->ReadFloat( blob->w );
		savefile->ReadFloat( blob->h );
		savefile->ReadFloat( blob->s1 );
		savefile->ReadFloat( blob->t1 );
		savefile->ReadFloat( blob->s2 );
		savefile->ReadFloat( blob->t2 );
		savefile->ReadInt( blob->finishTime );
		savefile->ReadInt( blob->startFadeTime );
		savefile->ReadFloat( blob->driftAmount );
	}

	savefile->ReadInt( dvFinishTime );
	savefile->ReadMaterial( dvMaterial );
	savefile->ReadInt( kickFinishTime );
	savefile->ReadAngles( kickAngles );			

	savefile->ReadMaterial( tunnelMaterial );
	savefile->ReadMaterial( bloodSprayMaterial );
	savefile->ReadFloat( lastDamageTime );

	savefile->ReadVec4( fadeColor );
	savefile->ReadVec4( fadeToColor );
	savefile->ReadVec4( fadeFromColor );
	savefile->ReadFloat( fadeRate );
	savefile->ReadInt( fadeTime );

	savefile->ReadAngles( shakeAng );

	savefile->ReadObject( reinterpret_cast<idClass *&>( player ) );
	savefile->ReadRenderView( view );

	// Re-Initialize the PostProcess Manager.	- JC Denton
	this->m_postProcessManager.Initialize();
}

/*
==============
idPlayerView::SetPlayerEntity
==============
*/
void idPlayerView::SetPlayerEntity( idPlayer *playerEnt ) {
	player = playerEnt;
}

/*
==============
idPlayerView::ClearEffects
==============
*/
void idPlayerView::ClearEffects() {
	lastDamageTime = MS2SEC( gameLocal.time - 99999 );

	dvFinishTime = ( gameLocal.time - 99999 );
	kickFinishTime = ( gameLocal.time - 99999 );

	for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		screenBlobs[i].finishTime = gameLocal.time;
	}

	fadeTime = 0;
}

/*
==============
idPlayerView::GetScreenBlob
==============
*/
screenBlob_t *idPlayerView::GetScreenBlob() {
	screenBlob_t	*oldest = &screenBlobs[0];

	for ( int i = 1 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		if ( screenBlobs[i].finishTime < oldest->finishTime ) {
			oldest = &screenBlobs[i];
		}
	}
	return oldest;
}

/*
==============
idPlayerView::DamageImpulse

LocalKickDir is the direction of force in the player's coordinate system,
which will determine the head kick direction
==============
*/
void idPlayerView::DamageImpulse( idVec3 localKickDir, const idDict *damageDef ) {
	//
	// double vision effect
	//
	if ( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.time ) {
		// keep shotgun from obliterating the view
		return;
	}

	float	dvTime = damageDef->GetFloat( "dv_time" );
	if ( dvTime ) {
		if ( dvFinishTime < gameLocal.time ) {
			dvFinishTime = gameLocal.time;
		}
		dvFinishTime += static_cast<int>(g_dvTime.GetFloat() * dvTime);
		// don't let it add up too much in god mode
		if ( dvFinishTime > gameLocal.time + 5000 ) {
			dvFinishTime = gameLocal.time + 5000;
		}
	}

	//
	// head angle kick
	//
	float	kickTime = damageDef->GetFloat( "kick_time" );
	if ( kickTime ) {
		kickFinishTime = gameLocal.time + static_cast<int>(g_kickTime.GetFloat() * kickTime);

		// forward / back kick will pitch view
		kickAngles[0] = localKickDir[0];

		// side kick will yaw view
		kickAngles[1] = localKickDir[1]*0.5f;

		// up / down kick will pitch view
		kickAngles[0] += localKickDir[2];

		// roll will come from  side
		kickAngles[2] = localKickDir[1];

		float kickAmplitude = damageDef->GetFloat( "kick_amplitude" );
		if ( kickAmplitude ) {
			kickAngles *= kickAmplitude;
		}
	}

	//
	// screen blob
	//
	float	blobTime = damageDef->GetFloat( "blob_time" );
	if ( blobTime ) {
		screenBlob_t	*blob = GetScreenBlob();
		blob->startFadeTime = gameLocal.time;
		blob->finishTime = gameLocal.time + static_cast<int>(blobTime * g_blobTime.GetFloat());

		const char *materialName = damageDef->GetString( "mtr_blob" );
		blob->material = declManager->FindMaterial( materialName );
		blob->x = damageDef->GetFloat( "blob_x" );
		blob->x += ( gameLocal.random.RandomInt()&63 ) - 32;
		blob->y = damageDef->GetFloat( "blob_y" );
		blob->y += ( gameLocal.random.RandomInt()&63 ) - 32;

		float scale = ( 256 + ( ( gameLocal.random.RandomInt()&63 ) - 32 ) ) / 256.0f;
		blob->w = damageDef->GetFloat( "blob_width" ) * g_blobSize.GetFloat() * scale;
		blob->h = damageDef->GetFloat( "blob_height" ) * g_blobSize.GetFloat() * scale;
		blob->s1 = 0;
		blob->t1 = 0;
		blob->s2 = 1;
		blob->t2 = 1;
	}

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.time );

}

/*
==================
idPlayerView::AddBloodSpray

If we need a more generic way to add blobs then we can do that
but having it localized here lets the material be pre-looked up etc.
==================
*/
void idPlayerView::AddBloodSpray( float duration ) {
	/*
	if ( duration <= 0 || bloodSprayMaterial == NULL || g_skipViewEffects.GetBool() ) {
	return;
	}
	// visit this for chainsaw
	screenBlob_t *blob = GetScreenBlob();
	blob->startFadeTime = gameLocal.time;
	blob->finishTime = gameLocal.time + ( duration * 1000 );
	blob->material = bloodSprayMaterial;
	blob->x = ( gameLocal.random.RandomInt() & 63 ) - 32;
	blob->y = ( gameLocal.random.RandomInt() & 63 ) - 32;
	blob->driftAmount = 0.5f + gameLocal.random.CRandomFloat() * 0.5;
	float scale = ( 256 + ( ( gameLocal.random.RandomInt()&63 ) - 32 ) ) / 256.0f;
	blob->w = 600 * g_blobSize.GetFloat() * scale;
	blob->h = 480 * g_blobSize.GetFloat() * scale;
	float s1 = 0.0f;
	float t1 = 0.0f;
	float s2 = 1.0f;
	float t2 = 1.0f;
	if ( blob->driftAmount < 0.6 ) {
	s1 = 1.0f;
	s2 = 0.0f;
	} else if ( blob->driftAmount < 0.75 ) {
	t1 = 1.0f;
	t2 = 0.0f;
	} else if ( blob->driftAmount < 0.85 ) {
	s1 = 1.0f;
	s2 = 0.0f;
	t1 = 1.0f;
	t2 = 0.0f;
	}
	blob->s1 = s1;
	blob->t1 = t1;
	blob->s2 = s2;
	blob->t2 = t2;
	*/
}

/*
==================
idPlayerView::WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayerView::WeaponFireFeedback( const idDict *weaponDef ) {
	int		recoilTime;

	recoilTime = weaponDef->GetInt( "recoilTime" );
	// don't shorten a damage kick in progress
	if ( recoilTime && kickFinishTime < gameLocal.time ) {
		idAngles angles;
		weaponDef->GetAngles( "recoilAngles", "5 0 0", angles );
		kickAngles = angles;
		int	finish = gameLocal.time + static_cast<int>(g_kickTime.GetFloat() * recoilTime);
		kickFinishTime = finish;
	}	

}

/*
===================
idPlayerView::CalculateShake
===================
*/
void idPlayerView::CalculateShake() {
	idVec3	origin, matrix;

	float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.time, player->firstPersonViewOrigin );
	//
	// shakeVolume should somehow be molded into an angle here
	// it should be thought of as being in the range 0.0 -> 1.0, although
	// since CurrentShakeAmplitudeForPosition() returns all the shake sounds
	// the player can hear, it can go over 1.0 too.
	//
	shakeAng[0] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[1] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[2] = gameLocal.random.CRandomFloat() * shakeVolume;
}

/*
===================
idPlayerView::ShakeAxis
===================
*/
idMat3 idPlayerView::ShakeAxis() const {
	return shakeAng.ToMat3();
}

/*
===================
idPlayerView::AngleOffset

kickVector, a world space direction that the attack should 
===================
*/
idAngles idPlayerView::AngleOffset() const {
	idAngles	ang;

	ang.Zero();

	if ( gameLocal.time < kickFinishTime ) {
		float offset = kickFinishTime - gameLocal.time;

		ang = kickAngles * offset * offset * g_kickAmplitude.GetFloat();

		for ( int i = 0 ; i < 3 ; i++ ) {
			if ( ang[i] > 70.0f ) {
				ang[i] = 70.0f;
			} else if ( ang[i] < -70.0f ) {
				ang[i] = -70.0f;
			}
		}
	}
	return ang;
}

/*
==================
idPlayerView::SingleView
==================
*/
void idPlayerView::SingleView( idUserInterface *hud, const renderView_t *view, bool drawHUD ) {

	// normal rendering
	if ( !view ) {
		return;
	}

	// place the sound origin for the player
	// TODO: Support overriding the location area so that reverb settings can be applied for listening thru doors?
	gameSoundWorld->PlaceListener( player->GetListenerLoc(), view->viewaxis, player->entityNumber + 1, gameLocal.time, hud ? hud->State().GetString( "location" ) : "Undefined" );

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

	//gameRenderWorld->RenderScene( &hackedView );

	// grayman #3108 - contributed by neuro & 7318
	idVec3 diff, currentEyePos, PSOrigin, Zero;
	
	Zero.Zero();
		
	if ( ( gameLocal.CheckGlobalPortalSky() ) || ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL ) ) {		
		// in a case of a moving portalSky
		
		currentEyePos = hackedView.vieworg;
		
		if ( gameLocal.playerOldEyePos == Zero ) {
			// Initialize playerOldEyePos. This will only happen in one tick.
			gameLocal.playerOldEyePos = currentEyePos;
		}

		diff = ( currentEyePos - gameLocal.playerOldEyePos) / gameLocal.portalSkyScale;
		gameLocal.portalSkyGlobalOrigin += diff; // This is for the global portalSky.
												 // It should keep going even when not active.
	}

	if ( gameLocal.portalSkyEnt.GetEntity() && gameLocal.IsPortalSkyActive() && g_enablePortalSky.GetBool() ) {
		
		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_STANDARD ) {
			PSOrigin = gameLocal.portalSkyOrigin;
		}
		
		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_GLOBAL ) {
			PSOrigin = gameLocal.portalSkyGlobalOrigin;
		}
		
		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL ) {
			gameLocal.portalSkyOrigin += diff;
			PSOrigin = gameLocal.portalSkyOrigin;
		}
	
		gameLocal.playerOldEyePos = currentEyePos;
		// end neuro & 7318

		renderView_t portalView = hackedView;

		portalView.vieworg = PSOrigin;	// grayman #3108 - contributed by neuro & 7318
//		portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();
		portalView.viewaxis = portalView.viewaxis * gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetAxis();

		// setup global fixup projection vars
		if ( 1 ) {
			int vidWidth, vidHeight;
			idVec2 shiftScale;

			renderSystem->GetGLSettings( vidWidth, vidHeight );

			float pot;
			int	 w = vidWidth;
			pot = MakePowerOfTwo( w );
			shiftScale.x = (float)w / pot;

			int	 h = vidHeight;
			pot = MakePowerOfTwo( h );
			shiftScale.y = (float)h / pot;

			hackedView.shaderParms[6] = shiftScale.x; // grayman #3108 - neuro used [4], we use [6]
			hackedView.shaderParms[7] = shiftScale.y; // grayman #3108 - neuro used [5], we use [7]
		}

		gameRenderWorld->RenderScene( &portalView );
		renderSystem->CaptureRenderToImage( "_currentRender" );

		hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
	}
	else // grayman #3108 - contributed by 7318 
	{
		// So if g_enablePortalSky is disabled, GlobalPortalSkies doesn't break.
		// When g_enablePortalSky gets re-enabled, GlobalPortalSkies keeps working. 
		gameLocal.playerOldEyePos = currentEyePos;
	}

	hackedView.forceUpdate = true; // Fix for lightgem problems? -Gildoran
	gameRenderWorld->RenderScene( &hackedView );
	// process the frame

	//	fxManager->Process( &hackedView );

	if ( player->spectating ) {
		return;
	}

	// draw screen blobs
	if ( !pm_thirdPerson.GetBool() && !g_skipViewEffects.GetBool() ) {
		for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
			screenBlob_t	*blob = &screenBlobs[i];
			if ( blob->finishTime <= gameLocal.time ) {
				continue;
			}

			blob->y += blob->driftAmount;

			float	fade = (float)( blob->finishTime - gameLocal.time ) / ( blob->finishTime - blob->startFadeTime );
			if ( fade > 1.0f ) {
				fade = 1.0f;
			}
			if ( fade ) {
				renderSystem->SetColor4( 1,1,1,fade );
				renderSystem->DrawStretchPic( blob->x, blob->y, blob->w, blob->h,blob->s1, blob->t1, blob->s2, blob->t2, blob->material );
			}
		}
		if (drawHUD)
		{
			player->DrawHUD( hud );
		}

		// tunnel vision

		// grayman - This is where the red screen overlay is
		// applied for player damage. The red overlay's alpha is
		// the player's current health divided by his max health.
		// Less health when damaged means more overlay is visible.
		// The amount of alpha is also used to determine how long
		// the overlay is visible, its alpha climbing to 1.0
		// over the duration of the effect. Less health means a longer
		// duration. Some key words for search purposes:
		// damage dealt, damage hud, hurt hud, blood overlay

		float health = 0.0f;
		if ( g_testHealthVision.GetFloat() != 0.0f )
		{
			health = g_testHealthVision.GetFloat();
		}
		else
		{
			health = player->health;
		}

		float alpha = health / 100.0f;
		if ( alpha < 0.0f )
		{
			alpha = 0.0f;
		}
		else if ( alpha > 1.0f )
		{
			alpha = 1.0f;
		}

		if ( alpha < 1.0f  )
		{
			// Tels: parm0: when the last damage occured
			// Tels: parm1: TODO: set here f.i. to color the material different when in gas cloud
			// Tels: parm2: TODO: set here f.i. to color the material different when poisoned
			// Tels: parm3: alpha value, depending on health
			renderSystem->SetColor4( ( player->health <= 0.0f ) ? MS2SEC( gameLocal.time ) : lastDamageTime, 1.0f, 1.0f, ( player->health <= 0.0f ) ? 0.0f : alpha );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, tunnelMaterial );
		}
	}

	// Rotoscope (Cartoon-like) rendering - (Rotoscope Shader v1.0 by Hellborg) - added by Dram
	if ( g_rotoscope.GetBool() ) {
		const idMaterial *mtr = declManager->FindMaterial( "textures/postprocess/rotoedge", false );
		if ( !mtr ) {
			common->Printf( "Rotoscope material not found.\n" );
		} else {
			renderSystem->CaptureRenderToImage( "_currentRender" );
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}

	// test a single material drawn over everything
	if ( g_testPostProcess.GetString()[0] ) {
		const idMaterial *mtr = declManager->FindMaterial( g_testPostProcess.GetString(), false );
		if ( !mtr ) {
			common->Printf( "Material not found.\n" );
			g_testPostProcess.SetString( "" );
		} else {
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}
}

/*
===================
idPlayerView::DoubleVision
===================
*/
void idPlayerView::DoubleVision( idUserInterface *hud, const renderView_t *view, int offset ) {

	if ( !g_doubleVision.GetBool() ) {
		SingleView( hud, view );
		return;
	}

	float	scale = offset * g_dvAmplitude.GetFloat();
	if ( scale > 0.5f ) {
		scale = 0.5f;
	}
	float shift = scale * sin( sqrt( (float)offset ) * g_dvFrequency.GetFloat() );
	shift = fabs( shift );

	// if double vision, render to a texture
	renderSystem->CropRenderSize( SCREEN_WIDTH, SCREEN_HEIGHT, true );

	// greebo: Draw the single view, but skip the HUD, this is done later
	SingleView( hud, view, false ); 

	renderSystem->CaptureRenderToImage( "_scratch" );
	renderSystem->UnCrop();

	idVec4 color(1, 1, 1, 1);

	renderSystem->SetColor4( color.x, color.y, color.z, 1.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, shift, 1-shift, 1, 0, dvMaterial );
	renderSystem->SetColor4( color.x, color.y, color.z, 0.5f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1-shift, shift, dvMaterial );

	// Do not post-process the HUD - JC Denton
	// Bloom related - added by Dram
	// 	if ( r_bloom_hud.GetBool() || !r_bloom.GetBool() ) // If HUD blooming is enabled or bloom is disabled
	// 	{
	// 		player->DrawHUD(hud);
	// 	}
}

/*
===================
idPlayerView::BerserkVision
===================
*/
void idPlayerView::BerserkVision( idUserInterface *hud, const renderView_t *view ) {
	renderSystem->CropRenderSize( 512, 256, true );
	SingleView( hud, view );
	renderSystem->CaptureRenderToImage( "_scratch" );
	renderSystem->UnCrop();
	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, dvMaterial );
}


/*
=================
idPlayerView::Flash

flashes the player view with the given color
=================
*/
void idPlayerView::Flash(idVec4 color, int time ) {
	Fade(idVec4(0, 0, 0, 0), time);
	fadeFromColor = colorWhite;
}

/*
=================
idPlayerView::Fade

used for level transition fades
assumes: color.w is 0 or 1
=================
*/
void idPlayerView::Fade( idVec4 color, int time ) {

	if ( !fadeTime ) {
		fadeFromColor.Set( 0.0f, 0.0f, 0.0f, 1.0f - color[ 3 ] );
	} else {
		fadeFromColor = fadeColor;
	}
	fadeToColor = color;

	if ( time <= 0 ) {
		fadeRate = 0;
		time = 0;
		fadeColor = fadeToColor;
	} else {
		fadeRate = 1.0f / ( float )time;
	}

	if ( gameLocal.realClientTime == 0 && time == 0 ) {
		fadeTime = 1;
	} else {
		fadeTime = gameLocal.realClientTime + time;
	}
}

/*
=================
idPlayerView::ScreenFade
=================
*/
void idPlayerView::ScreenFade() {
	int		msec;
	float	t;

	if ( !fadeTime ) {
		return;
	}

	msec = fadeTime - gameLocal.realClientTime;

	if ( msec <= 0 ) {
		fadeColor = fadeToColor;
		if ( fadeColor[ 3 ] == 0.0f ) {
			fadeTime = 0;
		}
	} else {
		t = ( float )msec * fadeRate;
		fadeColor = fadeFromColor * t + fadeToColor * ( 1.0f - t );
	}

	if ( fadeColor[ 3 ] != 0.0f ) {
		renderSystem->SetColor4( fadeColor[ 0 ], fadeColor[ 1 ], fadeColor[ 2 ], fadeColor[ 3 ] );
		renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
	}
}

/*
===================
idPlayerView::InfluenceVision
===================
*/
void idPlayerView::InfluenceVision( idUserInterface *hud, const renderView_t *view ) {

	float distance = 0.0f;
	float pct = 1.0f;
	if ( player->GetInfluenceEntity() ) {
		distance = ( player->GetInfluenceEntity()->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin() ).Length();
		if ( player->GetInfluenceRadius() != 0.0f && distance < player->GetInfluenceRadius() ) {
			pct = distance / player->GetInfluenceRadius();
			pct = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, pct );
		}
	}
	if ( player->GetInfluenceMaterial() ) {
		SingleView( hud, view );
		renderSystem->CaptureRenderToImage( "_currentRender" );

		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, pct );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, player->GetInfluenceMaterial() );
	} else if ( player->GetInfluenceEntity() == NULL ) {
		SingleView( hud, view );
		return;
	} else {
		int offset =  static_cast<int>(25 + sin(static_cast<float>(gameLocal.time)));
		DoubleVision( hud, view, static_cast<int>(pct * offset) );
	}
}

/*
===================
idPlayerView::RenderPlayerView
===================
*/
void idPlayerView::RenderPlayerView( idUserInterface *hud )
{
	const renderView_t *view = player->GetRenderView();

	if(g_skipViewEffects.GetBool())
	{
		SingleView( hud, view );
	} else {

		/*if ( player->GetInfluenceMaterial() || player->GetInfluenceEntity() ) {
		InfluenceVision( hud, view );
		} else if ( gameLocal.time < dvFinishTime ) {
		DoubleVision( hud, view, dvFinishTime - gameLocal.time );
		} else {*/

		// greebo: For underwater effects, use the Doom3 Doubleview
		if (static_cast<idPhysics_Player*>(player->GetPlayerPhysics())->GetWaterLevel() >= WATERLEVEL_HEAD)
		{
			DoubleVision(hud, view, cv_tdm_underwater_blur.GetInteger());
		}
		else
		{
			// Do not postprocess the HUD
			// 			if ( r_bloom_hud.GetBool() || !r_bloom.GetBool() ) // If HUD blooming is enabled or bloom is disabled
			// 			{
			// 				SingleView( hud, view );
			// 			}
			// 			else
			{
				SingleView( hud, view, false );
			}
		}
		//}

		// Bloom related - J.C.Denton
		/* Update  post-process */
		this->m_postProcessManager.Update();

		ScreenFade();
	}

	player->DrawHUD(hud);


	// TDM Ambient Method checking. By Dram
	// Modified by JC Denton
	if ( cv_ambient_method.IsModified() ) // If the ambient method option has changed
	{
		UpdateAmbientLight();
	}
}

void idPlayerView::UpdateAmbientLight()
{
	// Finds a light with name set as ambient_world, or turns the ambient light with greatest radius into main ambient.
	idLight* pAmbientLight = gameLocal.FindMainAmbientLight(true);

	if (pAmbientLight != NULL) // If the light exists
	{
		if ( 0 == cv_ambient_method.GetInteger() ) // If the Ambient Light method is used
		{
			gameLocal.globalShaderParms[5] = 0;				// Make sure we set this flag to 0 so that materials know which pass is to be enabled.
			gameLocal.globalShaderParms[2] = 0; // Set global shader parm 2 to 0
			gameLocal.globalShaderParms[3] = 0; // Set global shader parm 3 to 0
			gameLocal.globalShaderParms[4] = 0; // Set global shader parm 4 to 0

			pAmbientLight->On(); // Turn on ambient light
		}
		else // If the Texture Brightness method is used
		{

			gameLocal.globalShaderParms[5] = 1;				// enable the extra shader branch
			idVec3 ambient_color = pAmbientLight->spawnArgs.GetVector( "_color" );				 // Get the ambient color from the spawn arguments
			gameLocal.globalShaderParms[2] = ambient_color.x * 1.5f; // Set global shader parm 2 to Red value of ambient light
			gameLocal.globalShaderParms[3] = ambient_color.y * 1.5f; // Set global shader parm 3 to Green value of ambient light
			gameLocal.globalShaderParms[4] = ambient_color.z * 1.5f; // Set global shader parm 4 to Blue value of ambient light

			pAmbientLight->Off(); // Turn off ambient light

		}
	}
	else // The ambient light does not exist
	{
		gameLocal.Printf( "Note: The main ambient light could not be determined\n"); // Show in console of light not existing in map
	}
	cv_ambient_method.ClearModified();
}

void idPlayerView::OnReloadImages()
{
	m_postProcessManager.ScheduleCookedDataUpdate();
}

void idPlayerView::OnVidRestart()
{
	m_postProcessManager.ScheduleCookedDataUpdate();
}

/*
===================
idPlayerView::dnPostProcessManager Class Definitions - JC Denton
===================
*/

idPlayerView::dnPostProcessManager::dnPostProcessManager():
m_imageCurrentRender				( "_currentRender"			),
m_imageBloom						( "_bloomImage"				),
m_imageCookedMath					( "_cookedMath"				),

m_matBrightPass			( declManager->FindMaterial( "postprocess/brightPassOptimized" )		),
m_matGaussBlurX			( declManager->FindMaterial( "postprocess/blurx" )			),
m_matGaussBlurY			( declManager->FindMaterial( "postprocess/blury" )			),
//m_matFinalScenePass		( declManager->FindMaterial( "postprocess/finalScenePass" )	),
m_matFinalScenePass		( declManager->FindMaterial( "postprocess/finalScenePassOptimized" )	),

m_matCookMath_pass1		( declManager->FindMaterial( "postprocess/cookMath_pass1" )		),
m_matCookMath_pass2		( declManager->FindMaterial( "postprocess/cookMath_pass2" )		),
m_ImageAnisotropyHandle(-1)
{
	m_iScreenHeight = m_iScreenWidth = 0;
	m_iScreenHeightPowOf2 = m_iScreenWidthPowOf2 = 0;
	m_nFramesToUpdateCookedData = 0;

	// Initialize once this object is created.	
	this->Initialize();

	// Get notified on image anisotropy changes
	idCVar* imageAnistropy = cvarSystem->Find("image_anisotropy");

	if (imageAnistropy != NULL)
	{
		m_ImageAnisotropyHandle = imageAnistropy->AddOnModifiedCallback(
			boost::bind(&idPlayerView::dnPostProcessManager::OnImageAnisotropyChanged, this));
	}
}

idPlayerView::dnPostProcessManager::~dnPostProcessManager()
{
	idCVar* imageAnistropy = cvarSystem->Find("image_anisotropy");

	if (imageAnistropy != NULL && m_ImageAnisotropyHandle != -1)
	{
		imageAnistropy->RemoveOnModifiedCallback(m_ImageAnisotropyHandle);
	}

}

void idPlayerView::dnPostProcessManager::OnImageAnisotropyChanged()
{
	ScheduleCookedDataUpdate();
}

void idPlayerView::dnPostProcessManager::ScheduleCookedDataUpdate()
{
	m_nFramesToUpdateCookedData = 1;

	if ( r_postprocess.GetBool())
	{
		gameLocal.Printf("Cooked Data will be updated after %d frames...\n", m_nFramesToUpdateCookedData);
	}
	else
	{
		gameLocal.Printf("Cooked Data will be updated after %d frames immediately after r_postprocess is enabled.\n", m_nFramesToUpdateCookedData);
	}
}

void idPlayerView::dnPostProcessManager::Initialize()
{
	m_bForceUpdateOnCookedData = true;
	r_postprocess_bloomKernelSize.SetModified(); // This will print message in console about bloom kernel size. 
}

void idPlayerView::dnPostProcessManager::UpdateCookedData( void )
{

	if( m_nFramesToUpdateCookedData > 0 )
	{
		m_nFramesToUpdateCookedData --;
		m_bForceUpdateOnCookedData = true;
		return;
	}

	if (	m_bForceUpdateOnCookedData || 
			r_postprocess_colorCurveBias.IsModified() || r_postprocess_brightPassOffset.IsModified()	|| 
			r_postprocess_brightPassThreshold.IsModified() || r_postprocess_sceneExposure.IsModified()	||
			r_postprocess_sceneGamma.IsModified() || r_postprocess_colorCorrection.IsModified()			||
			r_postprocess_colorCorrectBias.IsModified()
		)
	{

		if( m_bForceUpdateOnCookedData )
			gameLocal.Printf( "Forcing an update on cooked math data.\n" );

		gameLocal.Printf( "Cooking math data please wait...\n" );

		//------------------------------------------------------------------------
		// Crop backbuffer image to the size of our cooked math image
		//------------------------------------------------------------------------
		renderSystem->CropRenderSize(256, 1, true);
		//------------------------------------------------------------------------

		//------------------------------------------------------------------------
		// Cook math Pass 1 
		//------------------------------------------------------------------------
		renderSystem->SetColor4( r_postprocess_colorCurveBias.GetFloat(), r_postprocess_sceneGamma.GetFloat(), r_postprocess_sceneExposure.GetFloat(), 1.0f );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matCookMath_pass1 );
		renderSystem->CaptureRenderToImage( m_imageCookedMath );

		//------------------------------------------------------------------------
		// Cook math Pass 2 
		//------------------------------------------------------------------------
		float fColorCurveBias = Max ( Min ( r_postprocess_colorCorrectBias.GetFloat(), 1.0f ), 0.0f );
 		renderSystem->SetColor4( r_postprocess_brightPassThreshold.GetFloat(), r_postprocess_brightPassOffset.GetFloat(), r_postprocess_colorCorrection.GetFloat(), fColorCurveBias );
 		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matCookMath_pass2 );
 		renderSystem->CaptureRenderToImage( m_imageCookedMath );

		//------------------------------------------------------------------------
		renderSystem->UnCrop();
		//------------------------------------------------------------------------
		r_postprocess_colorCurveBias.ClearModified(); 
		r_postprocess_brightPassOffset.ClearModified();
		r_postprocess_brightPassThreshold.ClearModified();
		r_postprocess_sceneExposure.ClearModified();
		r_postprocess_sceneGamma.ClearModified();
		r_postprocess_colorCorrection.ClearModified();
		r_postprocess_colorCorrectBias.ClearModified();

		m_bForceUpdateOnCookedData = false;

		gameLocal.Printf( "Cooking complete.\n" );

		//gameLocal.Printf( "Screen size: %d, %d Power of 2 Size: %d, %d", m_iScreenWidth, m_iScreenHeight, m_iScreenWidthPowOf2, m_iScreenHeightPowOf2 );
	}
}


void idPlayerView::dnPostProcessManager::Update( void )
{
	float fBloomImageDownScale = Max(Min(r_postprocess_bloomKernelSize.GetInteger(), 2), 1 ) == 1 ? 2 : 4;

	if( r_postprocess_bloomKernelSize.IsModified() )
	{
		gameLocal.Printf(" Bloom Kernel size is set to: %s \n", fBloomImageDownScale == 2.0f ? "Large": "Small" );
		r_postprocess_bloomKernelSize.ClearModified();
	}

	// Check the interaction.vfp settings
	if( cv_interaction_vfp_type.IsModified() )
	{
		this->UpdateInteractionShader();
		cv_interaction_vfp_type.ClearModified();
	}

	const int iPostProcessType = r_postprocess.GetInteger();

	if ( iPostProcessType != 0 ) 
	{
		this->UpdateBackBufferParameters();

		// Note to self1: CropRenderSize if not used before CaptureRenderToImage, then image caputured is of screen's size(non power of two) 
		// Note to self2: CropRenderSize when used with dimensions greater than backbuffer res, automatically crops screen to res <= backbuffer res.

		renderSystem->CaptureRenderToImage( m_imageCurrentRender );

		this->UpdateCookedData();

		const float fBloomIntensity = r_postprocess_bloomIntensity.GetFloat();
		
		if( fBloomIntensity > 0.0f )
		{
			//-------------------------------------------------
			// Apply the bright-pass filter to acquire bloom image
			//-------------------------------------------------
			renderSystem->CropRenderSize(m_iScreenWidthPowOf2/fBloomImageDownScale, m_iScreenHeightPowOf2/fBloomImageDownScale, true);

			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matBrightPass );
			renderSystem->CaptureRenderToImage( m_imageBloom );

			//-------------------------------------------------
			// Apply Gaussian Smoothing to create bloom
			//-------------------------------------------------

			renderSystem->SetColor4( fBloomImageDownScale/m_iScreenWidthPowOf2, 1.0f, 1.0f, 1.0f );			 
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matGaussBlurX );
			renderSystem->CaptureRenderToImage( m_imageBloom );
			renderSystem->SetColor4( fBloomImageDownScale/m_iScreenHeightPowOf2, 1.0f, 1.0f, 1.0f );		 
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matGaussBlurY );

			renderSystem->CaptureRenderToImage( m_imageBloom );
			renderSystem->UnCrop();
			//---------------------

		}

		//-------------------------------------------------
		// Calculate and Render Final Image
		//-------------------------------------------------
		float fDesaturation = Max ( Min ( r_postprocess_desaturation.GetFloat(), 1.0f ), 0.0f );
		renderSystem->SetColor4( fBloomIntensity, fDesaturation, 1.0f, 1.0f );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, m_fShiftScale_y, m_fShiftScale_x, 0, m_matFinalScenePass );
		//-------------------------------------------------

		this->RenderDebugTextures();
	}
}

void idPlayerView::dnPostProcessManager::UpdateBackBufferParameters()
{
	// This condition makes sure that, the 2 loops inside run once only when resolution changes or map starts.
	if( m_iScreenHeight != renderSystem->GetScreenHeight() || m_iScreenWidth !=renderSystem->GetScreenWidth() )
	{
		m_iScreenWidthPowOf2 = 256, m_iScreenHeightPowOf2 = 256;

		// This should probably fix the ATI issue...
		renderSystem->GetGLSettings( m_iScreenWidth, m_iScreenHeight );

		//assert( iScreenWidth != 0 && iScreenHeight != 0 );

		while( m_iScreenWidthPowOf2 < m_iScreenWidth ) {
			m_iScreenWidthPowOf2 <<= 1;
		}
		while( m_iScreenHeightPowOf2 < m_iScreenHeight ) {
			m_iScreenHeightPowOf2 <<= 1;
		}
		m_fShiftScale_x = m_iScreenWidth  / (float)m_iScreenWidthPowOf2;
		m_fShiftScale_y = m_iScreenHeight / (float)m_iScreenHeightPowOf2;
	}
}

void idPlayerView::dnPostProcessManager::RenderDebugTextures()
{
	const int iDebugTexture = r_postprocess_debugMode.GetInteger();

	if( 0 < iDebugTexture && 4 > iDebugTexture ) 
	{
		struct {
			dnImageWrapper *m_pImage;
			float m_fShiftScaleX, m_fShiftScaleY;
		} 
		const arrStretchedImages[3] = { 
				{&m_imageCurrentRender,	m_fShiftScale_x, m_fShiftScale_y },
				{&m_imageBloom,			m_fShiftScale_x, m_fShiftScale_y },
				{&m_imageCookedMath,	1.0f, 1.0f},
		};
		
		int i = iDebugTexture - 1;

		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
 			renderSystem->DrawStretchPic( 0, SCREEN_HEIGHT * .2f, SCREEN_WIDTH * 0.6f, SCREEN_HEIGHT * 0.6f, 0, 
				arrStretchedImages[i].m_fShiftScaleY, arrStretchedImages[i].m_fShiftScaleX, 0, 
				*arrStretchedImages[i].m_pImage );
	}
}

// Moved Greebo's method from gameLocal to here. - J.C.Denton
// The CVar is rendering related and from now on, would work when g_stoptime is set to 0

void idPlayerView::dnPostProcessManager::UpdateInteractionShader()
{
	// Check the CVARs
	switch (cv_interaction_vfp_type.GetInteger())
	{
	case 0: // Doom 3's default interaction shader
		gameLocal.Printf("Using default interaction.vfp\n");
		cvarSystem->SetCVarInteger("r_testARBProgram", 0);
		break;

	case 1: // JC Denton's enhanced interaction
		gameLocal.Printf("Using TDM's enhanced interaction\n");
		cvarSystem->SetCVarInteger("r_testARBProgram", 1);
		break;

	default:
		gameLocal.Warning("Unknown interaction type setting found, reverting to enhanced standard.");
		cv_interaction_vfp_type.SetInteger(0);
		this->UpdateInteractionShader();
	};
}

