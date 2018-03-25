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

// Copyright (C) 2004 Id Software, Inc.
//
/*

  SecurityCamera.cpp

  Security camera that watches for the player

*/

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "StimResponse/StimResponseCollection.h"

/***********************************************************************

  idSecurityCamera
	
***********************************************************************/

// grayman #4615 - Refactored for 2.06

const idEventDef EV_SecurityCam_AddLight( "<addLight>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_SecurityCam_SpotLightToggle( "toggle_light", EventArgs(), EV_RETURNS_VOID, "Toggles the spotlight on/off." );
const idEventDef EV_SecurityCam_SweepToggle( "toggle_sweep", EventArgs(), EV_RETURNS_VOID, "Toggles the camera sweep." );

// Obsttorte
const idEventDef EV_SecurityCam_GetSpotLight("getSpotLight", EventArgs(), 'e', "Returns the spotlight used by the camera. Returns null_entity if none is used.");
CLASS_DECLARATION( idEntity, idSecurityCamera )
	EVENT( EV_SecurityCam_AddLight,			idSecurityCamera::Event_AddLight )
	EVENT( EV_SecurityCam_SpotLightToggle,	idSecurityCamera::Event_SpotLight_Toggle )
	EVENT( EV_SecurityCam_SweepToggle,		idSecurityCamera::Event_Sweep_Toggle)
	EVENT( EV_PostSpawn,					idSecurityCamera::PostSpawn )
	EVENT( EV_SecurityCam_GetSpotLight,		idSecurityCamera::Event_GetSpotLight)	
END_CLASS

#define ALERT_INTERVAL 5000 // time between alert sounds (ms)
#define PAUSE_SOUND_TIMING 500 // start sound prior to finishing sweep
#define SPARK_DELAY_BASE 3000  // base delay to next death spark
#define SPARK_DELAY_VARIANCE 2000 // randomize spark delay
#define SPARK_REMOVE_DELAY_FUTURE 20000 // temp assignment for when next to spawn sparks
#define SPARK_REMOVE_DELAY 1000 // sparks are visible for this duration

/*
================
idSecurityCamera::Save
================
*/
void idSecurityCamera::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( angle );
	savefile->WriteFloat( sweepAngle );
	savefile->WriteInt( modelAxis );
	savefile->WriteBool( flipAxis );
	savefile->WriteFloat( scanDist );
	savefile->WriteFloat( scanFov );
							
	savefile->WriteInt( sweepStartTime );
	savefile->WriteInt( sweepEndTime );
	savefile->WriteInt( nextSparkTime );
	savefile->WriteInt( removeSparkTime );
	savefile->WriteBool( negativeSweep );
	savefile->WriteBool( sweeping );
	savefile->WriteInt( alertMode );
	savefile->WriteFloat( scanFovCos );

	savefile->WriteVec3( viewOffset );
							
	savefile->WriteInt( pvsArea );
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteTraceModel( trm );

	savefile->WriteBool(rotate);
	savefile->WriteBool(stationary);
	savefile->WriteInt(nextAlertTime);
	savefile->WriteInt(state);
	savefile->WriteInt(startAlertTime);
	savefile->WriteBool(emitPauseSound);
	savefile->WriteInt(emitPauseSoundTime);
	savefile->WriteInt(pauseEndTime);
	savefile->WriteInt(endAlertTime);
	savefile->WriteInt(lostInterestEndTime);
	savefile->WriteFloat(percentSwept);
	spotLight.Save(savefile);
	sparks.Save(savefile);
	cameraDisplay.Save(savefile);
	savefile->WriteBool(powerOn);
	savefile->WriteBool(spotlightPowerOn);
}

/*
================
idSecurityCamera::Restore
================
*/
void idSecurityCamera::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( angle );
	savefile->ReadFloat( sweepAngle );
	savefile->ReadInt( modelAxis );
	savefile->ReadBool( flipAxis );
	savefile->ReadFloat( scanDist );
	savefile->ReadFloat( scanFov );
							
	savefile->ReadInt( sweepStartTime );
	savefile->ReadInt( sweepEndTime );
	savefile->ReadInt( nextSparkTime );
	savefile->ReadInt( removeSparkTime );
	savefile->ReadBool( negativeSweep );
	savefile->ReadBool( sweeping );
	savefile->ReadInt( alertMode );
	savefile->ReadFloat( scanFovCos );

	savefile->ReadVec3( viewOffset );
							
	savefile->ReadInt( pvsArea );
	savefile->ReadStaticObject( physicsObj );
	savefile->ReadTraceModel( trm );

	savefile->ReadBool(rotate);
	savefile->ReadBool(stationary);
	savefile->ReadInt(nextAlertTime);
	savefile->ReadInt(state);
	savefile->ReadInt(startAlertTime);
	savefile->ReadBool(emitPauseSound);
	savefile->ReadInt(emitPauseSoundTime);
	savefile->ReadInt(pauseEndTime);
	savefile->ReadInt(endAlertTime);
	savefile->ReadInt(lostInterestEndTime);
	savefile->ReadFloat(percentSwept);
	spotLight.Restore(savefile);
	sparks.Restore(savefile);
	cameraDisplay.Restore(savefile);
	savefile->ReadBool(powerOn);
	savefile->ReadBool(spotlightPowerOn);
}

/*
================
idSecurityCamera::Spawn
================
*/
void idSecurityCamera::Spawn( void )
{
	idStr	str;

	rotate		= spawnArgs.GetBool("rotate", "1");
	sweepAngle	= spawnArgs.GetFloat( "sweepAngle", "90" );
	health		= spawnArgs.GetInt( "health", "100" );
	scanFov		= spawnArgs.GetFloat( "scanFov", "90" );
	scanDist	= spawnArgs.GetFloat( "scanDist", "200" );
	flipAxis	= spawnArgs.GetBool( "flipAxis", "0" );
	stationary	= false;
	nextAlertTime = 0;
	nextSparkTime = 0;
	removeSparkTime = 0;
	state		  = STATE_SWEEPING;
	emitPauseSound = true;
	startAlertTime = 0;
	emitPauseSoundTime = 0;
	pauseEndTime = 0;
	endAlertTime = 0;
	lostInterestEndTime = 0;
	spotLight	= NULL;
	sparks = NULL;
	cameraDisplay = NULL;

	modelAxis	= spawnArgs.GetInt( "modelAxis", "0" );
	if ( modelAxis < 0 || modelAxis > 2 ) {
		modelAxis = 0;
	}

	spawnArgs.GetVector( "viewOffset", "0 0 0", viewOffset );

	if ( spawnArgs.GetBool( "spotLight", "0" ) ) {
		PostEventMS( &EV_SecurityCam_AddLight, 0 );
	}

	negativeSweep = ( sweepAngle < 0 ) ? true : false;
	sweepAngle = fabs( sweepAngle );

	scanFovCos = cos( scanFov * idMath::PI / 360.0f );

	angle = GetPhysics()->GetAxis().ToAngles().yaw;

	percentSwept = 0.0f;

	if ( rotate )
	{
		StartSweep();
	}
	else
	{
		StartSound( "snd_stationary", SND_CHANNEL_BODY, 0, false, NULL );
	}
	SetAlertMode( MODE_SCANNING );
	BecomeActive( TH_THINK | TH_UPDATEVISUALS );

	if ( health ) {
		fl.takedamage = true;
	}

	pvsArea = gameLocal.pvs.GetPVSArea( GetPhysics()->GetOrigin() );
	// if no target specified use ourself
	str = spawnArgs.GetString( "cameraTarget" );
	if ( str.Length() == 0 ) {
		spawnArgs.Set( "cameraTarget", spawnArgs.GetString( "name" ) );
	}

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", str );
	if ( !str[0] ) {
		str = spawnArgs.GetString( "model" );		// use the visual model
	}

	if ( !collisionModelManager->TrmFromModel( str, trm ) ) {
		gameLocal.Error( "idSecurityCamera '%s': cannot load collision model %s", name.c_str(), str.c_str() );
		return;
	}

	GetPhysics()->SetContents( CONTENTS_SOLID );
	// SR CONTENTS_RESPONSE FIX
	if( m_StimResponseColl->HasResponse() )
		physicsObj.SetContents( physicsObj.GetContents() | CONTENTS_RESPONSE );

	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
	// setup the physics
	UpdateChangeableSpawnArgs( NULL );

	powerOn = spawnArgs.GetBool("start_on", "1");
	spotlightPowerOn = spawnArgs.GetBool("spotlight", "1");

	// Schedule a post-spawn event to setup other spawnargs
	PostEventMS( &EV_PostSpawn, 1 );
}

/*
================
idSecurityCamera::PostSpawn
================
*/
void idSecurityCamera::PostSpawn()
{
	// Search entities for those who have a "cameraTarget" pointing to this camera.
	// One should be found, and set 'cameraDisplay' to that entity.

	for ( int i = 0; i < MAX_GENTITIES; ++i )
	{
		idEntity* ent = gameLocal.entities[i];
		if ( !ent )
		{
			continue;	// skip past nulls in the index
		}

		if ( ent == this )
		{
			continue;	// skip yourself
		}

		idEntity *ect = ent->cameraTarget;
		if ( ect )
		{
			if ( ect == this )
			{
				cameraDisplay = ent;
				break;
			}

			if ( cameraTarget == ect )
			{
				cameraDisplay = ent;
				break;
			}
		}
	}
}

/*
================
idSecurityCamera::Event_AddLight
================
*/
void idSecurityCamera::Event_AddLight( void )
{
	idDict	args;
	idVec3	lightOffset;
	idLight	*light;
	idVec3  cameraOrigin = GetPhysics()->GetOrigin();
	idVec3	lightColor;
	
	spawnArgs.GetVector( "lightOffset", "0 0 0", lightOffset );
	spawnArgs.GetVector("_color", "1 1 1", lightColor);

	// rotate the light origin offset around the z axis

	float angle_radians = angle*(idMath::PI / 180.0f);
	
	float a = lightOffset.x*idMath::Cos(angle_radians) - lightOffset.y*idMath::Sin(angle_radians);
	float b = lightOffset.x*idMath::Sin(angle_radians) + lightOffset.y*idMath::Cos(angle_radians);
	lightOffset = idVec3(a, b, lightOffset.z);

	// set target, right, up for the spotlight,
	// as if the light were pointing along the +x axis

	idVec3 target = idVec3(scanDist, 0, 0);
	idVec3 right = idVec3(0, -scanDist / 2.0f, 0);
	idVec3 up = idVec3(0, 0, scanDist / 2.0f);

	args.Set( "origin", ( cameraOrigin + lightOffset ).ToString() );
	args.Set( "light_target", target.ToString() );
	args.Set( "light_right", right.ToString() );
	args.Set( "light_up", up.ToString() );
	args.SetFloat( "angle", angle );
	args.Set("texture", "lights/biground1");
	args.Set("_color", lightColor.ToString());

	light = static_cast<idLight *>( gameLocal.SpawnEntityType( idLight::Type, &args ) );
	light->Bind( this, true );
	spotLight = light;
	light->UpdateVisuals();
}

/*
================
idSecurityCamera::Event_SpotLight_Toggle
================
*/
void idSecurityCamera::Event_SpotLight_Toggle(void)
{
	idLight* light = spotLight.GetEntity();
	if ( light == NULL )
	{
		return; // no spotlight was defined; nothing to do
	}

	// toggle the spotlight

	spotlightPowerOn = !spotlightPowerOn;

	if ( powerOn )
	{
		if ( spotlightPowerOn )
		{
			light->On();
			Event_SetSkin("security_camera_on");
		}
		else
		{
			light->Off();
			Event_SetSkin("security_camera_on_spotlight_off");
		}
	}
}

/*
================
idSecurityCamera::Event_GetSpotLight
================
*/
void idSecurityCamera::Event_GetSpotLight()
{
	idLight* light = spotLight.GetEntity();
	if (light == NULL)
	{
		idThread::ReturnEntity(NULL);
	}
	else
	{
		idThread::ReturnEntity(light);
	}
}

/*
================
idSecurityCamera::DrawFov
================
*/
void idSecurityCamera::DrawFov( void ) {
	int i;
	float radius, a, s, c, halfRadius;
	idVec3 right, up;
	idVec4 color(1, 0, 0, 1), color2(0, 0, 1, 1);
	idVec3 lastPoint, point, lastHalfPoint, halfPoint, center;

	idVec3 dir = GetAxis();
	dir.NormalVectors( right, up );

	radius = tan( scanFov * idMath::PI / 360.0f );
	halfRadius = radius * 0.5f;
	lastPoint = dir + up * radius;
	lastPoint.Normalize();
	lastPoint = GetPhysics()->GetOrigin() + lastPoint * scanDist;
	lastHalfPoint = dir + up * halfRadius;
	lastHalfPoint.Normalize();
	lastHalfPoint = GetPhysics()->GetOrigin() + lastHalfPoint * scanDist;
	center = GetPhysics()->GetOrigin() + dir * scanDist;
	for ( i = 1; i < 12; i++ ) {
		a = idMath::TWO_PI * i / 12.0f;
		idMath::SinCos( a, s, c );
		point = dir + right * s * radius + up * c * radius;
		point.Normalize();
		point = GetPhysics()->GetOrigin() + point * scanDist;
		gameRenderWorld->DebugLine( color, lastPoint, point );
		gameRenderWorld->DebugLine( color, GetPhysics()->GetOrigin(), point );
		lastPoint = point;

		halfPoint = dir + right * s * halfRadius + up * c * halfRadius;
		halfPoint.Normalize();
		halfPoint = GetPhysics()->GetOrigin() + halfPoint * scanDist;
		gameRenderWorld->DebugLine( color2, point, halfPoint );
		gameRenderWorld->DebugLine( color2, lastHalfPoint, halfPoint );
		lastHalfPoint = halfPoint;

		gameRenderWorld->DebugLine( color2, halfPoint, center );
	}
}

/*
================
idSecurityCamera::GetRenderView
================
*/
renderView_t *idSecurityCamera::GetRenderView() {
	renderView_t *rv = idEntity::GetRenderView();
	rv->fov_x = scanFov;
	rv->fov_y = scanFov;
	rv->viewaxis = GetAxis().ToAngles().ToMat3();
	idVec3 forward = GetAxis().ToAngles().ToForward(); // vector along forward sightline
	rv->vieworg = GetPhysics()->GetOrigin() + viewOffset.LengthFast()*forward;
	return rv;
}

/*
================
idSecurityCamera::GetCalibratedLightgemValue
================
*/
float idSecurityCamera::GetCalibratedLightgemValue(idPlayer* player)
{
	float lgem = static_cast<float>(player->GetCurrentLightgemValue());
	float term0 = -0.03f;
	float term1 = 0.03f * lgem;
	float term2 = 0.001f * idMath::Pow16(lgem, 2);
	float term3 = 0.00013f * idMath::Pow16(lgem, 3);
	float term4 = -0.000011f * idMath::Pow16(lgem, 4);
	float term5 = 0.0000001892f * idMath::Pow16(lgem, 5);
	float clampVal = term0 + term1 + term2 + term3 + term4 + term5;
	return clampVal;
}

/*
================
idSecurityCamera::IsEntityHiddenByDarkness
================
*/

bool idSecurityCamera::IsEntityHiddenByDarkness(idPlayer* player, const float sightThreshold)
{
	// Quick test using LAS at entity origin
	idPhysics* p_physics = player->GetPhysics();

	if (p_physics == NULL) 
	{
		return false; // Not in darkness
	}

	// Use lightgem
		
	// greebo: Check the visibility of the player depending on lgem and distance
	float visFraction = GetCalibratedLightgemValue(player); // returns values in [0..1]

	// Very low threshold for visibility
	if (visFraction < sightThreshold)
	{
		// Not visible, entity is hidden in darkness
		return true;
	}

	// Visible, visual stim above threshold
	return false;
}

/*
================
idSecurityCamera::CanSeePlayer
================
*/
bool idSecurityCamera::CanSeePlayer( void )
{
	int i;
	float dist;
	idPlayer *ent;
	trace_t tr;
	idVec3 dir;
	pvsHandle_t handle;

	handle = gameLocal.pvs.SetupCurrentPVS( pvsArea );

	for ( i = 0; i < gameLocal.numClients; i++ ) {
		ent = static_cast<idPlayer*>(gameLocal.entities[ i ]);

		if ( !spawnArgs.GetBool("seePlayer", "1") ) // does this camera react to the player?
		{
			continue;
		}

		if ( !ent || ent->fl.notarget || ent->fl.invisible )
		{
			continue;
		}

		// if there is no way we can see this player
		if ( !gameLocal.pvs.InCurrentPVS( handle, ent->GetPVSAreas(), ent->GetNumPVSAreas() ) ) {
			continue;
		}

		dir = ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		dist = dir.Normalize();

		if ( dist > scanDist ) {
			continue;
		}

		if ( dir * GetAxis() < scanFovCos ) {
			continue;
		}

		// take lighting into account
		if (IsEntityHiddenByDarkness(ent, 0.1f))
		{
			continue;
		}

		idVec3 eye = ent->EyeOffset();

		gameLocal.clip.TracePoint( tr, GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin() + eye, MASK_OPAQUE, this );
		if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == ent ) ) {
			gameLocal.pvs.FreeCurrentPVS( handle );
			return true;
		}
	}

	gameLocal.pvs.FreeCurrentPVS( handle );

	return false;
}

/*
================
idSecurityCamera::SetAlertMode
================
*/
void idSecurityCamera::SetAlertMode( int alert ) {
	if (alert >= MODE_SCANNING && alert <= MODE_ALERT) {
		alertMode = alert;
	}
	renderEntity.shaderParms[ SHADERPARM_MODE ] = alertMode;
	UpdateVisuals();
}

/*
================
idSecurityCamera::AddSparks
================
*/
void idSecurityCamera::AddSparks( void )
{
	if ( !powerOn )
	{
		sparks = NULL; // no sparks if there's no power
		return;
	}

	// Create sparks

	idEntity *sparkEntity;
	idDict args;

	args.Set("classname","func_emitter");
	args.Set("origin", GetPhysics()->GetOrigin().ToString());
	args.Set("model","sparks_wires.prt");
	gameLocal.SpawnEntityDef( args, &sparkEntity );
	sparks = sparkEntity;
	sparkEntity->Show();

	nextSparkTime = gameLocal.time + SPARK_DELAY_BASE + gameLocal.random.RandomInt(SPARK_DELAY_VARIANCE);
	removeSparkTime = gameLocal.time + SPARK_REMOVE_DELAY;
}

/*
================
idSecurityCamera::Think
================
*/
void idSecurityCamera::Think( void )
{
	float travel;

	if ( thinkFlags & TH_THINK )
	{
		if ( g_showEntityInfo.GetBool() )
		{
			DrawFov();
		}

		if (health <= 0)
		{
			BecomeInactive( TH_THINK );
			return;
		}
	}

	// run physics
	RunPhysics();

	if ( state == STATE_DEAD )
	{
		// check if it's time to remove the sparks

		if (sparks.GetEntity() && ( gameLocal.time >= removeSparkTime ))
		{
			sparks.GetEntity()->PostEventMS( &EV_SafeRemove, 0 );
			sparks = NULL;
			removeSparkTime = gameLocal.time + SPARK_REMOVE_DELAY_FUTURE; // far in the future
		}

		// handle electric sparking sound

		if (powerOn && ( gameLocal.time >= nextSparkTime ))
		{
			StopSound(SND_CHANNEL_ANY, false);
			StartSound("snd_sparks", SND_CHANNEL_BODY, 0, false, NULL);
			AddSparks(); // Create sparks
		}
	}

	if ( thinkFlags & TH_THINK )
	{
		switch ( state )
		{
		case STATE_SWEEPING:
			if ( CanSeePlayer() )
			{
				StopSound(SND_CHANNEL_ANY, false);
				StartSound("snd_sight", SND_CHANNEL_BODY, 0, false, NULL);
				float sightTime = spawnArgs.GetFloat("sightTime", "5");
				startAlertTime = gameLocal.time + SEC2MS(sightTime);
				sweeping = false;
				state = STATE_PLAYERSIGHTED;
				SetAlertMode(MODE_SIGHTED);
			}
			else if ( rotate && !stationary )
			{
				// snd_end is played once, just before a pause begins
				if ( emitPauseSound && (gameLocal.time >= emitPauseSoundTime) )
				{
					StopSound(SND_CHANNEL_ANY, false);
					StartSound("snd_end", SND_CHANNEL_BODY, 0, false, NULL);
					emitPauseSound = false;
				}

				if ( gameLocal.time >= sweepEndTime )
				{
					pauseEndTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat("sweepWait", "0.5"));
					sweeping = false;
					state = STATE_PAUSED;
				}
			}
			break;
		case STATE_PLAYERSIGHTED:
			if ( gameLocal.time >= startAlertTime )
			{
				if ( CanSeePlayer() )
				{
					StopSound(SND_CHANNEL_ANY, false);
					StartSound("snd_alert", SND_CHANNEL_BODY, 0, false, NULL);
					nextAlertTime = gameLocal.time + ALERT_INTERVAL;
					endAlertTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat("wait", "20"));
					SetAlertMode(MODE_ALERT);
					ActivateTargets(this);
					state = STATE_ALERTED;
				}
				else
				{
					SetAlertMode(MODE_LOSINGINTEREST);
					lostInterestEndTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat("sightResume", "1.5"));
					state = STATE_LOSTINTEREST;
				}
			}
			break;
		case STATE_LOSTINTEREST:
			if ( gameLocal.time >= lostInterestEndTime )
			{
				if ( rotate && !stationary )
				{
					ContinueSweep(); // changes state to STATE_SWEEPING
				}
				else
				{
					StopSound( SND_CHANNEL_ANY, false );
					StartSound( "snd_stationary", SND_CHANNEL_BODY, 0, false, NULL );
					SetAlertMode(MODE_SCANNING);
					sweeping = false;
					stationary = true;
					state = STATE_SWEEPING;
				}
			}
			break;
		case STATE_ALERTED:
			if ( gameLocal.time < endAlertTime )
			{
				// is it time to sound the alert again?

				if ( gameLocal.time >= nextAlertTime )
				{
					nextAlertTime = gameLocal.time + ALERT_INTERVAL;
					StopSound(SND_CHANNEL_ANY, false);
					StartSound("snd_alert", SND_CHANNEL_BODY, 0, false, NULL);
				}
			}
			else
			{
				if ( rotate && !stationary )
				{
					ContinueSweep(); // changes state to STATE_SWEEPING
				}
				else
				{
					StopSound( SND_CHANNEL_ANY, false );
					StartSound( "snd_stationary", SND_CHANNEL_BODY, 0, false, NULL );
					SetAlertMode(MODE_SCANNING);
					sweeping = false;
					stationary = true;
					state = STATE_SWEEPING;
				}
			}
			break;
		case STATE_POWERRETURNS_SWEEPING:
			if ( rotate )
			{
				ContinueSweep(); // changes state to STATE_SWEEPING
			}
			else
			{
				StartSound( "snd_stationary", SND_CHANNEL_BODY, 0, false, NULL );
				state = STATE_SWEEPING;
			}
			break;
		case STATE_POWERRETURNS_PAUSED:
			if ( rotate )
			{
				ReverseSweep(); // changes state to STATE_SWEEPING
			}
			else
			{
				StartSound( "snd_stationary", SND_CHANNEL_BODY, 0, false, NULL );
				state = STATE_SWEEPING;
			}
			break;
		case STATE_PAUSED:
			if ( gameLocal.time >= pauseEndTime )
			{
				if ( rotate && !stationary )
				{
					ReverseSweep(); // changes state to STATE_SWEEPING
				}
			}
			break;
		case STATE_DEAD:
			break;
		}

		if ( rotate && sweeping )
		{
			idAngles a = GetPhysics()->GetAxis().ToAngles();

			percentSwept = (float)(gameLocal.time - sweepStartTime) / (float)(sweepEndTime - sweepStartTime);
			travel = percentSwept * sweepAngle;
			if ( negativeSweep ) {
				a.yaw = angle + travel;
			}
			else {
				a.yaw = angle - travel;
			}

			SetAngles(a);
		}
	}
	Present();
}

/*
================
idSecurityCamera::GetAxis
================
*/
const idVec3 idSecurityCamera::GetAxis( void ) const {
	return (flipAxis) ? -GetPhysics()->GetAxis()[modelAxis] : GetPhysics()->GetAxis()[modelAxis];
};

/*
================
idSecurityCamera::SweepTime
================
*/
float idSecurityCamera::SweepTime( void ) const {
	return spawnArgs.GetFloat( "sweepSpeed", "5" );
}

/*
================
idSecurityCamera::StartSweep
================
*/
void idSecurityCamera::StartSweep( void ) {
	int sweepTime;

	sweeping = true;
	sweepStartTime = gameLocal.time;
	sweepTime = SEC2MS( SweepTime() );
	sweepEndTime = sweepStartTime + sweepTime;
	emitPauseSoundTime = sweepEndTime - PAUSE_SOUND_TIMING;
	StartSound( "snd_moving", SND_CHANNEL_BODY, 0, false, NULL );
	emitPauseSound = true;
	state = STATE_SWEEPING;
}

/*
================
idSecurityCamera::ContinueSweep
================
*/
void idSecurityCamera::ContinueSweep( void )
{
	if ( !rotate || stationary )
	{
		SetAlertMode(MODE_SCANNING);
		return;
	}

	int sweepTime = static_cast<int>(SEC2MS( SweepTime() ));
	int timeRemaining = static_cast<int>((1.0f - percentSwept)*sweepTime);
	sweepEndTime = gameLocal.time + timeRemaining;
	sweepStartTime = sweepEndTime - sweepTime; // represents start time if the sweep hadn't been interrupted
	emitPauseSoundTime = sweepEndTime - PAUSE_SOUND_TIMING;
	StopSound( SND_CHANNEL_ANY, false );
	StartSound( "snd_moving", SND_CHANNEL_BODY, 0, false, NULL );
	SetAlertMode(MODE_SCANNING);
	sweeping = true;
	stationary = false;
	state = STATE_SWEEPING;
}

/*
================
idSecurityCamera::ReverseSweep
================
*/
void idSecurityCamera::ReverseSweep( void ) {
	angle = GetPhysics()->GetAxis().ToAngles().yaw;
	negativeSweep = !negativeSweep;
	StartSweep();
}

/*
============
idSecurityCamera::Killed
============
*/
void idSecurityCamera::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	sweeping = false;
	StopSound( SND_CHANNEL_ANY, false );
	StartSound("snd_death", SND_CHANNEL_BODY, 0, false, NULL);
	const char *fx = spawnArgs.GetString( "fx_destroyed" );
	if ( fx[0] != '\0' )
	{
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
	}

	// call base class method to switch to broken model
	idEntity::BecomeBroken( inflictor );

	// Remove a spotlight, if there is one.

	idLight* light = spotLight.GetEntity();
	if ( light )
	{
		light->PostEventMS( &EV_Remove, 0 );
	}

	// Turn off the display screen

	if ( cameraDisplay.GetEntity() )
	{
		cameraDisplay.GetEntity()->Hide();
	}

	AddSparks(); // Create sparks
	state = STATE_DEAD;
	BecomeActive(TH_UPDATEPARTICLES); // keeps stationary camera thinking to display sparks
}


/*
============
idSecurityCamera::Pain
============
*/
bool idSecurityCamera::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	const char *fx = spawnArgs.GetString( "fx_damage" );
	if ( fx[0] != '\0' ) {
		idEntityFx::StartFx( fx, NULL, NULL, this, true );
	}
	return true;
}

/*
================
idSecurityCamera::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/

void idSecurityCamera::Present( void ) 
{
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}

	if ( cameraTarget )
	{
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
	}

	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() ) {
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}

/*
================
idSecurityCamera::Activate - turn camera power on/off
================
*/
void idSecurityCamera::Activate(idEntity* activator)
{
	powerOn = !powerOn;
	
	if ( state == STATE_DEAD )
	{
		if (powerOn && ( sparks.GetEntity() == NULL ))
		{
			AddSparks();
		}
		return;
	}

	idLight* light = spotLight.GetEntity();

	if ( light )
	{
		if ( light->GetLightLevel() > 0 )
		{
			if ( !powerOn )
			{
				light->Off();
			}
		}
		else // spotlight is off
		{
			if ( powerOn && spotlightPowerOn )
			{
				light->On();
			}
		}
	}

	if ( powerOn )
	{
		BecomeActive(TH_THINK);
		switch ( state )
		{
		case STATE_SWEEPING:
			state = STATE_POWERRETURNS_SWEEPING;
			break;
		case STATE_PLAYERSIGHTED:
		case STATE_ALERTED:
			state = STATE_LOSTINTEREST;
		case STATE_LOSTINTEREST:
			lostInterestEndTime = gameLocal.time;
			break;
		case STATE_PAUSED:
			state = STATE_POWERRETURNS_PAUSED;
			break;
		case STATE_DEAD:
			break;
		}
	}
	else
	{
		StopSound(SND_CHANNEL_ANY, false);
		BecomeInactive(TH_THINK);
	}

	// set skin

	if ( powerOn )
	{
		if ( light && spotlightPowerOn )
		{
			Event_SetSkin("security_camera_on"); // change skin
		}
		else
		{
			Event_SetSkin("security_camera_on_spotlight_off"); // change skin
		}
	}
	else
	{
		Event_SetSkin("security_camera_off"); // change skin
	}

	// Toggle display screen

	if ( cameraDisplay.GetEntity() )
	{
		if ( powerOn )
		{
			cameraDisplay.GetEntity()->Show();
		}
		else
		{
			cameraDisplay.GetEntity()->Hide();
		}
	}
}

/*
================
idSecurityCamera::Event_Sweep_Toggle
================
*/
void idSecurityCamera::Event_Sweep_Toggle( void )
{
	stationary = !stationary;
	switch(state)
	{
	case STATE_SWEEPING:
		if ( stationary )
		{
			sweeping = false;
			StopSound(SND_CHANNEL_ANY, false);
			StartSound("snd_end", SND_CHANNEL_BODY, 0, false, NULL);
		}
		else
		{
			ContinueSweep(); // changes state to STATE_SWEEPING
		}
		break;
	case STATE_PLAYERSIGHTED:
	case STATE_LOSTINTEREST:
	case STATE_ALERTED:
	case STATE_DEAD:
		break;
	case STATE_PAUSED:
		if ( !stationary )
		{
			ReverseSweep(); // changes state to STATE_SWEEPING
		}
		break;
	}
}

