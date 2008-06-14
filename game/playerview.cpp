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

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "game_local.h"

static int MakePowerOfTwo( int num ) {
	int		pot;

	for (pot = 1 ; pot < num ; pot<<=1) {}

	return pot;
}

const int IMPULSE_DELAY = 150;

// Bloom related - by JC_Denton & Maha_X - added by Dram
const char *blurxMaterial[6] = { "textures/fsfx/blurx64", "textures/fsfx/blurx128", "textures/fsfx/blurx256", "textures/fsfx/blurx512", "textures/fsfx/blurx1024", "textures/fsfx/blurx2048" };
const char *bluryMaterial[6] = { "textures/fsfx/blury32", "textures/fsfx/blury64", "textures/fsfx/blury128", "textures/fsfx/blury256", "textures/fsfx/blury512", "textures/fsfx/blury1024" };

/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView() {
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = NULL;
	dvMaterial = declManager->FindMaterial( "_scratch" );
	tunnelMaterial = declManager->FindMaterial( "textures/decals/tunnel" );
	armorMaterial = declManager->FindMaterial( "armorViewEffect" );
	berserkMaterial = declManager->FindMaterial( "textures/decals/berserk" );
	irGogglesMaterial = declManager->FindMaterial( "textures/decals/irblend" );
	bloodSprayMaterial = declManager->FindMaterial( "textures/decals/bloodspray" );
	bfgMaterial = declManager->FindMaterial( "textures/decals/bfgvision" );
	lagoMaterial = declManager->FindMaterial( LAGO_MATERIAL, false );
	bfgVision = false;

	// Bloom related - by JC_Denton - added by Dram
	shiftSensitivityDelay = 0;
	screenHeight = screenWidth = 0;

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

	// greebo: Set the bool to the inverse of the CVAR, so that the code triggers 
	// an update in the first frame
	cur_amb_method = !cv_ambient_method.GetBool();
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
	savefile->WriteBool( bfgVision );

	savefile->WriteMaterial( tunnelMaterial );
	savefile->WriteMaterial( armorMaterial );
	savefile->WriteMaterial( berserkMaterial );
	savefile->WriteMaterial( irGogglesMaterial );
	savefile->WriteMaterial( bloodSprayMaterial );
	savefile->WriteMaterial( bfgMaterial );
	savefile->WriteFloat( lastDamageTime );

	savefile->WriteVec4( fadeColor );
	savefile->WriteVec4( fadeToColor );
	savefile->WriteVec4( fadeFromColor );
	savefile->WriteFloat( fadeRate );
	savefile->WriteInt( fadeTime );

	savefile->WriteAngles( shakeAng );

	savefile->WriteObject( player );
	savefile->WriteRenderView( view );

	savefile->WriteBool(cur_amb_method);
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
	savefile->ReadBool( bfgVision );

	savefile->ReadMaterial( tunnelMaterial );
	savefile->ReadMaterial( armorMaterial );
	savefile->ReadMaterial( berserkMaterial );
	savefile->ReadMaterial( irGogglesMaterial );
	savefile->ReadMaterial( bloodSprayMaterial );
	savefile->ReadMaterial( bfgMaterial );
	savefile->ReadFloat( lastDamageTime );

	savefile->ReadVec4( fadeColor );
	savefile->ReadVec4( fadeToColor );
	savefile->ReadVec4( fadeFromColor );
	savefile->ReadFloat( fadeRate );
	savefile->ReadInt( fadeTime );

	savefile->ReadAngles( shakeAng );

	savefile->ReadObject( reinterpret_cast<idClass *&>( player ) );
	savefile->ReadRenderView( view );

	savefile->ReadBool(cur_amb_method);
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
	bfgVision = false;
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

	// if the objective system is up, don't do normal drawing
	if ( player->objectiveSystemOpen ) {
		player->objectiveSystem->Redraw( gameLocal.time );
		return;
	}

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

	//gameRenderWorld->RenderScene( &hackedView );

	if ( gameLocal.portalSkyEnt.GetEntity() && gameLocal.IsPortalSkyAcive() && g_enablePortalSky.GetBool() ) {

		renderView_t	portalView = hackedView;

		portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();
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

			hackedView.shaderParms[4] = shiftScale.x;
			hackedView.shaderParms[5] = shiftScale.y;
		}

		gameRenderWorld->RenderScene( &portalView );
		renderSystem->CaptureRenderToImage( "_currentRender" );

		hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
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

		//PrintMessage( 100, 20, strText, idVec4( 1, 1, 1, 1 ), font_an );
		//PrintMessage( 100, 120, strText, idVec4( 1, 1, 1, 1 ), font_bank );
		//PrintMessage( 100, 140, strText, idVec4( 1, 1, 1, 1 ), font_micro );

		// armor impulse feedback
		float	armorPulse = ( gameLocal.time - player->lastArmorPulse ) / 250.0f;

		if ( armorPulse > 0.0f && armorPulse < 1.0f ) {
			renderSystem->SetColor4( 1, 1, 1, 1.0 - armorPulse );
			renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, armorMaterial );
		}


		// tunnel vision
		float	health = 0.0f;
		if ( g_testHealthVision.GetFloat() != 0.0f ) {
			health = g_testHealthVision.GetFloat();
		} else {
			health = player->health;
		}
		float alpha = health / 100.0f;
		if ( alpha < 0.0f ) {
			alpha = 0.0f;
		}
		if ( alpha > 1.0f ) {
			alpha = 1.0f;
		}

		if ( alpha < 1.0f  ) {
			renderSystem->SetColor4( ( player->health <= 0.0f ) ? MS2SEC( gameLocal.time ) : lastDamageTime, 1.0f, 1.0f, ( player->health <= 0.0f ) ? 0.0f : alpha );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, tunnelMaterial );
		}

		/*if ( player->PowerUpActive(BERSERK) ) {
			int berserkTime = player->inventory.powerupEndTime[ BERSERK ] - gameLocal.time;
			if ( berserkTime > 0 ) {
				// start fading if within 10 seconds of going away
				alpha = (berserkTime < 10000) ? (float)berserkTime / 10000 : 1.0f;
				renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, alpha );
				renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, berserkMaterial );
			}
		}*/

		if ( bfgVision ) {
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, bfgMaterial );
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

	// carry red tint if in berserk mode
	idVec4 color(1, 1, 1, 1);
	/*if ( gameLocal.time < player->inventory.powerupEndTime[ BERSERK ] ) {
		color.y = 0;
		color.z = 0;
	}*/

	renderSystem->SetColor4( color.x, color.y, color.z, 1.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, shift, 1-shift, 1, 0, dvMaterial );
	renderSystem->SetColor4( color.x, color.y, color.z, 0.5f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1-shift, shift, dvMaterial );

	// Bloom related - added by Dram
	if ( r_bloom_hud.GetBool() || !r_bloom.GetBool() ) // If HUD blooming is enabled or bloom is disabled
	{
		player->DrawHUD(hud);
	}
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
	}
	else 
	{
		// Bloom related - by JC_Denton & Maha_X - added by Dram
		// This condition makes sure that, the 2 loops inside run once only when resolution changes or map starts.
		if( screenHeight != renderSystem->GetScreenHeight() || screenWidth !=renderSystem->GetScreenWidth() ) {
			int width = 1, height = 1;	

			screenHeight	= renderSystem->GetScreenHeight();
			screenWidth		= renderSystem->GetScreenWidth();

			while( width < screenWidth ) {
				width <<= 1;
			}
			while( height < screenHeight ) {
				height <<= 1;
			}
			shiftScale_x = screenWidth  / (float)width;
			shiftScale_y = screenHeight / (float)height;
		}

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
			if ( r_bloom_hud.GetBool() || !r_bloom.GetBool() ) // If HUD blooming is enabled or bloom is disabled
			{
				SingleView( hud, view );
			}
			else
			{
				SingleView( hud, view, false );
			}
		}
		//}

		// =====================================================
		// Bloom related - by JC_Denton & Maha_X - added by Dram
		// =====================================================

		if ( !r_bloom.GetBool() ) // if bloom is disabled
		{
			// A global parameter (0) that is used by fake bloom effects to hide/lessen them etc - by Dram
			if ( gameLocal.globalShaderParms[0] != 1 ) // if the brightness of the hazes is not 1
			{
				gameLocal.globalShaderParms[0] = 1; // set it to 1
			}

			// A global parameter (1) that is used to scale the brightness of light rays if bloom is on - by Dram
			if ( gameLocal.globalShaderParms[1] != 1 ) // if the brightness scale of the light rays is not 1
			{
				gameLocal.globalShaderParms[1] = 1; // set it to 1
			}
		}
		else // if bloom is enabled
		{
			// set up the light haze brightness - by Dram
			float lightHazeBrightness = 1 - ( r_bloom_blur_mult.GetFloat() - 0.5 ); // get the blur mult and derive a haze brightness

			if ( lightHazeBrightness < 0.2f ) // if the haze brightness is lower then 0.2
			{
				lightHazeBrightness = 0.2f; // set it to 0.2
			}
			else if ( lightHazeBrightness > 1 ) // if the haze brightness is higher then 1
			{
				lightHazeBrightness = 1; // set it to 1
			}

			if ( gameLocal.globalShaderParms[0] != lightHazeBrightness ) // if the haze global parameter other then equals the light haze brightness
			{
				gameLocal.globalShaderParms[0] = lightHazeBrightness; // set it to light haze brightness
			}

			if ( gameLocal.globalShaderParms[1] != r_bloom_lightRayScale.GetFloat() ) // if the brightness scale of the light rays is not r_bloom_lightRayScale
			{
				gameLocal.globalShaderParms[1] = r_bloom_lightRayScale.GetFloat(); // set it to r_bloom_lightRayScale
			}

			renderSystem->CaptureRenderToImage( "_currentRender" );

			//------------------------------------------------
			// Maha_x's Shift Sensitivity - Modified for compatibilty by Clone JCDenton
			//------------------------------------------------

			bool disableShiftSensitivity = ( r_bloom_shift_delay.GetInteger() == -1 );

			if( !disableShiftSensitivity && gameLocal.time > shiftSensitivityDelay ) { 
				renderSystem->CropRenderSize(2, 2, true, true);
				shiftSensitivityDelay = gameLocal.time + r_bloom_shift_delay.GetInteger(); 
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, declManager->FindMaterial( "textures/AFX/AFXweight" ) );
				renderSystem->CaptureRenderToImage( "_afxweight" );
				renderSystem->UnCrop();
			}
			//------------------------------------------------

			//-------------------------------------------------
			// User configurable buffer size - By Clone JC Denton
			//-------------------------------------------------

			int bufferMult = 6 - r_bloom_buffer.GetInteger();
			int blurMtrIndex;

			if( bufferMult < 6 ) {
				if( bufferMult <= 0 ) {
					bufferMult = 1;
					blurMtrIndex = 5;
				}
				else {
					blurMtrIndex = 5 - bufferMult;
					bufferMult = 1<<bufferMult;
				}
				renderSystem->CropRenderSize(2048/bufferMult, 1024/bufferMult, true, true);
			}
			else {
				renderSystem->CropRenderSize(512, 256, true, true);
				blurMtrIndex = 3;
			}
			//-------------------------------------------------

			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, shiftScale_y, shiftScale_x, 0, declManager->FindMaterial( "_currentRender" ) );

			//Use the blurred image for obtaining bloom contrast.
			renderSystem->CaptureRenderToImage( "_fsfx_input" );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, declManager->FindMaterial( blurxMaterial[blurMtrIndex] ) );
			renderSystem->CaptureRenderToImage( "_fsfx_input" );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, declManager->FindMaterial( bluryMaterial[blurMtrIndex] ) );
			renderSystem->CaptureRenderToImage( "_fsfx_input" );	

			if( !disableShiftSensitivity ) {
				renderSystem->SetColor4(r_bloom_contrast_mult.GetFloat(), r_bloom_contrast_min.GetFloat(), 1, 1);
			} else {
				renderSystem->SetColor4(r_bloom_contrast_mult.GetFloat(), r_bloom_contrast_mult.GetFloat(), 1, 1);
			}
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, declManager->FindMaterial( "textures/AFX/AFXmodulate" ) );

			for( int i=0; i < r_bloom_blurIterations.GetInteger(); i++ ) {
				// Two pass gaussian filter					
				renderSystem->CaptureRenderToImage( "_fsfx_input" );
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, declManager->FindMaterial( blurxMaterial[blurMtrIndex] ) );
				renderSystem->CaptureRenderToImage( "_fsfx_input" );
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, declManager->FindMaterial( bluryMaterial[blurMtrIndex] ) );
			}

			renderSystem->CaptureRenderToImage( "_fsfx_input" );

			renderSystem->UnCrop();

			renderSystem->SetColor4( r_bloom_blur_mult.GetFloat(), r_bloom_src_mult.GetFloat(), 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, declManager->FindMaterial( "textures/AFX/AFXadd" ) );

			if ( !r_bloom_hud.GetBool() ) // If HUD blooming is disabled
			{
				player->DrawHUD(hud);
			}
		}

		ScreenFade();
	}

	if ( net_clientLagOMeter.GetBool() && lagoMaterial && gameLocal.isClient ) {
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
		renderSystem->DrawStretchPic( 10.0f, 380.0f, 64.0f, 64.0f, 0.0f, 0.0f, 1.0f, 1.0f, lagoMaterial );
	}

	// TDM Ambient Method checking. By Dram
	if ( cur_amb_method != cv_ambient_method.GetBool() ) // If the ambient method option has changed
	{
		UpdateAmbientLight();
	}
}

void idPlayerView::UpdateAmbientLight()
{
	// Looks for a light named "ambient_world"
	idEntity* ambient_light = gameLocal.FindEntity("ambient_world");
	
	if (ambient_light != NULL) // If the light exists
	{
		if (!cv_ambient_method.GetBool()) // If the Ambient Light method is used
		{
			gameLocal.globalShaderParms[2] = 0; // Set global shader parm 2 to 0
			gameLocal.globalShaderParms[3] = 0; // Set global shader parm 3 to 0
			gameLocal.globalShaderParms[4] = 0; // Set global shader parm 4 to 0

			if (ambient_light->IsType(idLight::Type))
			{
				static_cast<idLight*>(ambient_light)->On(); // Turn on ambient light
			}
		}
		else // If the Texture Brightness method is used
		{
			idVec3 ambient_color = ambient_light->spawnArgs.GetVector( "_color" ); // Get the ambient color from the spawn arguments
			gameLocal.globalShaderParms[2] = ambient_color.x * 1.5f; // Set global shader parm 2 to Red value of ambient light
			gameLocal.globalShaderParms[3] = ambient_color.y * 1.5f; // Set global shader parm 3 to Green value of ambient light
			gameLocal.globalShaderParms[4] = ambient_color.z * 1.5f; // Set global shader parm 4 to Blue value of ambient light

			if (ambient_light->IsType(idLight::Type))
			{
				static_cast<idLight*>(ambient_light)->Off(); // Turn off ambient light
			}
		}
	}
	else // The ambient light does not exist
	{
		gameLocal.Printf( "Note: Ambient light by name of 'ambient_world' not found"); // Show in console of light not existing in map
	}

	cur_amb_method = cv_ambient_method.GetBool(); // Set the current ambient method to the CVar value
}
