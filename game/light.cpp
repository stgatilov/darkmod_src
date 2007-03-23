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

#include "Game_local.h"
#include "../darkmod/darkmodglobals.h"
#include "../darkmod/playerdata.h"
#include "../DarkMod/StimResponse.h"

/*
===============================================================================

  idLight

===============================================================================
*/

const idEventDef EV_Light_SetShader( "setShader", "s" );
const idEventDef EV_Light_GetLightParm( "getLightParm", "d", 'f' );
const idEventDef EV_Light_SetLightParm( "setLightParm", "df" );
const idEventDef EV_Light_SetLightParms( "setLightParms", "ffff" );
const idEventDef EV_Light_SetRadiusXYZ( "setRadiusXYZ", "fff" );
const idEventDef EV_Light_SetRadius( "setRadius", "f" );
const idEventDef EV_Light_On( "On", NULL );
const idEventDef EV_Light_Off( "Off", NULL );
const idEventDef EV_Light_FadeOut( "fadeOutLight", "f" );
const idEventDef EV_Light_FadeIn( "fadeInLight", "f" );

// TDM Additions:
const idEventDef EV_Light_GetLightOrigin( "getLightOrigin", NULL, 'v' );
const idEventDef EV_Light_SetLightOrigin( "setLightOrigin", "v" );
const idEventDef EV_Light_GetLightLevel ("getLightLevel", NULL, 'f');


CLASS_DECLARATION( idEntity, idLight )
	EVENT( EV_Light_SetShader,		idLight::Event_SetShader )
	EVENT( EV_Light_GetLightParm,	idLight::Event_GetLightParm )
	EVENT( EV_Light_SetLightParm,	idLight::Event_SetLightParm )
	EVENT( EV_Light_SetLightParms,	idLight::Event_SetLightParms )
	EVENT( EV_Light_SetRadiusXYZ,	idLight::Event_SetRadiusXYZ )
	EVENT( EV_Light_SetRadius,		idLight::Event_SetRadius )
	EVENT( EV_Hide,					idLight::Event_Hide )
	EVENT( EV_Show,					idLight::Event_Show )
	EVENT( EV_Light_On,				idLight::Event_On )
	EVENT( EV_Light_Off,			idLight::Event_Off )
	EVENT( EV_Activate,				idLight::Event_ToggleOnOff )
	EVENT( EV_PostSpawn,			idLight::Event_SetSoundHandles )
	EVENT( EV_Light_FadeOut,		idLight::Event_FadeOut )
	EVENT( EV_Light_FadeIn,			idLight::Event_FadeIn )

	EVENT( EV_Light_SetLightOrigin, idLight::Event_SetLightOrigin )
	EVENT( EV_Light_GetLightOrigin, idLight::Event_GetLightOrigin )
	EVENT( EV_Light_GetLightLevel,	idLight::Event_GetLightLevel )
	EVENT( EV_InPVS,				idLight::Event_InPVS )
END_CLASS


/*
================
idGameEdit::ParseSpawnArgsToRenderLight

parse the light parameters
this is the canonical renderLight parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderLight( const idDict *args, renderLight_t *renderLight ) {
	bool	gotTarget, gotUp, gotRight;
	const char	*texture;
	idVec3	color;

	memset( renderLight, 0, sizeof( *renderLight ) );

	if (!args->GetVector("light_origin", "", renderLight->origin)) {
		args->GetVector( "origin", "", renderLight->origin );
	}

	gotTarget = args->GetVector( "light_target", "", renderLight->target );
	gotUp = args->GetVector( "light_up", "", renderLight->up );
	gotRight = args->GetVector( "light_right", "", renderLight->right );
	args->GetVector( "light_start", "0 0 0", renderLight->start );
	if ( !args->GetVector( "light_end", "", renderLight->end ) ) {
		renderLight->end = renderLight->target;
	}

	// we should have all of the target/right/up or none of them
	if ( ( gotTarget || gotUp || gotRight ) != ( gotTarget && gotUp && gotRight ) ) {
		gameLocal.Printf( "Light at (%f,%f,%f) has bad target info\n",
			renderLight->origin[0], renderLight->origin[1], renderLight->origin[2] );
		return;
	}

	if ( !gotTarget ) {
		renderLight->pointLight = true;

		// allow an optional relative center of light and shadow offset
		args->GetVector( "light_center", "0 0 0", renderLight->lightCenter );

		// create a point light
		if (!args->GetVector( "light_radius", "300 300 300", renderLight->lightRadius ) ) {
			float radius;

			args->GetFloat( "light", "300", radius );
			renderLight->lightRadius[0] = renderLight->lightRadius[1] = renderLight->lightRadius[2] = radius;
		}
	}

	// get the rotation matrix in either full form, or single angle form
	idAngles angles;
	idMat3 mat;
	if ( !args->GetMatrix( "light_rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
		if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", mat ) ) {
	   		args->GetFloat( "angle", "0", angles[ 1 ] );
   			angles[ 0 ] = 0;
			angles[ 1 ] = idMath::AngleNormalize360( angles[ 1 ] );
	   		angles[ 2 ] = 0;
			mat = angles.ToMat3();
		}
	}

	// fix degenerate identity matrices
	mat[0].FixDegenerateNormal();
	mat[1].FixDegenerateNormal();
	mat[2].FixDegenerateNormal();

	renderLight->axis = mat;

	// check for other attributes
	args->GetVector( "_color", "1 1 1", color );
	renderLight->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderLight->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderLight->shaderParms[ SHADERPARM_BLUE ]		= color[2];
	args->GetFloat( "shaderParm3", "1", renderLight->shaderParms[ SHADERPARM_TIMESCALE ] );
	if ( !args->GetFloat( "shaderParm4", "0", renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] ) ) {
		// offset the start time of the shader to sync it to the game time
		renderLight->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	}

	args->GetFloat( "shaderParm5", "0", renderLight->shaderParms[5] );
	args->GetFloat( "shaderParm6", "0", renderLight->shaderParms[6] );
	args->GetFloat( "shaderParm7", "0", renderLight->shaderParms[ SHADERPARM_MODE ] );
	args->GetBool( "noshadows", "0", renderLight->noShadows );
	args->GetBool( "nospecular", "0", renderLight->noSpecular );
	args->GetBool( "parallel", "0", renderLight->parallel );

	args->GetString( "texture", "lights/squarelight1", &texture );
	// allow this to be NULL
	renderLight->shader = declManager->FindMaterial( texture, false );
}

/*
================
idLight::UpdateChangeableSpawnArgs
================
*/
void idLight::UpdateChangeableSpawnArgs( const idDict *source ) {

	idEntity::UpdateChangeableSpawnArgs( source );

	if ( source ) {
		source->Print();
	}
	FreeSoundEmitter( true );
	gameEdit->ParseSpawnArgsToRefSound( source ? source : &spawnArgs, &refSound );
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
	}

	gameEdit->ParseSpawnArgsToRenderLight( source ? source : &spawnArgs, &renderLight );

	UpdateVisuals();
}

/*
================
idLight::idLight
================
*/
idLight::idLight()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX %s\r", this, __FUNCTION__);

	memset( &renderLight, 0, sizeof( renderLight ) );
	localLightOrigin	= vec3_zero;
	localLightAxis		= mat3_identity;
	lightDefHandle		= -1;
	levels				= 0;
	currentLevel		= 0;
	baseColor			= vec3_zero;
	breakOnTrigger		= false;
	count				= 0;
	triggercount		= 0;
	lightParent			= NULL;
	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;
	soundWasPlaying		= false;
	m_MaxLightRadius	= 0.0f;
	m_LightMaterial		= NULL;

	/*!
	Darkmod LAS
	*/
	LASAreaIndex = -1;

}

/*
================
idLight::~idLight
================
*/
idLight::~idLight()
{
	if ( lightDefHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
	}

	/*!
	* Darkmod LAS
	*/
	// Remove light from LAS
	if (LASAreaIndex != -1)
	{
		LAS.removeLight(this);
	}

	/*!
	Darkmod player lighting
	*/
	g_Global.m_DarkModPlayer->RemoveLight(this);


}

/*
================
idLight::Save

archives object for save game file
================
*/
void idLight::Save( idSaveGame *savefile ) const {
	savefile->WriteRenderLight( renderLight );
	
	savefile->WriteBool( renderLight.prelightModel != NULL );

	savefile->WriteVec3( localLightOrigin );
	savefile->WriteMat3( localLightAxis );

	savefile->WriteString( brokenModel );
	savefile->WriteInt( levels );
	savefile->WriteInt( currentLevel );

	savefile->WriteVec3( baseColor );
	savefile->WriteBool( breakOnTrigger );
	savefile->WriteInt( count );
	savefile->WriteInt( triggercount );
	savefile->WriteObject( lightParent );

	savefile->WriteVec4( fadeFrom );
	savefile->WriteVec4( fadeTo );
	savefile->WriteInt( fadeStart );
	savefile->WriteInt( fadeEnd );
	savefile->WriteBool( soundWasPlaying );
}

/*
================
idLight::Restore

unarchives object from save game file
================
*/
void idLight::Restore( idRestoreGame *savefile ) {
	bool hadPrelightModel;

	savefile->ReadRenderLight( renderLight );

	savefile->ReadBool( hadPrelightModel );
	renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	if ( ( renderLight.prelightModel == NULL ) && hadPrelightModel ) {
		assert( 0 );
		if ( developer.GetBool() ) {
			// we really want to know if this happens
			gameLocal.Error( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		} else {
			// but let it slide after release
			gameLocal.Warning( "idLight::Restore: prelightModel '_prelight_%s' not found", name.c_str() );
		}
	}

	savefile->ReadVec3( localLightOrigin );
	savefile->ReadMat3( localLightAxis );

	savefile->ReadString( brokenModel );
	savefile->ReadInt( levels );
	savefile->ReadInt( currentLevel );

	savefile->ReadVec3( baseColor );
	savefile->ReadBool( breakOnTrigger );
	savefile->ReadInt( count );
	savefile->ReadInt( triggercount );
	savefile->ReadObject( reinterpret_cast<idClass *&>( lightParent ) );

	savefile->ReadVec4( fadeFrom );
	savefile->ReadVec4( fadeTo );
	savefile->ReadInt( fadeStart );
	savefile->ReadInt( fadeEnd );
	savefile->ReadBool( soundWasPlaying );

	lightDefHandle = -1;

	SetLightLevel();
}

/*
================
idLight::Spawn
================
*/
void idLight::Spawn( void )
{
	bool start_off;
	bool needBroken;
	const char *demonic_shader;

	// do the parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRenderLight( &spawnArgs, &renderLight );

	// we need the origin and axis relative to the physics origin/axis
	localLightOrigin = ( renderLight.origin - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();
	localLightAxis = renderLight.axis * GetPhysics()->GetAxis().Transpose();

	// set the base color from the shader parms
	baseColor.Set( renderLight.shaderParms[ SHADERPARM_RED ], renderLight.shaderParms[ SHADERPARM_GREEN ], renderLight.shaderParms[ SHADERPARM_BLUE ] );

	// set the number of light levels
	spawnArgs.GetInt( "levels", "1", levels );
	currentLevel = levels;
	if ( levels <= 0 ) {
		gameLocal.Error( "Invalid light level set on entity #%d(%s)", entityNumber, name.c_str() );
	}

	// make sure the demonic shader is cached
	if ( spawnArgs.GetString( "mat_demonic", NULL, &demonic_shader ) ) {
		declManager->FindType( DECL_MATERIAL, demonic_shader );
	}

	// game specific functionality, not mirrored in
	// editor or dmap light parsing

	// also put the light texture on the model, so light flares
	// can get the current intensity of the light
	renderEntity.referenceShader = renderLight.shader;

	lightDefHandle = -1;		// no static version yet

	// see if an optimized shadow volume exists
	// the renderer will ignore this value after a light has been moved,
	// but there may still be a chance to get it wrong if the game moves
	// a light before the first present, and doesn't clear the prelight
	renderLight.prelightModel = 0;
	if ( name[ 0 ] ) {
		// this will return 0 if not found
		renderLight.prelightModel = renderModelManager->CheckModel( va( "_prelight_%s", name.c_str() ) );
	}

	spawnArgs.GetBool( "start_off", "0", start_off );
	if ( start_off ) {
		Off();
	}

	health = spawnArgs.GetInt( "health", "0" );
	spawnArgs.GetString( "broken", "", brokenModel );
	spawnArgs.GetBool( "break", "0", breakOnTrigger );
	spawnArgs.GetInt( "count", "1", count );

	triggercount = 0;

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart			= 0;
	fadeEnd				= 0;

	// if we have a health make light breakable
	if ( health ) {
		idStr model = spawnArgs.GetString( "model" );		// get the visual model
		if ( !model.Length() ) {
			gameLocal.Error( "Breakable light without a model set on entity #%d(%s)", entityNumber, name.c_str() );
		}

		fl.takedamage	= true;

		// see if we need to create a broken model name
		needBroken = true;
		if ( model.Length() && !brokenModel.Length() ) {
			int	pos;

			needBroken = false;
		
			pos = model.Find( "." );
			if ( pos < 0 ) {
				pos = model.Length();
			}
			if ( pos > 0 ) {
				model.Left( pos, brokenModel );
			}
			brokenModel += "_broken";
			if ( pos > 0 ) {
				brokenModel += &model[ pos ];
			}
		}
	
		// make sure the model gets cached
		if ( !renderModelManager->CheckModel( brokenModel ) ) {
			if ( needBroken ) {
				gameLocal.Error( "Model '%s' not found for entity %d(%s)", brokenModel.c_str(), entityNumber, name.c_str() );
			} else {
				brokenModel = "";
			}
		}

		GetPhysics()->SetContents( spawnArgs.GetBool( "nonsolid" ) ? 0 : CONTENTS_SOLID );
		// SR CONTENTS_RESONSE FIX
		if( m_StimResponseColl->HasResponse() )
			GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );
		
		// make sure the collision model gets cached
		idClipModel::CheckModel( brokenModel );
	}

	PostEventMS( &EV_PostSpawn, 0 );

	UpdateVisuals();

//	CDarkModPlayer *pDM = g_Global.m_DarkModPlayer;

	if(renderLight.pointLight == true)
		m_MaxLightRadius = renderLight.lightRadius.Length();
	else
	{
		idVec3 pos = GetPhysics()->GetOrigin();
		idVec3 max = renderLight.target + renderLight.right + renderLight.up;
		m_MaxLightRadius = max.Length();
	}
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("this: %08lX [%s] MaxLightRadius: %f\r", this, name.c_str(), m_MaxLightRadius);

	m_MaterialName = NULL;
	spawnArgs.GetString( "texture", "lights/squarelight1", &m_MaterialName);
	if(m_MaterialName != NULL)
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light has an image: %s\r", m_MaterialName);

	idImage *pImage;
	if(renderLight.shader != NULL && (pImage = renderLight.shader->LightFalloffImage()) != NULL)
	{
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Light has an image: %08lX\r", pImage);
	}

	g_Global.m_DarkModPlayer->AddLight(this);

	// Sophisiticated Zombie (DMH)
	// Darkmod Light Awareness System: Also need to add light to LAS
	LAS.addLight (this);

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("this: %08lX [%s]   noShadows: %u   noSpecular: %u   pointLight: %u     parallel: %u\r",
		this, name.c_str(),
		renderLight.noShadows,
		renderLight.noSpecular,
		renderLight.pointLight,
		renderLight.parallel);

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Red: %f    Green: %f    Blue: %f\r", baseColor.x, baseColor.y, baseColor.z);

	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Origin", GetPhysics()->GetOrigin());
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Radius", renderLight.lightRadius);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Center", renderLight.lightCenter);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Target", renderLight.target);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Right", renderLight.right);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Up", renderLight.up);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "Start", renderLight.start);
	DM_LOGVECTOR3(LC_LIGHT, LT_DEBUG, "End", renderLight.end);
}

/*
================
idLight::SetLightLevel
================
*/
void idLight::SetLightLevel( void ) {
	idVec3	color;
	float	intensity;

	intensity = ( float )currentLevel / ( float )levels;
	color = baseColor * intensity;
	renderLight.shaderParms[ SHADERPARM_RED ]	= color[ 0 ];
	renderLight.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderLight.shaderParms[ SHADERPARM_BLUE ]	= color[ 2 ];
	renderEntity.shaderParms[ SHADERPARM_RED ]	= color[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]= color[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]	= color[ 2 ];
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::GetColor
================
*/
void idLight::GetColor( idVec3 &out ) const {
	out[ 0 ] = renderLight.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderLight.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderLight.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
idLight::GetColor
================
*/
void idLight::GetColor( idVec4 &out ) const {
	out[ 0 ] = renderLight.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderLight.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderLight.shaderParms[ SHADERPARM_BLUE ];
	out[ 3 ] = renderLight.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( float red, float green, float blue ) {
	baseColor.Set( red, green, blue );
	SetLightLevel();
}

/*
================
idLight::SetColor
================
*/
void idLight::SetColor( const idVec4 &color ) {
	baseColor = color.ToVec3();
	renderLight.shaderParms[ SHADERPARM_ALPHA ]		= color[ 3 ];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[ 3 ];
	SetLightLevel();
}

/*
================
idLight::SetShader
================
*/
void idLight::SetShader( const char *shadername ) {
	// allow this to be NULL
	renderLight.shader = declManager->FindMaterial( shadername, false );
	PresentLightDefChange();
}

/*
================
idLight::SetLightParm
================
*/
void idLight::SetLightParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	renderLight.shaderParms[ parmnum ] = value;
	PresentLightDefChange();
}

/*
================
idLight::SetLightParms
================
*/
void idLight::SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	renderLight.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderLight.shaderParms[ SHADERPARM_GREEN ]		= parm1;
	renderLight.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderLight.shaderParms[ SHADERPARM_ALPHA ]		= parm3;
	renderEntity.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= parm3;
	PresentLightDefChange();
	PresentModelDefChange();
}

/*
================
idLight::SetRadiusXYZ
================
*/
void idLight::SetRadiusXYZ( float x, float y, float z ) {
	renderLight.lightRadius[0] = x;
	renderLight.lightRadius[1] = y;
	renderLight.lightRadius[2] = z;
	PresentLightDefChange();
}

/*
================
idLight::SetRadius
================
*/
void idLight::SetRadius( float radius ) {
	renderLight.lightRadius[0] = renderLight.lightRadius[1] = renderLight.lightRadius[2] = radius;
	PresentLightDefChange();
}

/*
================
idLight::On
================
*/
void idLight::On( void ) {
	currentLevel = levels;
	// offset the start time of the shader to sync it to the game time
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	if ( ( soundWasPlaying || refSound.waitfortrigger ) && refSound.shader ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
		soundWasPlaying = false;
	}
	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idLight::Off
================
*/
void idLight::Off( void ) {
	currentLevel = 0;
	// kill any sound it was making
	if ( refSound.referenceSound && refSound.referenceSound->CurrentlyPlaying() ) {
		StopSound( SND_CHANNEL_ANY, false );
		soundWasPlaying = true;
	}
	SetLightLevel();
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idLight::Fade
================
*/
void idLight::Fade( const idVec4 &to, float fadeTime ) {
	GetColor( fadeFrom );
	fadeTo = to;
	fadeStart = gameLocal.time;
	fadeEnd = gameLocal.time + SEC2MS( fadeTime );
	BecomeActive( TH_THINK );
}

/*
================
idLight::FadeOut
================
*/
void idLight::FadeOut( float time ) {
	Fade( colorBlack, time );
}

/*
================
idLight::FadeIn
================
*/
void idLight::FadeIn( float time ) {
	idVec3 color;
	idVec4 color4;

	currentLevel = levels;
	spawnArgs.GetVector( "_color", "1 1 1", color );
	color4.Set( color.x, color.y, color.z, 1.0f );
	Fade( color4, time );
}

/*
================
idLight::Killed
================
*/
void idLight::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	BecomeBroken( attacker );
}

/*
================
idLight::BecomeBroken
================
*/
void idLight::BecomeBroken( idEntity *activator ) {
	const char *damageDefName;

	fl.takedamage = false;

	if ( brokenModel.Length() ) {
		SetModel( brokenModel );

		if ( !spawnArgs.GetBool( "nonsolid" ) ) {
			GetPhysics()->SetClipModel( new idClipModel( brokenModel.c_str() ), 1.0f );
			GetPhysics()->SetContents( CONTENTS_SOLID );
			// SR CONTENTS_RESONSE FIX
			if( m_StimResponseColl->HasResponse() )
				GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );
		}
	} else if ( spawnArgs.GetBool( "hideModelOnBreak" ) ) {
		SetModel( "" );
		GetPhysics()->SetContents( 0 );
	}

	if ( gameLocal.isServer ) {

		ServerSendEvent( EVENT_BECOMEBROKEN, NULL, true, -1 );

		if ( spawnArgs.GetString( "def_damage", "", &damageDefName ) ) {
			idVec3 origin = renderEntity.origin + renderEntity.bounds.GetCenter() * renderEntity.axis;
			gameLocal.RadiusDamage( origin, activator, activator, this, this, damageDefName );
		}

	}

		ActivateTargets( activator );

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	// set the state parm
	renderEntity.shaderParms[ SHADERPARM_MODE ] = 1;
	renderLight.shaderParms[ SHADERPARM_MODE ] = 1;

	// if the light has a sound, either start the alternate (broken) sound, or stop the sound
	const char *parm = spawnArgs.GetString( "snd_broken" );
	if ( refSound.shader || ( parm && *parm ) ) {
		StopSound( SND_CHANNEL_ANY, false );
		const idSoundShader *alternate = refSound.shader ? refSound.shader->GetAltSound() : declManager->FindSound( parm );
		if ( alternate ) {
			// start it with no diversity, so the leadin break sound plays
			refSound.referenceSound->StartSound( alternate, SND_CHANNEL_ANY, 0.0, 0 );
		}
	}

	parm = spawnArgs.GetString( "mtr_broken" );
	if ( parm && *parm ) {
		SetShader( parm );
	}

	UpdateVisuals();
}

/*
================
idLight::PresentLightDefChange
================
*/
void idLight::PresentLightDefChange( void )
{
/*
DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s] Radius ( %0.3f / %0.3f / %03f )\r", this, name.c_str(),
		renderLight.lightRadius[0],		// x
		renderLight.lightRadius[1],		// y
		renderLight.lightRadius[2]);	// z
*/

	// let the renderer apply it to the world
	if ( ( lightDefHandle != -1 ) ) {
		gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
	} else {
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}
}

/*
================
idLight::PresentModelDefChange
================
*/
void idLight::PresentModelDefChange( void ) {

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
idLight::Present
================
*/
void idLight::Present( void ) {
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}

	// Clear the bounds, so idLight::PresentRenderTrigger() has a way to know
	// if idEntity::PresentRenderTrigger() added anything.
	m_renderTrigger.bounds.Clear();

	// add the model
	idEntity::Present();

	// current transformation
	renderLight.axis	= localLightAxis * GetPhysics()->GetAxis();
	renderLight.origin  = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis() * localLightOrigin;

	// reference the sound for shader synced effects
	if ( lightParent ) {
		renderLight.referenceSound = lightParent->GetSoundEmitter();
		renderEntity.referenceSound = lightParent->GetSoundEmitter();
	}
	else {
		renderLight.referenceSound = refSound.referenceSound;
		renderEntity.referenceSound = refSound.referenceSound;
	}

	// update the renderLight and renderEntity to render the light and flare
	PresentLightDefChange();
	PresentModelDefChange();
	PresentRenderTrigger();
}

/*
================
idLight::Think
================
*/
void idLight::Think( void ) {
	idVec4 color;

	if ( thinkFlags & TH_THINK ) {
		if ( fadeEnd > 0 ) {
			if ( gameLocal.time < fadeEnd ) {
				color.Lerp( fadeFrom, fadeTo, ( float )( gameLocal.time - fadeStart ) / ( float )( fadeEnd - fadeStart ) );
			} else {
				color = fadeTo;
				fadeEnd = 0;
				BecomeInactive( TH_THINK );
			}
			SetColor( color );
		}
	}

	RunPhysics();
	Present();
}

/*
================
idLight::GetPhysicsToSoundTransform
================
*/
bool idLight::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	origin = localLightOrigin + renderLight.lightCenter;
	axis = localLightAxis * GetPhysics()->GetAxis();
	return true;
}

/*
================
idLight::FreeLightDef
================
*/
void idLight::FreeLightDef( void ) {
	if ( lightDefHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
		lightDefHandle = -1;
	}
}

/*
================
idLight::SaveState
================
*/
void idLight::SaveState( idDict *args ) {
	int i, c = spawnArgs.GetNumKeyVals();
	for ( i = 0; i < c; i++ ) {
		const idKeyValue *pv = spawnArgs.GetKeyVal(i);
		if ( pv->GetKey().Find( "editor_", false ) >= 0 || pv->GetKey().Find( "parse_", false ) >= 0 ) {
			continue;
		}
		args->Set( pv->GetKey(), pv->GetValue() );
	}
}

/*
===============
idLight::ShowEditingDialog
===============
*/
void idLight::ShowEditingDialog( void ) {
	if ( g_editEntityMode.GetInteger() == 1 ) {
		common->InitTool( EDITOR_LIGHT, &spawnArgs );
	} else {
		common->InitTool( EDITOR_SOUND, &spawnArgs );
	}
}

/*
================
idLight::Event_SetShader
================
*/
void idLight::Event_SetShader( const char *shadername ) {
	SetShader( shadername );
}

/*
================
idLight::Event_GetLightParm
================
*/
void idLight::Event_GetLightParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	idThread::ReturnFloat( renderLight.shaderParms[ parmnum ] );
}

/*
================
idLight::Event_SetLightParm
================
*/
void idLight::Event_SetLightParm( int parmnum, float value ) {
	SetLightParm( parmnum, value );
}

/*
================
idLight::Event_SetLightParms
================
*/
void idLight::Event_SetLightParms( float parm0, float parm1, float parm2, float parm3 ) {
	SetLightParms( parm0, parm1, parm2, parm3 );
}

/*
================
idLight::Event_SetRadiusXYZ
================
*/
void idLight::Event_SetRadiusXYZ( float x, float y, float z ) {
	SetRadiusXYZ( x, y, z );
}

/*
================
idLight::Event_SetRadius
================
*/
void idLight::Event_SetRadius( float radius ) {
	SetRadius( radius );
}

/*
================
idLight::Event_Hide
================
*/
void idLight::Event_Hide( void ) {
	Hide();
	PresentModelDefChange();
	Off();
}

/*
================
idLight::Event_Show
================
*/
void idLight::Event_Show( void ) {
	Show();
	PresentModelDefChange();
	On();
}

/*
================
idLight::Event_On
================
*/
void idLight::Event_On( void ) {
	On();
}

/*
================
idLight::Event_Off
================
*/
void idLight::Event_Off( void ) {
	Off();
}

/*
================
idLight::Event_ToggleOnOff
================
*/
void idLight::Event_ToggleOnOff( idEntity *activator ) {
	triggercount++;
	if ( triggercount < count ) {
		return;
	}

	// reset trigger count
	triggercount = 0;

	if ( breakOnTrigger ) {
		BecomeBroken( activator );
		breakOnTrigger = false;
		return;
	}

	if ( !currentLevel ) {
		On();
	}
	else {
		currentLevel--;
		if ( !currentLevel ) {
			Off();
		}
		else {
			SetLightLevel();
		}
	}
}

/*
================
idLight::Event_SetSoundHandles

  set the same sound def handle on all targeted lights
================
*/
void idLight::Event_SetSoundHandles( void ) {
	int i;
	idEntity *targetEnt;

	if ( !refSound.referenceSound ) {
		return;
	}

	for ( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( targetEnt && targetEnt->IsType( idLight::Type ) ) {
			idLight	*light = static_cast<idLight*>(targetEnt);
			light->lightParent = this;

			// explicitly delete any sounds on the entity
			light->FreeSoundEmitter( true );

			// manually set the refSound to this light's refSound
			light->renderEntity.referenceSound = renderEntity.referenceSound;

			// update the renderEntity to the renderer
			light->UpdateVisuals();
		}
	}
}

/*
================
idLight::Event_FadeOut
================
*/
void idLight::Event_FadeOut( float time ) {
	FadeOut( time );
}

/*
================
idLight::Event_FadeIn
================
*/
void idLight::Event_FadeIn( float time ) {
	FadeIn( time );
}

/*
================
idLight::ClientPredictionThink
================
*/
void idLight::ClientPredictionThink( void ) {
	Think();
}

/*
================
idLight::WriteToSnapshot
================
*/
void idLight::WriteToSnapshot( idBitMsgDelta &msg ) const {

	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );

	msg.WriteByte( currentLevel );
	msg.WriteLong( PackColor( baseColor ) );
	// msg.WriteBits( lightParent.GetEntityNum(), GENTITYNUM_BITS );

/*	// only helps prediction
	msg.WriteLong( PackColor( fadeFrom ) );
	msg.WriteLong( PackColor( fadeTo ) );
	msg.WriteLong( fadeStart );
	msg.WriteLong( fadeEnd );
*/

	// FIXME: send renderLight.shader
	msg.WriteFloat( renderLight.lightRadius[0], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[1], 5, 10 );
	msg.WriteFloat( renderLight.lightRadius[2], 5, 10 );

	msg.WriteLong( PackColor( idVec4( renderLight.shaderParms[SHADERPARM_RED],
									  renderLight.shaderParms[SHADERPARM_GREEN],
									  renderLight.shaderParms[SHADERPARM_BLUE],
									  renderLight.shaderParms[SHADERPARM_ALPHA] ) ) );

	msg.WriteFloat( renderLight.shaderParms[SHADERPARM_TIMESCALE], 5, 10 );
	msg.WriteLong( renderLight.shaderParms[SHADERPARM_TIMEOFFSET] );
	//msg.WriteByte( renderLight.shaderParms[SHADERPARM_DIVERSITY] );
	msg.WriteShort( renderLight.shaderParms[SHADERPARM_MODE] );

	WriteColorToSnapshot( msg );
}

/*
================
idLight::ReadFromSnapshot
================
*/
void idLight::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	idVec4	shaderColor;
	int		oldCurrentLevel = currentLevel;
	idVec3	oldBaseColor = baseColor;

	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );

	currentLevel = msg.ReadByte();
	if ( currentLevel != oldCurrentLevel ) {
		// need to call On/Off for flickering lights to start/stop the sound
		// while doing it this way rather than through events, the flickering is out of sync between clients
		// but at least there is no question about saving the event and having them happening globally in the world
		if ( currentLevel ) {
			On();
		} else {
			Off();
		}
	}
	UnpackColor( msg.ReadLong(), baseColor );
	// lightParentEntityNum = msg.ReadBits( GENTITYNUM_BITS );	

/*	// only helps prediction
	UnpackColor( msg.ReadLong(), fadeFrom );
	UnpackColor( msg.ReadLong(), fadeTo );
	fadeStart = msg.ReadLong();
	fadeEnd = msg.ReadLong();
*/

	// FIXME: read renderLight.shader
	renderLight.lightRadius[0] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[1] = msg.ReadFloat( 5, 10 );
	renderLight.lightRadius[2] = msg.ReadFloat( 5, 10 );

	UnpackColor( msg.ReadLong(), shaderColor );
	renderLight.shaderParms[SHADERPARM_RED] = shaderColor[0];
	renderLight.shaderParms[SHADERPARM_GREEN] = shaderColor[1];
	renderLight.shaderParms[SHADERPARM_BLUE] = shaderColor[2];
	renderLight.shaderParms[SHADERPARM_ALPHA] = shaderColor[3];

	renderLight.shaderParms[SHADERPARM_TIMESCALE] = msg.ReadFloat( 5, 10 );
	renderLight.shaderParms[SHADERPARM_TIMEOFFSET] = msg.ReadLong();
	//renderLight.shaderParms[SHADERPARM_DIVERSITY] = msg.ReadFloat();
	renderLight.shaderParms[SHADERPARM_MODE] = msg.ReadShort();

	ReadColorFromSnapshot( msg );

	if ( msg.HasChanged() ) {
		if ( ( currentLevel != oldCurrentLevel ) || ( baseColor != oldBaseColor ) ) {
			SetLightLevel();
		} else {
			PresentLightDefChange();
			PresentModelDefChange();
		}
	}
}

/*
================
idLight::ClientReceiveEvent
================
*/
bool idLight::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_BECOMEBROKEN: {
			BecomeBroken( NULL );
			return true;
		}
		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
//	return false;
}


/**	Returns a bounding box surrounding the light.
 */
idBounds idLight::GetBounds()
{
	// NOTE: I need to add caching.

	idBounds b;

	if ( renderLight.pointLight )
	{
		b = idBounds( -renderLight.lightRadius, renderLight.lightRadius );
	} else {
		gameLocal.Warning("idLight::GetBounds() not correctly implemented for projected lights.");
		// Fake a set of bounds. This might work ok for squarish spotlights with no start/stop specified.
		// FIXME: These bounds are incorrect.
		b.Zero();
		b.AddPoint( renderLight.target );
		b.AddPoint( renderLight.target + renderLight.up + renderLight.right );
		b.AddPoint( renderLight.target + renderLight.up - renderLight.right );
		b.AddPoint( renderLight.target - renderLight.up + renderLight.right );
		b.AddPoint( renderLight.target - renderLight.up - renderLight.right );
	}

	return b;
}


/**	Called to update m_renderTrigger after the render entity is modified.
 *	Only updates the render trigger if a thread is waiting for it.
 */
void idLight::PresentRenderTrigger()
{
	if ( !m_renderWaitingThread ) {
		goto Quit;
	}

	// Have the bounds been set yet?
	if ( m_renderTrigger.bounds.IsCleared() )
	{
		// No.

		// Pre-rotate our bounds and give them to m_renderTrigger.
		m_renderTrigger.origin = renderLight.origin;
		m_renderTrigger.bounds = GetBounds() * renderLight.axis;

	} else {
		// Yes.

		// Convert our light's bounds to be relative to m_renderTrigger,
		// and add them to what already exists.
		m_renderTrigger.bounds += GetBounds() * renderLight.axis + ( renderLight.origin - m_renderTrigger.origin );
	}

	// I haven't yet figured out where renderEntity.entityNum is set...
	m_renderTrigger.entityNum = entityNumber;

	// Update the renderTrigger in the render world.
	if ( m_renderTriggerHandle == -1 )
		m_renderTriggerHandle = gameRenderWorld->AddEntityDef( &m_renderTrigger );
	else
		gameRenderWorld->UpdateEntityDef( m_renderTriggerHandle, &m_renderTrigger );

	Quit:
	return;
}


int idLight::GetTextureIndex(float x, float y, int w, int h, int bpp)
{
	int rc = 0;
	float w2, h2;

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Calculating texture index: x/y: %f/%f w/h: %u/%u\r", x, y, w, h);

	w2 = ((float)w)/2.0;
	h2 = ((float)h)/2.0;

	if(renderLight.pointLight)
	{
		// The lighttextures are spread out over the range of the axis. The z axis
		// can be ignored for this and we assume that the x/y is already properly
		// calculated.
		x = w2 - (w2/renderLight.lightRadius.x) * x;
		y = h2 - (h2/renderLight.lightRadius.y) * y;
	}
	else
	{
		// TODO: Just for now until the projected lights are implemented to not cause any crash.
		// x = right
		// y = up
#pragma message(DARKMOD_NOTE "-------------------------------------------------idLight::GetTextureIndex")
#pragma message(DARKMOD_NOTE "For projected lights this is likely not enough. As long as the light will")
#pragma message(DARKMOD_NOTE "be straight up or down it should work, but if the cone is angled it might")
#pragma message(DARKMOD_NOTE "give wrong results.")
#pragma message(DARKMOD_NOTE "-------------------------------------------------idLight::GetTextureIndex")
		x = w2 - (w2/renderLight.right.x) * x;
		y = h2 - (h2/renderLight.up.y) * y;
	}

	if(y > h)
		y = h;
	if(x > w)
		x = w;

	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;

	rc = ((int)y * (w*bpp)) + ((int)x*bpp);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Index result: %u   x/y: %f/%f\r", rc, x, y);

	return rc;
}


float idLight::GetDistanceColor(float fDistance, float fx, float fy)
{
	float fColVal(0), fImgVal(0);
	int fw(0), fh(0), iw(0), ih(0), i(0), fbpp(0), ibpp(0);
	unsigned char *img = NULL;
	unsigned char *fot = NULL;

	if(m_LightMaterial == NULL)
	{
		if((m_LightMaterial = g_Global.GetMaterial(m_MaterialName)) != NULL)
		{
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Material found for [%s]\r", name.c_str());
			fot = m_LightMaterial->GetFallOffTexture(fw, fh, fbpp);
			img = m_LightMaterial->GetImage(iw, ih, ibpp);
		}
	}
	else
	{
		fot = m_LightMaterial->GetFallOffTexture(fw, fh, fbpp);
		img = m_LightMaterial->GetImage(iw, ih, ibpp);
	}

	fColVal = (baseColor.x * DARKMOD_LG_RED + baseColor.y * DARKMOD_LG_GREEN + baseColor.z * DARKMOD_LG_BLUE);
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Pointlight: %u   Red: %f/%f    Green: %f/%f    Blue: %f/%f   ColVal: %f\r", renderLight.pointLight,
		baseColor.x, baseColor.x * DARKMOD_LG_RED,
		baseColor.y, baseColor.y * DARKMOD_LG_GREEN,
		baseColor.z, baseColor.z * DARKMOD_LG_BLUE,
		fColVal);

	// If we have neither falloff texture nor a projection image, we do a 
	// simple linear falloff
	if(fot == NULL && img == NULL)
	{
		// TODO: Light falloff calculation
#pragma message(DARKMOD_NOTE "------------------------------------------------idLight::GetDistanceColor")
#pragma message(DARKMOD_NOTE "The lightfalloff should be calculated for ellipsoids instead of spheres")
#pragma message(DARKMOD_NOTE "when no textures are defined. The current code will give wrong results")
#pragma message(DARKMOD_NOTE "when a light is defined as an ellipsoid.")
#pragma message(DARKMOD_NOTE "------------------------------------------------idLight::GetDistanceColor")
		fColVal = (fColVal / m_MaxLightRadius) * (m_MaxLightRadius - fDistance);
		fImgVal = 1;
		DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("No textures defined using distance: [%f]\r", fDistance);
	}
	else
	{
		// If we have a falloff texture ...
		if(fot != NULL)
		{
			i = GetTextureIndex((float)fabs(fx), (float)fabs(fy), fw, fh, fbpp);
			fColVal = fColVal * (fot[i] * DARKMOD_LG_SCALE);
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Falloff: Index: %u   Value: %u [%f]\r", i, (int)fot[i], (float)(fot[i] * DARKMOD_LG_SCALE));
		}
		else
			fColVal = 1;

		// ... or a projection image.
		if(img != NULL)
		{
			i = GetTextureIndex((float)fabs(fx), (float)fabs(fy), iw, ih, ibpp);
			fImgVal = img[i] * DARKMOD_LG_SCALE;
			DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Map: Index: %u   Value: %u [%f]\r", i, (int)img[i], (float)(img[i] * DARKMOD_LG_SCALE));
		}
		else
			fImgVal = 1;
	}

	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("Final ColVal: %f   ImgVal: %f\r", fColVal, fImgVal);

	return (fColVal*fImgVal);
}

bool idLight::CastsShadow(void)
{
	if(m_LightMaterial == NULL)
		m_LightMaterial = g_Global.GetMaterial(m_MaterialName);

	if(m_LightMaterial != NULL)
	{
		if(m_LightMaterial->m_AmbientLight == true)
			return false;
	}

	return !renderLight.noShadows; 
}

bool idLight::GetLightCone(idVec3 &Origin, idVec3 &Target, idVec3 &Right, idVec3 &Up, idVec3 &Start, idVec3 &End)
{
	bool rc = false;

	Origin = GetPhysics()->GetOrigin();
	Target = renderLight.target;
	Right = renderLight.right;
	Up = renderLight.up;

	Start = renderLight.start;
	End = renderLight.end;

	return rc;
}

bool idLight::GetLightCone(idVec3 &Origin, idVec3 &Axis, idVec3 &Center)
{
	bool rc = false;

	Origin = GetPhysics()->GetOrigin();
	Axis = renderLight.lightRadius;
	Center = renderLight.lightCenter;

	return rc;
}

void idLight::Event_SetLightOrigin( idVec3 &pos )
{
	localLightOrigin = pos;
	BecomeActive( TH_UPDATEVISUALS );
}

void idLight::Event_GetLightOrigin( void )
{
	idThread::ReturnVector( localLightOrigin );
}

void idLight::Event_GetLightLevel ( void )
{
	idThread::ReturnFloat( currentLevel );
}

/**	Returns 1 if the light is in PVS.
 *	Doesn't take into account vis-area optimizations for shadowcasting lights.
 */
void idLight::Event_InPVS()
// NOTE: Current extremely inefficent.
// Caching needs to be done.
{
	int localNumPVSAreas, localPVSAreas[32];
	idBounds modelAbsBounds;

	m_renderTrigger.bounds = GetBounds(); // currently too innefficient but I'll worry about caching later
	m_renderTrigger.origin = renderLight.origin;
	m_renderTrigger.axis = renderLight.axis;

	modelAbsBounds.FromTransformedBounds( m_renderTrigger.bounds, m_renderTrigger.origin, m_renderTrigger.axis );
	localNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );

	idThread::ReturnFloat( gameLocal.pvs.InCurrentPVS( gameLocal.GetPlayerPVS(), localPVSAreas, localNumPVSAreas ) );
}


