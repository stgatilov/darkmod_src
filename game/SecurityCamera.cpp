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
const idEventDef EV_Peek_AddDisplay("<addDisplay>", EventArgs(), EV_RETURNS_VOID, "internal"); // grayman #4882
const idEventDef EV_SecurityCam_GetSpotLight("getSpotLight", EventArgs(), 'e', "Returns the spotlight used by the camera. Returns null_entity if none is used.");
const idEventDef EV_SecurityCam_GetSecurityCameraState("getSecurityCameraState", EventArgs(), 'f', "Returns the security camera's state. 1 = unalerted, 2 = suspicious, 3 = fully alerted, 4 = inactive, 5 = destroyed.");

CLASS_DECLARATION( idEntity, idSecurityCamera )
	EVENT( EV_SecurityCam_AddLight,			idSecurityCamera::Event_AddLight )
	EVENT( EV_SecurityCam_SpotLightToggle,		idSecurityCamera::Event_SpotLight_Toggle )
	EVENT( EV_SecurityCam_SweepToggle,		idSecurityCamera::Event_Sweep_Toggle)
	EVENT( EV_PostSpawn,				idSecurityCamera::PostSpawn )
	EVENT( EV_SecurityCam_GetSpotLight,		idSecurityCamera::Event_GetSpotLight)	
	EVENT( EV_SecurityCam_GetSecurityCameraState,	idSecurityCamera::Event_GetSecurityCameraState)	
	END_CLASS

#define PAUSE_SOUND_TIMING 500 // start sound prior to finishing sweep

/*
================
idSecurityCamera::Save
================
*/
void idSecurityCamera::Save( idSaveGame *savefile ) const {
	savefile->WriteBool(rotate);
	savefile->WriteBool(stationary);
	savefile->WriteBool(sweeping);

	savefile->WriteBool(follow);
	savefile->WriteFloat(followSpeedMult);
	savefile->WriteFloat(followTolerance);
	savefile->WriteBool(followIncline);
	savefile->WriteFloat(followInclineTolerance);

	savefile->WriteFloat(sweepAngle);
	savefile->WriteFloat(sweepTime);
	savefile->WriteFloat(sweepSpeed);
	savefile->WriteFloat(sweepStartTime);
	savefile->WriteFloat(sweepEndTime);
	savefile->WriteFloat(percentSwept);
	savefile->WriteBool(negativeSweep);

	savefile->WriteFloat(angle);
	savefile->WriteFloat(angleTarget);
	savefile->WriteFloat(anglePos1);
	savefile->WriteFloat(anglePos2);
	savefile->WriteFloat(angleToPlayer);

	savefile->WriteFloat(inclineAngle);
	savefile->WriteFloat(inclineSpeed);
	savefile->WriteFloat(inclineStartTime);
	savefile->WriteFloat(inclineEndTime);
	savefile->WriteFloat(percentInclined);
	savefile->WriteBool(negativeIncline);

	savefile->WriteFloat(incline);
	savefile->WriteFloat(inclineTarget);
	savefile->WriteFloat(inclinePos1);
	savefile->WriteFloat(inclineToPlayer);

	savefile->WriteFloat(timeLastSeen);
	savefile->WriteFloat(alarm_duration);

	savefile->WriteFloat(scanDist);
	savefile->WriteFloat(scanFov);
	savefile->WriteFloat(scanFovCos);
	savefile->WriteFloat(sightThreshold);

	savefile->WriteInt(modelAxis);
	savefile->WriteBool(flipAxis);
	savefile->WriteVec3(viewOffset);

	savefile->WriteInt(pvsArea);
	savefile->WriteStaticObject(physicsObj);
	savefile->WriteTraceModel(trm);

	spotLight.Save(savefile);
	sparks.Save(savefile);
	cameraDisplay.Save(savefile);

	savefile->WriteInt(state);
	savefile->WriteInt(alertMode);
	savefile->WriteBool(powerOn);
	savefile->WriteBool(spotlightPowerOn);

	savefile->WriteFloat(lostInterestEndTime);
	savefile->WriteFloat(nextAlertTime);
	savefile->WriteFloat(startAlertTime);
	savefile->WriteFloat(endAlertTime);
	savefile->WriteBool(emitPauseSound);
	savefile->WriteFloat(emitPauseSoundTime);
	savefile->WriteFloat(pauseEndTime);
	savefile->WriteFloat(nextSparkTime);

	savefile->WriteBool(sparksOn);
	savefile->WriteBool(sparksPowerDependent);
	savefile->WriteBool(sparksPeriodic);
	savefile->WriteFloat(sparksInterval);
	savefile->WriteFloat(sparksIntervalRand);

	savefile->WriteBool(useColors);
	savefile->WriteVec3(colorSweeping);
	savefile->WriteVec3(colorSighted);
	savefile->WriteVec3(colorAlerted);
}

/*
================
idSecurityCamera::Restore
================
*/
void idSecurityCamera::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool(rotate);
	savefile->ReadBool(stationary);
	savefile->ReadBool(sweeping);

	savefile->ReadBool(follow);
	savefile->ReadFloat(followSpeedMult);
	savefile->ReadFloat(followTolerance);
	savefile->ReadBool(followIncline);
	savefile->ReadFloat(followInclineTolerance);
	   
	savefile->ReadFloat(sweepAngle);
	savefile->ReadFloat(sweepTime);
	savefile->ReadFloat(sweepSpeed);
	savefile->ReadFloat(sweepStartTime);
	savefile->ReadFloat(sweepEndTime);
	savefile->ReadFloat(percentSwept);
	savefile->ReadBool(negativeSweep);

	savefile->ReadFloat(angle);
	savefile->ReadFloat(angleTarget);
	savefile->ReadFloat(anglePos1);
	savefile->ReadFloat(anglePos2);
	savefile->ReadFloat(angleToPlayer);

	savefile->ReadFloat(inclineAngle);
	savefile->ReadFloat(inclineSpeed);
	savefile->ReadFloat(inclineStartTime);
	savefile->ReadFloat(inclineEndTime);
	savefile->ReadFloat(percentInclined);
	savefile->ReadBool(negativeIncline);

	savefile->ReadFloat(incline);
	savefile->ReadFloat(inclineTarget);
	savefile->ReadFloat(inclinePos1);
	savefile->ReadFloat(inclineToPlayer);

	savefile->ReadFloat(timeLastSeen);
	savefile->ReadFloat(alarm_duration);

	savefile->ReadFloat(scanDist);
	savefile->ReadFloat(scanFov);
	savefile->ReadFloat(scanFovCos);
	savefile->ReadFloat(sightThreshold);

	savefile->ReadInt(modelAxis);
	savefile->ReadBool(flipAxis);
	savefile->ReadVec3(viewOffset);

	savefile->ReadInt(pvsArea);
	savefile->ReadStaticObject(physicsObj);
	savefile->ReadTraceModel(trm);

	spotLight.Restore(savefile);
	sparks.Restore(savefile);
	cameraDisplay.Restore(savefile);

	savefile->ReadInt(state);
	savefile->ReadInt(alertMode);
	savefile->ReadBool(powerOn);
	savefile->ReadBool(spotlightPowerOn);

	savefile->ReadFloat(lostInterestEndTime);
	savefile->ReadFloat(nextAlertTime);
	savefile->ReadFloat(startAlertTime);
	savefile->ReadFloat(endAlertTime);
	savefile->ReadBool(emitPauseSound);
	savefile->ReadFloat(emitPauseSoundTime);
	savefile->ReadFloat(pauseEndTime);
	savefile->ReadFloat(nextSparkTime);

	savefile->ReadBool(sparksOn);
	savefile->ReadBool(sparksPowerDependent);
	savefile->ReadBool(sparksPeriodic);
	savefile->ReadFloat(sparksInterval);
	savefile->ReadFloat(sparksIntervalRand);

	savefile->ReadBool(useColors);
	savefile->ReadVec3(colorSweeping);
	savefile->ReadVec3(colorSighted);
	savefile->ReadVec3(colorAlerted);
}

/*
================
idSecurityCamera::Spawn
================
*/
void idSecurityCamera::Spawn( void )
{
	idStr	str;

	rotate			= spawnArgs.GetBool("rotate", "1");
	sweepAngle		= spawnArgs.GetFloat( "sweepAngle", "90" );
	sweepTime		= spawnArgs.GetFloat( "sweepTime", "5" );
	health			= spawnArgs.GetInt( "health", "100" );
	scanFov			= spawnArgs.GetFloat( "scanFov", "90" );
	scanDist		= spawnArgs.GetFloat( "scanDist", "200" );
	flipAxis		= spawnArgs.GetBool( "flipAxis", "0" );
	useColors		= spawnArgs.GetBool("useColors");
	colorSweeping	= spawnArgs.GetVector("color_sweeping", "0.3 0.7 0.4");
	colorSighted	= spawnArgs.GetVector("color_sighted", "0.7 0.7 0.3");
	colorAlerted	= spawnArgs.GetVector("color_alerted", "0.7 0.3 0.3");
	sparksPowerDependent	= spawnArgs.GetBool("sparks_power_dependent", "1");
	sparksPeriodic			= spawnArgs.GetBool("sparks_periodic", "1");
	sparksInterval			= spawnArgs.GetFloat("sparks_interval", "3");
	sparksIntervalRand		= spawnArgs.GetFloat("sparks_interval_rand", "2");
	sightThreshold			= spawnArgs.GetFloat("sight_threshold", "0.1");
	follow					= spawnArgs.GetBool("follow", "0");
	followIncline			= spawnArgs.GetBool("follow_incline", "0");
	followTolerance			= spawnArgs.GetFloat("follow_tolerance", "15");
	followInclineTolerance	= spawnArgs.GetFloat("follow_incline_tolerance", "10");
	followSpeedMult = 0;
	state	= STATE_SWEEPING;
	sweeping = false;
	following = false;
	sparksOn = false;
	stationary	= false;
	nextAlertTime = 0;
	sweepStartTime = sweepEndTime = 0;
	inclineStartTime = inclineEndTime = 0;
	nextSparkTime = 0;
	emitPauseSound = true;
	startAlertTime = 0;
	emitPauseSoundTime = 0;
	pauseEndTime = 0;
	endAlertTime = 0;
	lostInterestEndTime = 0;
	timeLastSeen = 0;
	spotLight	= NULL;
	sparks = NULL;
	cameraDisplay = NULL;

	modelAxis	= spawnArgs.GetInt( "modelAxis", "0" );
	if ( modelAxis < 0 || modelAxis > 2 ) {
		modelAxis = 0;
	}

	spawnArgs.GetVector( "viewOffset", "0 0 0", viewOffset );

	if ( spawnArgs.GetBool( "spotLight", "0" ) ) {
			PostEventMS(&EV_SecurityCam_AddLight, 0);
	}

	//Use scanFov if cameraFovX and cameraFovY are not set
	if ( !spawnArgs.GetInt("cameraFovX", "0") )	{
		cameraFovX = scanFov;
	}
	if ( !spawnArgs.GetInt("cameraFovY", "0") )	{
		cameraFovY = scanFov;
	}

	//check if this is an old entity that still uses sweepSpeed as if it was sweepTime
	if (spawnArgs.GetFloat("sweepSpeed", "0") > 0) {
		sweepTime = spawnArgs.GetFloat("sweepSpeed", "5");
	}

	//if "alarm_duration" is not set, use "wait" instead
	alarm_duration = spawnArgs.GetFloat("alarm_duration", "0");
	if (alarm_duration == 0) {
		alarm_duration = spawnArgs.GetFloat("wait", "20");
	}

	scanFovCos = cos( scanFov * idMath::PI / 360.0f );

	//yaw angle
	angle		= anglePos1 = GetPhysics()->GetAxis().ToAngles().yaw;
	angleTarget	= anglePos2 = idMath::AngleNormalize180(angle - sweepAngle);
	angleToPlayer = 0;

	negativeSweep = (sweepAngle < 0) ? true : false;
	sweepAngle = fabs(sweepAngle);
	sweepSpeed = sweepAngle / sweepTime;
	percentSwept = 0.0f;

	//pitch angle
	incline	= inclinePos1 = GetPhysics()->GetAxis().ToAngles().pitch;
	inclineTarget	= inclineToPlayer = 0;

	negativeIncline = false;
	inclineAngle = 0;
	inclineSpeed = spawnArgs.GetFloat("follow_incline_speed", "30");
	percentInclined = 0.0f;

	powerOn = !spawnArgs.GetBool("start_off", "0");
	spotlightPowerOn = true;

	if ( powerOn ) {
		if ( rotate )
		{
			StartSweep();
			Event_SetSkin(spawnArgs.GetString("skin_on", "security_camera_on"));
		}
		else
		{
			StartSound("snd_stationary", SND_CHANNEL_BODY, 0, false, NULL);
			Event_SetSkin(spawnArgs.GetString("skin_off", "security_camera_off"));
		}

		BecomeActive( TH_THINK | TH_UPDATEVISUALS );
	}

	//sets initial shaderParms and color
	SetAlertMode(MODE_SCANNING);
	if ( useColors ) {
		Event_SetColor(colorSweeping[0], colorSweeping[1], colorSweeping[2]);
	}

	fl.takedamage = ( health > 0 ) ? true : false;
	
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

	memset(&trm, 0, sizeof(trm));	//stgatilov: uninitialized member warning
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
				ect->cameraFovX = cameraFovX;
				ect->cameraFovY = cameraFovY;
				break;
			}
		}
	}

	//If no power, toggle self, spotlight and camera display off
	if (!powerOn) {
		powerOn = true;		//make sure powerOn is false again after Activate() toggles it
		Activate(NULL);
	}
}

/*
================
idSecurityCamera::Event_AddLight
================
*/
void idSecurityCamera::Event_AddLight( void )
{
	//Check whether the mapper has specified a valid custom spotlight
	idLight	*light;
	idStr str;
	
	str = spawnArgs.GetString("spotlight_custom", "");
	light = static_cast<idLight *>( gameLocal.FindEntity(str) );

	if ( light && light->IsType(idLight::Type) ) {
		spotLight = light;
		if (powerOn)	light->On();
		else			light->Off();
	}

	//Otherwise spawn a spotlight
	else {
		idDict	args;
		idVec3	lightOffset;
		idVec3	lightColor;
		idVec3  cameraOrigin = GetPhysics()->GetOrigin();
		idStr	spotlightTexture;
		float	spotlightRange;
		float	spotlightDiameter;
		idVec3	target;
		idVec3	right;
		idVec3	up;

		spawnArgs.GetVector("lightOffset", "0 0 0", lightOffset);
		spawnArgs.GetVector("_color", "1 1 1", lightColor);
		spawnArgs.GetString("spotlight_texture", "lights/biground1", spotlightTexture);
		spawnArgs.GetFloat("spotlight_range", "0", spotlightRange);
		spawnArgs.GetFloat("spotlight_diameter", "0", spotlightDiameter);

		//if neither range nor diameter were set (old entity), use scanDist for both
		if ( spotlightRange == 0 && spotlightDiameter == 0 )
		{
			spotlightRange = scanDist;
			spotlightDiameter = scanDist / 2.0f;
		}

		//if only one was not set, find which one
		else
		{
			if (spotlightRange == 0) {
				spotlightRange = scanDist;
			}
			//automatically calculate diameter to match range & scanFov
			if ( spotlightDiameter == 0 ) {
				if ( scanFov > 90 ) spotlightDiameter = 1.5f * spotlightRange;
				else				spotlightDiameter = 1.5f * spotlightRange * idMath::Tan( DEG2RAD(scanFov / 2) );
			}
		}

		// rotate the light origin offset around the z axis

		float angle_radians = angle * (idMath::PI / 180.0f);

		float a = lightOffset.x*idMath::Cos(angle_radians) - lightOffset.y*idMath::Sin(angle_radians);
		float b = lightOffset.x*idMath::Sin(angle_radians) + lightOffset.y*idMath::Cos(angle_radians);
		lightOffset = idVec3(a, b, lightOffset.z);

		// set target, right, up for the spotlight,
		// as if the light were pointing along the +x axis
		target = idVec3(spotlightRange, 0, 0);
		right = idVec3(0, -spotlightDiameter, 0);
		up = idVec3(0, 0, spotlightDiameter);

		args.Set("origin", (cameraOrigin + lightOffset).ToString());
		args.Set("light_target", target.ToString());
		args.Set("light_right", right.ToString());
		args.Set("light_up", up.ToString());
		args.SetFloat("angle", angle);
		args.Set("texture", spotlightTexture);
		args.Set("_color", lightColor.ToString());

		light = static_cast<idLight *>(gameLocal.SpawnEntityType(idLight::Type, &args));
		light->Bind(this, true);
		light->SetAngles( idAngles(0, 0, 0) );
		spotLight = light;
		light->UpdateVisuals();
	}

	if ( useColors ) {
		light->Event_SetColor(colorSweeping[0], colorSweeping[1], colorSweeping[2]);
	}
}

/*
================
idSecurityCamera::UpdateColors
================
*/
void idSecurityCamera::UpdateColors()
{
	if ( !useColors ) {
		return;
	}

	idVec3	colorNew;
	idLight* light = spotLight.GetEntity();

	switch (state)
	{
	case STATE_PLAYERSIGHTED:
		colorNew = colorSighted;
		break;
	case STATE_ALERTED:
		colorNew = colorAlerted;
		break;
	default:
		colorNew = colorSweeping;
		break;
	}

	Event_SetColor(colorNew[0], colorNew[1], colorNew[2]);

	if ( light ) {
		light->Event_SetColor(colorNew[0], colorNew[1], colorNew[2]);
	}
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
			Event_SetSkin(spawnArgs.GetString("skin_on", "security_camera_on"));
		}
		else
		{
			light->Off();
			Event_SetSkin(spawnArgs.GetString("skin_on_spotlight_off", "security_camera_on_spotlight_off"));
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
idSecurityCamera::Event_GetSecurityCameraState
================
*/
void idSecurityCamera::Event_GetSecurityCameraState()
{
	int retFloat;

	if (!powerOn && state != STATE_DEAD)
	{
		//camera is switched off and not destroyed
		retFloat = 4;
		idThread::ReturnFloat(retFloat);
		return;
	}

	/*
	4 states are quickly converted to STATE_SWEEPING. 
	They will therefore only rarely be detected and
	are functionally identical to STATE_SWEEPING (camera is unalerted and active).
	*/

	switch (state)
	{
	case STATE_SWEEPING:
		retFloat = 1;
		break;
	case STATE_PLAYERSIGHTED:
		retFloat = 2;
		break;
	case STATE_ALERTED:
		retFloat = 3;
		break;
	case STATE_LOSTINTEREST:
		retFloat = 1;
		break;
	case STATE_POWERRETURNS_SWEEPING:
		retFloat = 1;
		break;
	case STATE_POWERRETURNS_PAUSED:
		retFloat = 1;
		break;
	case STATE_PAUSED:
		retFloat = 1;
		break;
	case STATE_DEAD:
		retFloat = 5;
		break;
	}

	idThread::ReturnFloat( retFloat );
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
	rv->fov_x = cameraFovX;
	rv->fov_y = cameraFovY;
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
	idVec3 origin = GetPhysics()->GetOrigin();
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

		// take lighting into account
		if ( IsEntityHiddenByDarkness(ent, sightThreshold) )
		{
			continue;
		}



		idVec3 eye = ent->EyeOffset();
		idVec3 start;
		idVec3 originPlayer = ent->GetPhysics()->GetOrigin();

		// check for eyes
		dir = (originPlayer + eye) - origin;
		dist = dir.Normalize();
		start = 0.95f* + 0.05f*(originPlayer + eye);
		if (dist < scanDist && dir * GetAxis() > scanFovCos) {
			gameLocal.clip.TracePoint(tr, start, originPlayer + eye, MASK_OPAQUE, this);
			if (tr.fraction == 1.0 || (gameLocal.GetTraceEntity(tr) == ent)) {
				gameLocal.pvs.FreeCurrentPVS(handle);
				timeLastSeen = gameLocal.time;
				if ( follow ) {
					dir = (originPlayer + eye/2) - origin;	//focus on the torso
					idAngles a		= dir.ToAngles();
					angleToPlayer	= a.yaw;
					inclineToPlayer	= a.pitch;
				}
				return true;
			}
		}

		// check for origin
		dir = originPlayer - origin;
		dist = dir.Normalize();
		start = 0.95f*origin + 0.05f*originPlayer;
		if (dist < scanDist && dir * GetAxis() > scanFovCos) {
			gameLocal.clip.TracePoint(tr, origin, originPlayer, MASK_OPAQUE, this);
			if (tr.fraction == 1.0 || (gameLocal.GetTraceEntity(tr) == ent)) {
				gameLocal.pvs.FreeCurrentPVS(handle);
				timeLastSeen = gameLocal.time;
				if ( follow ) {
					dir = (originPlayer + eye / 2) - origin;	//focus on the torso
					idAngles a		= dir.ToAngles();
					angleToPlayer	= a.yaw;
					inclineToPlayer = a.pitch;
				}
				return true;
			}
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
idSecurityCamera::TriggerSparks
================
*/
void idSecurityCamera::TriggerSparks( void )
{
	//do nothing if it is not yet time to spark
	if ( gameLocal.time < nextSparkTime )
	{
		return;
	}

	if ( !sparksPeriodic )
	{
		BecomeInactive(TH_UPDATEPARTICLES);

		if ( sparksPowerDependent )
		{
			if ( sparksOn == powerOn )
			{
				return;
			}

			else
			{
				sparksOn = powerOn;
			}
		}
	}

	idEntity *sparkEntity = sparks.GetEntity();

	//Create a func_emitter if none exists yet
	if (sparkEntity == NULL)
	{
		idDict args;
		const char *model;
		const char *cycleTrigger;

		spawnArgs.GetString("sparks_particle", "sparks_wires_oneshot.prt", &model);
		spawnArgs.GetString("sparks_periodic", "1", &cycleTrigger);

		args.Set("classname", "func_emitter");
		args.Set("origin", GetPhysics()->GetOrigin().ToString());
		args.Set("model", model);
		args.Set("cycleTrigger", cycleTrigger);
		gameLocal.SpawnEntityDef(args, &sparkEntity);
		sparks = sparkEntity;
		sparksOn = true;
		if ( sparksPeriodic )
		{
			sparkEntity->Activate(NULL);
		}
	}

	else
	{
		sparkEntity->Activate(NULL);
	}

	StopSound(SND_CHANNEL_ANY, false);
	StartSound("snd_sparks", SND_CHANNEL_BODY, 0, false, NULL);
	nextSparkTime = gameLocal.time + SEC2MS(sparksInterval) + SEC2MS(gameLocal.random.RandomInt(sparksIntervalRand));
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

		if ( health <= 0 && fl.takedamage )
		{
			BecomeInactive( TH_THINK );
			return;
		}
	}

	// run physics
	RunPhysics();

	if ( state == STATE_DEAD && ( thinkFlags & TH_UPDATEPARTICLES ) )
	{
		TriggerSparks(); // Trigger spark effect
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
				UpdateColors();
				if (follow)
				{
					following = true;
					angleTarget = angleToPlayer;
					inclineTarget = inclineToPlayer;
					followSpeedMult = spawnArgs.GetFloat("follow_speed_mult", "1.2");
					TurnToTarget();
				}
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
					nextAlertTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat("alarm_interval", "5"));
					endAlertTime = gameLocal.time + SEC2MS(alarm_duration);
					SetAlertMode(MODE_ALERT);
					ActivateTargets(this);
					state = STATE_ALERTED;
					UpdateColors();
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
				UpdateColors();
			}
			break;
		case STATE_ALERTED:
			if ( gameLocal.time < endAlertTime )
			{
				// is it time to sound the alert again?
				if ( gameLocal.time >= nextAlertTime )
				{
					nextAlertTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat("alarm_interval", "5"));
					StopSound(SND_CHANNEL_ANY, false);
					StartSound("snd_alert", SND_CHANNEL_BODY, 0, false, NULL);
				}

				//extend the alert state if the camera has recently seen the player
				if ( endAlertTime - timeLastSeen < SEC2MS(alarm_duration / 2) )
				{
					endAlertTime = gameLocal.time + SEC2MS(alarm_duration / 2);
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
				UpdateColors();
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
			UpdateColors();
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
			UpdateColors();
			break;
		case STATE_PAUSED:
			if ( followIncline && (gameLocal.time < inclineEndTime) )
			{
				break;
			}
			else if ( ( gameLocal.time >= pauseEndTime ) )
			{
				if ( rotate && !stationary )
				{
					ReverseSweep(); // changes state to STATE_SWEEPING
					UpdateColors();
				}
			}
			break;
		case STATE_DEAD:
			break;
		}

		if ( rotate )
		{
			if ( sweeping || following )
			{
				idAngles a = GetPhysics()->GetAxis().ToAngles();

				if ( gameLocal.time <= sweepEndTime )
				{
					percentSwept = (gameLocal.time - sweepStartTime) / (sweepEndTime - sweepStartTime);
					travel = percentSwept * sweepAngle;
					a.yaw = (negativeSweep) ? angle + travel : angle - travel;
				}

				if ( followIncline && (gameLocal.time <= inclineEndTime) )
				{
					percentInclined = (gameLocal.time - inclineStartTime) / (inclineEndTime - inclineStartTime);
					travel = percentInclined * inclineAngle;
					a.pitch = (negativeIncline) ? incline + travel : incline - travel;

				}

				SetAngles(a);
			}

			//check whether the player has moved to another position in the camera's view
			if ( following && CanSeePlayer() )
			{
				float sweepDist		= fabs(idMath::AngleDelta(angleToPlayer, angleTarget));
				float inclineDist	= fabs(idMath::AngleDelta(inclineToPlayer, inclineTarget));

				if ( ( sweepDist > followTolerance ) || ( followIncline && ( inclineDist > followInclineTolerance ) ) )
				{
					angleTarget = angleToPlayer;
					inclineTarget = inclineToPlayer;
					TurnToTarget();
				}
			}
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
idSecurityCamera::StartSweep
================
*/
void idSecurityCamera::StartSweep( void ) {
	sweeping = true;
	sweepStartTime = gameLocal.time;
	sweepEndTime = sweepStartTime + SEC2MS(sweepTime);
	emitPauseSoundTime = sweepEndTime - PAUSE_SOUND_TIMING;
	StartSound( "snd_moving", SND_CHANNEL_BODY, 0, false, NULL );
	emitPauseSound = true;
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

	angle = GetPhysics()->GetAxis().ToAngles().yaw;

	if ( following )
	{
		following = false;
		followSpeedMult = 1;

		float dist1 = fabs( idMath::AngleDelta(anglePos1, angle) );
		float dist2 = fabs( idMath::AngleDelta(anglePos2, angle) );

		angleTarget		= (dist1 < dist2) ? anglePos1 : anglePos2;
		inclineTarget	= inclinePos1;

		TurnToTarget();

		//if the sweep is relatively short, slow it down to finish simultaneously with the incline
		if ( followIncline && (sweepEndTime < inclineEndTime) ) {
			sweepEndTime = inclineEndTime;
		}
	}

	else if ( !following )
	{
		float timeRemaining = (1.0f - percentSwept)*sweepTime;
		sweepStartTime = gameLocal.time;
		sweepEndTime = gameLocal.time + SEC2MS(timeRemaining);
	}

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
	angle			= GetPhysics()->GetAxis().ToAngles().yaw;
	angleTarget		= (angleTarget == anglePos1) ? anglePos2 : anglePos1;

	sweepAngle		= idMath::AngleDelta(angle, angleTarget);
	negativeSweep	= (sweepAngle < 0) ? true : false;
	sweepAngle		= fabs(sweepAngle);

	StartSweep();
}

/*
================
idSecurityCamera::TurnToTarget
================
*/
void idSecurityCamera::TurnToTarget( void )
{
	angle			= GetPhysics()->GetAxis().ToAngles().yaw;
	sweepAngle		= idMath::AngleDelta(angle, angleTarget);

	if ( sweepAngle == 0 ) {
		sweepEndTime = gameLocal.time - 1;
	}

	else {
		negativeSweep = (sweepAngle < 0) ? true : false;
		sweepAngle = fabs(sweepAngle);

		sweepStartTime = gameLocal.time;
		sweepEndTime = gameLocal.time + SEC2MS(sweepAngle / ( sweepSpeed * followSpeedMult ) );
	}

	//also calculate incline parameters, if enabled
	if ( followIncline ) {
		incline			= GetPhysics()->GetAxis().ToAngles().pitch;
		inclineAngle	= idMath::AngleDelta(incline, inclineTarget);

		if (inclineAngle == 0) {
			inclineEndTime = gameLocal.time - 1;
		}

		else {
			negativeIncline = (inclineAngle < 0) ? true : false;
			inclineAngle = fabs(inclineAngle);

			inclineStartTime = gameLocal.time;
			inclineEndTime = gameLocal.time + SEC2MS(inclineAngle / inclineSpeed);
		}
	}
}

/*
============
idSecurityCamera::Killed
============
*/
void idSecurityCamera::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( state == STATE_DEAD )
	{
		return;
	}

	sweeping = false;
	StopSound( SND_CHANNEL_ANY, false );

	idStr fx;

	if ( powerOn ) {
		StartSound("snd_death", SND_CHANNEL_BODY, 0, false, NULL);
		fx = spawnArgs.GetString("fx_destroyed");
	}
	else if ( !powerOn ) {
		StartSound("snd_death_nopower", SND_CHANNEL_BODY, 0, false, NULL);
		fx = spawnArgs.GetString("fx_destroyed_nopower");
	}

	if ( fx.Length() ) {
			idEntityFx::StartFx(fx, NULL, NULL, this, true);
	}

	// call base class method to switch to broken model
	idEntity::BecomeBroken( inflictor );

	Event_SetSkin(spawnArgs.GetString("skin_broken", "security_camera_off"));

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

	state = STATE_DEAD;

	if (spawnArgs.GetBool("sparks", "1"))
	{
		if ( !powerOn && sparksPowerDependent )
		{
			return;
		}

		nextSparkTime = gameLocal.time + SEC2MS(spawnArgs.GetFloat("sparks_delay", "2"));
		BecomeActive(TH_UPDATEPARTICLES); // keeps stationary camera thinking to display sparks
	}
}


/*
============
idSecurityCamera::Pain
============
*/
bool idSecurityCamera::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idStr fx;

	if ( powerOn ) {
		fx = spawnArgs.GetString("fx_damage");
	}
	else if ( !powerOn ) {
		fx = spawnArgs.GetString("fx_damage_nopower");
	}

	if ( fx.Length() ) {
		idEntityFx::StartFx(fx, NULL, NULL, this, true);
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
		if (spawnArgs.GetBool("sparks", "1") && sparksPowerDependent )
		{
			if (powerOn)
			{
				nextSparkTime = gameLocal.time;
				BecomeActive(TH_UPDATEPARTICLES);
			}
			else if (!powerOn)
			{
				BecomeInactive(TH_UPDATEPARTICLES);

				if ( sparksOn && !sparksPeriodic )
				{
					idEntity *sparksEntity = sparks.GetEntity();

					sparksEntity->Activate(NULL);	// for non-periodic particles
					StopSound(SND_CHANNEL_ANY, false);
					sparksOn = false;
				}
			}
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
			Event_SetSkin(spawnArgs.GetString("skin_on", "security_camera_on"));
		}
		else
		{
			Event_SetSkin(spawnArgs.GetString("skin_on_spotlight_off", "security_camera_off"));
		}
	}
	else
	{
		Event_SetSkin(spawnArgs.GetString("skin_off", "security_camera_off"));
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