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
// dragofer #5528 - Developed for 2.10

const idEventDef EV_SecurityCam_AddLight( "<addLight>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_SecurityCam_AddSparks( "<addSparks>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Peek_AddDisplay("<addDisplay>", EventArgs(), EV_RETURNS_VOID, "internal"); // grayman #4882
const idEventDef EV_SecurityCam_SpotLightToggle( "toggle_light", EventArgs(), EV_RETURNS_VOID, "Toggles the spotlight on/off." );
const idEventDef EV_SecurityCam_SpotLightState( "state_light", EventArgs('d', "set", ""), EV_RETURNS_VOID, "Switches the spotlight on or off. Respects the security camera's power state." );
const idEventDef EV_SecurityCam_SweepToggle( "toggle_sweep", EventArgs(), EV_RETURNS_VOID, "Toggles the camera sweep." );
const idEventDef EV_SecurityCam_SweepState( "state_sweep", EventArgs('d', "set", ""), EV_RETURNS_VOID, "Enables or disables the camera's sweeping." );
const idEventDef EV_SecurityCam_SeePlayerToggle( "toggle_see_player", EventArgs(), EV_RETURNS_VOID, "Toggles whether the camera can see the player." );
const idEventDef EV_SecurityCam_SeePlayerState( "state_see_player", EventArgs('d', "set", ""), EV_RETURNS_VOID, "Set whether the camera can see the player." );
const idEventDef EV_SecurityCam_GetSpotLight("getSpotLight", EventArgs(), 'e', "Returns the spotlight used by the camera. Returns null_entity if none is used.");
const idEventDef EV_SecurityCam_GetSecurityCameraState("getSecurityCameraState", EventArgs(), 'f', "Returns the security camera's state. 1 = unalerted, 2 = suspicious, 3 = fully alerted, 4 = inactive, 5 = destroyed.");
const idEventDef EV_SecurityCam_GetHealth("getHealth", EventArgs(), 'f', "Returns the health of the security camera.");
const idEventDef EV_SecurityCam_SetHealth("setHealth", EventArgs('f', "health", ""), EV_RETURNS_VOID, "Set the health of the security camera. Setting to 0 or lower will destroy it.");
const idEventDef EV_SecurityCam_SetSightThreshold("setSightThreshold", EventArgs('f', "sightThreshold", ""), EV_RETURNS_VOID, "Set the sight threshold of the security camera: how lit up the player's lightgem needs to be in order to be seen. 0.0 to 1.0");
const idEventDef EV_SecurityCam_On( "On", EventArgs(), EV_RETURNS_VOID, "Switches the security camera on." );
const idEventDef EV_SecurityCam_Off( "Off", EventArgs(), EV_RETURNS_VOID, "Switches the security camera off." );

CLASS_DECLARATION( idEntity, idSecurityCamera )
	EVENT( EV_PostSpawn,							idSecurityCamera::PostSpawn )
	EVENT( EV_SecurityCam_AddLight,					idSecurityCamera::Event_AddLight )
	EVENT( EV_SecurityCam_AddSparks,				idSecurityCamera::Event_AddSparks )
	EVENT( EV_SecurityCam_SpotLightToggle,			idSecurityCamera::Event_SpotLight_Toggle )
	EVENT( EV_SecurityCam_SpotLightState,			idSecurityCamera::Event_SpotLight_State )
	EVENT( EV_SecurityCam_SweepToggle,				idSecurityCamera::Event_Sweep_Toggle )
	EVENT( EV_SecurityCam_SweepState,				idSecurityCamera::Event_Sweep_State )
	EVENT( EV_SecurityCam_SeePlayerToggle,			idSecurityCamera::Event_SeePlayer_Toggle )
	EVENT( EV_SecurityCam_SeePlayerState,			idSecurityCamera::Event_SeePlayer_State )
	EVENT( EV_SecurityCam_GetSpotLight,				idSecurityCamera::Event_GetSpotLight )	
	EVENT( EV_SecurityCam_GetSecurityCameraState,	idSecurityCamera::Event_GetSecurityCameraState )	
	EVENT( EV_SecurityCam_GetHealth,				idSecurityCamera::Event_GetHealth )
	EVENT( EV_SecurityCam_SetHealth,				idSecurityCamera::Event_SetHealth )
	EVENT( EV_SecurityCam_SetSightThreshold,		idSecurityCamera::Event_SetSightThreshold )
	EVENT( EV_SecurityCam_On,						idSecurityCamera::Event_On )
	EVENT( EV_SecurityCam_Off,						idSecurityCamera::Event_Off )
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

	savefile->WriteFloat(constrainPositive);
	savefile->WriteFloat(constrainNegative);
	savefile->WriteFloat(constrainUp);
	savefile->WriteFloat(constrainDown);

	savefile->WriteFloat(timeLastSeen);
	savefile->WriteFloat(alertDuration);

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
	savefile->WriteBool(flinderized);

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

	savefile->ReadFloat(constrainPositive);
	savefile->ReadFloat(constrainNegative);
	savefile->ReadFloat(constrainUp);
	savefile->ReadFloat(constrainDown);

	savefile->ReadFloat(timeLastSeen);
	savefile->ReadFloat(alertDuration);

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
	savefile->ReadBool(flinderized);

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
	sweepAngle		= spawnArgs.GetFloat("sweepAngle", "90");
	sweepSpeed		= spawnArgs.GetFloat("sweepSpeed", "15");
	health			= spawnArgs.GetInt("health", "100");
	scanFov			= spawnArgs.GetFloat("scanFov", "90");
	scanDist		= spawnArgs.GetFloat("scanDist", "200");
	flipAxis		= spawnArgs.GetBool("flipAxis", "0");
	useColors		= spawnArgs.GetBool("useColors");
	colorSweeping	= spawnArgs.GetVector("color_sweeping", "0.3 0.7 0.4");
	colorSighted	= spawnArgs.GetVector("color_sighted", "0.7 0.7 0.3");
	colorAlerted	= spawnArgs.GetVector("color_alerted", "0.7 0.3 0.3");
	sparksPowerDependent	= spawnArgs.GetBool("sparks_power_dependent", "1");
	sparksInterval			= spawnArgs.GetFloat("sparks_interval", "3");
	sparksIntervalRand		= spawnArgs.GetFloat("sparks_interval_rand", "2");
	sightThreshold			= spawnArgs.GetFloat("sight_threshold", "0.1");
	follow					= spawnArgs.GetBool("follow", "0");
	followIncline			= spawnArgs.GetBool("follow_incline", "0");
	followTolerance			= spawnArgs.GetFloat("follow_tolerance", "15");
	followInclineTolerance	= spawnArgs.GetFloat("follow_incline_tolerance", "10");
	state	= STATE_SWEEPING;
	sweeping = false;
	following = false;
	sparksOn = false;
	stationary = false;
	m_bFlinderize = false;
	flinderized = false;
	sparksPeriodic = true;
	followSpeedMult = 0;
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

	//check if this is an old version of the entity
	if ( spawnArgs.GetBool("legacy", "0") ) {

		//sweepSpeed acts as sweepTime
		int sweepTime = spawnArgs.GetFloat("sweepSpeed", "5");
		sweepSpeed = fabs(sweepAngle) / sweepTime;

		//wait acts as alertDuration
		alertDuration = spawnArgs.GetFloat("wait", "20");
	}

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

	scanFovCos = cos( scanFov * idMath::PI / 360.0f );

	//yaw angle
	angle			= GetPhysics()->GetAxis().ToAngles().yaw;
	angleTarget		= idMath::AngleNormalize180( angle - sweepAngle );
	angleToPlayer	= 0;

	negativeSweep	= ( sweepAngle < 0 ) ? true : false;
	anglePos1		= ( negativeSweep ) ? angle : angleTarget;
	anglePos2		= ( negativeSweep ) ? angleTarget : angle;

	sweepAngle		= fabs(sweepAngle);
	percentSwept	= 0.0f;

	//pitch angle
	incline			= inclinePos1 = GetPhysics()->GetAxis().ToAngles().pitch;
	inclineTarget	= inclineToPlayer = 0;

	negativeIncline = false;
	inclineAngle	= 0;
	inclineSpeed	= spawnArgs.GetFloat("follow_incline_speed", "30");
	percentInclined = 0.0f;

	//constrain how far the security camera is able to rotate when following the player
	constrainPositive	= idMath::AngleNormalize180( anglePos1 - fabs(spawnArgs.GetFloat("follow_constrain_ccw", "45")) );
	constrainNegative	= idMath::AngleNormalize180( anglePos2 + fabs(spawnArgs.GetFloat("follow_constrain_cw", "45")) );
	constrainUp			= idMath::AngleNormalize180( inclinePos1 - fabs(spawnArgs.GetFloat("follow_constrain_up", "25")) );
	constrainDown		= idMath::AngleNormalize180( inclinePos1 + fabs(spawnArgs.GetFloat("follow_constrain_down", "25")) );
	
	//start on or off
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

	// find out whether the mapper has selected a looping particle. This changes how the code will control it
	const char *particle;
	spawnArgs.GetString("sparks_particle", "sparks_wires_oneshot.prt", &particle);

	const idDeclParticle *particleDecl;
	particleDecl = static_cast<const idDeclParticle *>( declManager->FindType(DECL_PARTICLE, particle) );

	if ( particleDecl )
	{
		for ( int stageNum = 0; stageNum < particleDecl->stages.Num(); stageNum++ )
		{
			idParticleStage *stage = particleDecl->stages[stageNum];
			if (stage->cycles == 0)
			{
				sparksPeriodic = false;
			}
		}
	}

	// setup the physics
	GetPhysics()->SetContents( CONTENTS_SOLID );

	// SR CONTENTS_RESPONSE FIX
	if( m_StimResponseColl->HasResponse() ) {
		GetPhysics()->SetContents(GetPhysics()->GetContents() | CONTENTS_RESPONSE);
	}

	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP);

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

	for( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
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
idSecurityCamera::Event_GetHealth
================
*/
void idSecurityCamera::Event_GetHealth()
{
	idThread::ReturnInt(health);
}

/*
================
idSecurityCamera::Event_SetHealth
================
*/
void idSecurityCamera::Event_SetHealth( float newHealth )
{
	health = static_cast<int>(newHealth);
	fl.takedamage = true;

	if ( health <= 0 ) {
		Killed( NULL, NULL, 0, idVec3(0, 0, 0), 0 );
	}
}

/*
================
idSecurityCamera::Event_SetSightThreshold
================
*/
void idSecurityCamera::Event_SetSightThreshold( float newThreshold )
{
	sightThreshold = newThreshold;
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
idSecurityCamera::Event_AddSparks
================
*/
void idSecurityCamera::Event_AddSparks(void)
{
		idEntity *sparkEntity;
		idDict args;
		const char *particle, *cycleTrigger;
		spawnArgs.GetString("sparks_particle", "sparks_wires_oneshot.prt", &particle);
		cycleTrigger = ( sparksPeriodic ) ? "1" : "0";

		args.Set( "classname", "func_emitter" );
		args.Set( "origin", GetPhysics()->GetOrigin().ToString() );
		args.Set( "model", particle );
		args.Set( "cycleTrigger", cycleTrigger );
		gameLocal.SpawnEntityDef( args, &sparkEntity );
		sparks = sparkEntity;
		sparksOn = true;
		if ( sparksPeriodic )
		{
			sparkEntity->Activate(NULL);
		}
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

	//check whether a looping particle emitter needs to be toggled
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

	//Create a func_emitter if none exists yet
	if ( sparks.GetEntity() == NULL)
	{
		Event_AddSparks();
	}
	//Otherwise use the existing one
	else
	{
		sparks.GetEntity()->Activate(NULL);
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

	if ( ( state == STATE_DEAD ) && ( thinkFlags & TH_UPDATEPARTICLES ) )
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
					endAlertTime = gameLocal.time + SEC2MS(alertDuration);
					SetAlertMode(MODE_ALERT);
					state = STATE_ALERTED;
					UpdateColors();
					if ( spawnArgs.GetBool("trigger_alarm_start", "1") )
					{
						ActivateTargets(this);
					}
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
				if ( endAlertTime - timeLastSeen < SEC2MS(alertDuration / 2) )
				{
					endAlertTime = gameLocal.time + SEC2MS(alertDuration / 2);
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
				if ( spawnArgs.GetBool("trigger_alarm_end", "0") )
				{
					ActivateTargets(this);
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

		if ( rotate && !stationary )
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

				if ( followIncline && ( gameLocal.time <= inclineEndTime ) )
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
				float sweepDist		= fabs( idMath::AngleNormalize180(angleToPlayer - angleTarget) );
				float inclineDist	= fabs( idMath::AngleNormalize180(inclineToPlayer - inclineTarget) );

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
	sweepEndTime = sweepStartTime + SEC2MS(sweepAngle / sweepSpeed);
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

	// camera was chasing the player; return to the closest position
	if ( following )
	{
		following = false;
		followSpeedMult = 1;

		float dist1 = fabs( idMath::AngleNormalize180(anglePos1 - angle) );
		float dist2 = fabs( idMath::AngleNormalize180(anglePos2 - angle) );

		angleTarget		= (dist1 < dist2) ? anglePos1 : anglePos2;
		inclineTarget	= inclinePos1;

		TurnToTarget();

		//synchronise sweep and incline, use the longer time
		if ( followIncline )
		{
			sweepEndTime = inclineEndTime = ( sweepEndTime > inclineEndTime ) ? sweepEndTime : inclineEndTime;
		}
	}

	// security camera was switched off or saw the player but didn't turn towards him
	else
	{
		sweepAngle = idMath::AngleNormalize180(angle - angleTarget);

		if ( sweepAngle == 0 ) {
			sweepEndTime = gameLocal.time -1;
		}
		else
		{
			if ( sweepAngle > 0 && negativeSweep ) {
				sweepAngle -= 360;
			}
			if ( sweepAngle < 0 && !negativeSweep )	{
				sweepAngle += 360;
			}
			sweepAngle = fabs(sweepAngle);
			sweepStartTime = gameLocal.time;
			sweepEndTime = gameLocal.time + SEC2MS(sweepAngle / sweepSpeed);
		}
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
	angleTarget		= ( angleTarget == anglePos2 ) ? anglePos1 : anglePos2;
	sweepAngle		= fabs( spawnArgs.GetFloat("sweepAngle", "90") );

	if ( anglePos1 == anglePos2 )	negativeSweep = !negativeSweep;
	else							negativeSweep = (angleTarget == anglePos2) ? true : false; 

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
	sweepAngle		= idMath::AngleNormalize180(angle - angleTarget);

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
		if ( idMath::AngleNormalize180(constrainDown - inclineTarget) < 0 )	inclineTarget = constrainDown;
		if ( idMath::AngleNormalize180(constrainUp - inclineTarget) > 0)	inclineTarget = constrainUp;

		incline			= GetPhysics()->GetAxis().ToAngles().pitch;
		inclineAngle	= idMath::AngleNormalize180(incline - inclineTarget);

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
================
idSecurityCamera::Damage

Modified version of idEntity::Damage that makes use of damage multipliers depending on the inflictor
================
*/
void idSecurityCamera::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
	const char *damageDefName, const float damageScale,	const int location, trace_t *tr)
{
	if ( !fl.takedamage ) {
		return;
	}
	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName, true); // grayman #3391 - don't create a default 'damageDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( !damageDef )
	{
		gameLocal.Error("Unknown damageDef '%s'\n", damageDefName);
	}
	

	// check what is damaging the security camera and adjust damage according to spawnargs
	idStr def = damageDefName;
	idStr inf = inflictor->GetEntityDefName();
	int damage = damageDef->GetInt( "damage" );
	float damage_mult = 1.0f;
	float exponential = spawnArgs.GetFloat("damage_splash_falloff", "2.0");

	// has the mapper specified a spawnarg for a specific damageDef or entityDef? Should take priority
	if ( spawnArgs.GetString("damage_mult_" + def, "") != "" ) {
		damage_mult = spawnArgs.GetFloat("damage_mult_" + def, "1.0");
	}
	else if ( spawnArgs.GetString("damage_mult_" + inf, "") != "" ) {
		damage_mult = spawnArgs.GetFloat("damage_mult_" + inf, "1.0");
	}

	// otherwise check the standard spawnargs
	else {
		if ( def == "atdm:damage_firearrowDirect" ) {
			damage_mult = spawnArgs.GetFloat("damage_mult_firearrow_direct", "1.0");
		}
		else if ( def == "atdm:damage_firearrowSplash" ) {
			damage_mult = spawnArgs.GetFloat("damage_mult_firearrow_splash", "3.5");
		}
		else if ( def == "atdm:damage_arrow" ) {
			damage_mult = spawnArgs.GetFloat("damage_mult_arrow", "0.0");
		}
		else if ( inf == "atdm:attachment_melee_shortsword" ) {
			damage_mult = spawnArgs.GetFloat("damage_mult_sword", "0.0");
		}
		else if ( inf == "atdm:attachment_meleetest_blackjack" ) {
			damage_mult = 1.0f;
			damage = spawnArgs.GetInt("damage_blackjack", "0");
		}
		else if ( inflictor->IsType(idMoveable::Type) )
		{
			float mass = inflictor->GetPhysics()->GetMass();
			damage = (int)( damage * (mass / 5.0f) * spawnArgs.GetFloat("damage_mult_moveable", "1.0") );
		}
		else {
			damage_mult = spawnArgs.GetFloat("damage_mult_other", "1.0");
		}
	}

	damage *= damage_mult * pow( fabs(damageScale), exponential );

	// inform the attacker that they hit someone
	attacker->DamageFeedback(this, inflictor, damage);
	if ( damage )
	{
		// do the damage
		health -= damage;
		if ( health <= 0 )
		{
			if ( health < -999 )
			{
				health = -999;
			}

			// decide wether to flinderize
			int flinderize_threshold = spawnArgs.GetInt("damage_flinderize", "100");
			if ( !flinderized && ( flinderize_threshold > 0 ) && ( damage >= flinderize_threshold ) )
			{
				m_bFlinderize = true;
			}
			else
			{
				m_bFlinderize = false;
			}

			Killed(inflictor, attacker, damage, dir, location);
		}
		else
		{
			Pain(inflictor, attacker, damage, dir, location);
		}
	}
}

/*
============
idSecurityCamera::Killed

Called whenever the camera is destroyed or damaged after destruction
============
*/
void idSecurityCamera::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idStr str;

	// Become broken, possibly also flinderize
	if ( !m_bIsBroken )
	{
		idEntity::BecomeBroken(inflictor);
		Event_SetSkin(spawnArgs.GetString("skin_broken", "security_camera_off"));
	}

	// If flinderizing, override model & skin
	if ( m_bFlinderize )
	{
		// if Flinderize didn't already get called during BecomeBroken, call it now
		if ( m_bIsBroken )
		{
			idEntity::Flinderize(inflictor);
		}

		m_bFlinderize = false;	// don't flinderize again
		flinderized = true;		// has been flinderized, don't check anymore whether to flinderize again

		spawnArgs.GetString("broken_flinderized", "-", str);
		if (str.Length() > 1) {
			SetModel(str);
		}
		spawnArgs.GetString("skin_flinderized", "-", str);
		if (str.Length() > 1) {
			Event_SetSkin(str);
		}
	}


	// Handle destruction / post-destruction fx
	// security camera is being broken right now
	if ( !m_bIsBroken )
	{
		if ( powerOn ) {
			StartSound("snd_death", SND_CHANNEL_BODY, 0, false, NULL);
			str = spawnArgs.GetString("fx_destroyed");
		}
		else if ( !powerOn ) {
			StartSound("snd_death_nopower", SND_CHANNEL_BODY, 0, false, NULL);
			str = spawnArgs.GetString("fx_destroyed_nopower");
		}
	}

	// security camera was already broken
	else
	{
		if ( powerOn ) {
			str = spawnArgs.GetString("fx_damage_nopower");
		}
		else if ( !powerOn ) {
			str = spawnArgs.GetString("fx_damage_nopower");
		}
	}

	if ( str.Length() ) {
		idEntityFx::StartFx(str, NULL, NULL, this, true);
	}

	if ( state == STATE_DEAD ) {
		return;
	}

	state = STATE_DEAD;
	sweeping = false;
	StopSound( SND_CHANNEL_ANY, false );

	if ( spawnArgs.GetBool("notice_destroyed", "1") ) {
		SetStimEnabled(ST_VISUAL, true); // let AIs see that the camera is destroyed
	}

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

	// Activate sparks
	if ( spawnArgs.GetBool("sparks", "1") )
	{
		if ( powerOn || !sparksPowerDependent )
		{
			nextSparkTime = gameLocal.time + SEC2MS( spawnArgs.GetFloat("sparks_delay", "2") );
			BecomeActive(TH_UPDATEPARTICLES); // keeps stationary camera thinking to display sparks
		}
	}

	// Active a designated entity if destroyed 
	spawnArgs.GetString("break_up_target", "", str);
	idEntity *ent = gameLocal.FindEntity(str);
	if ( ent )
	{
		ent->Activate( this );
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
	if ( m_bFrobable )
	{
		UpdateFrobState();
		UpdateFrobDisplay();
	}

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

	// handle trigger Responses and Signals
	TriggerResponse(activator, ST_TRIGGER);

	if ( RespondsTo(EV_Activate) || HasSignal(SIG_TRIGGER) )
	{
		Signal(SIG_TRIGGER);
		ProcessEvent(&EV_Activate, activator);
		TriggerGuis();
	}
	
	// handle sparks (post-destruction)
	if ( state == STATE_DEAD && spawnArgs.GetBool("sparks", "1") && sparksPowerDependent )
	{
		if ( powerOn )
		{
			nextSparkTime = gameLocal.time;
			BecomeActive(TH_UPDATEPARTICLES);
		}
		else
		{
			BecomeInactive(TH_UPDATEPARTICLES);

			// toggle off looping particles
			if ( sparksOn && !sparksPeriodic )
			{
				idEntity *sparksEntity = sparks.GetEntity();

				sparksEntity->Activate(NULL);
				StopSound(SND_CHANNEL_ANY, false);
				sparksOn = false;
			}
		}
		return;
	}

	// handle spotlight
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

	// handle skin
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
idSecurityCamera::Event_SpotLight_Toggle
================
*/
void idSecurityCamera::Event_SpotLight_Toggle( void )
{
	Event_SpotLight_State( !spotlightPowerOn );
}

/*
================
idSecurityCamera::Event_SpotLight_State
================
*/
void idSecurityCamera::Event_SpotLight_State( bool set )
{
	idLight* light = spotLight.GetEntity();
	if ( light == NULL )
	{
		return; // no spotlight was defined; nothing to do
	}

	spotlightPowerOn = set;

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
idSecurityCamera::Event_Sweep_Toggle
================
*/
void idSecurityCamera::Event_Sweep_Toggle( void )
{
	Event_Sweep_State( stationary );	//will invert 'stationary'
}

/*
================
idSecurityCamera::Event_Sweep_State
================
*/
void idSecurityCamera::Event_Sweep_State( bool set )
{
	if ( stationary == !set )
	{
		return;	//do nothing if the security camera is already in the desired sweep status
	}
	else
	{
		stationary = !set;

		switch( state )
		{
		case STATE_SWEEPING:
			if ( stationary )
			{
				sweeping = false;
				StopSound(SND_CHANNEL_ANY, false);
				StartSound("snd_end", SND_CHANNEL_BODY, 0, false, NULL);
			}
			else if ( !stationary )
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
}

/*
================
idSecurityCamera::Event_SeePlayer_Toggle
================
*/
void idSecurityCamera::Event_SeePlayer_Toggle( void )
{
	//Use a spawnarg-based approach for backwards compatibility
	const char *kv = ( spawnArgs.GetBool("seePlayer", "0") ) ? "0" : "1";
	spawnArgs.Set( "seePlayer", kv );
}

/*
================
idSecurityCamera::Event_SeePlayer_State
================
*/
void idSecurityCamera::Event_SeePlayer_State( bool set )
{
	const char *kv = ( set ) ? "1" : "0";
	spawnArgs.Set( "seePlayer", kv );
}

/*
================
idSecurityCamera::Event_On
================
*/
void idSecurityCamera::Event_On( void )
{
	if ( !powerOn ) {
		Activate(NULL);
	}
}

/*
================
idSecurityCamera::Event_Off
================
*/
void idSecurityCamera::Event_Off( void )
{
	if ( powerOn ) {
		Activate(NULL);
	}
}