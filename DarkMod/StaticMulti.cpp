// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4071 $
 * $Date: 2010-07-18 15:57:08 +0200 (Sun, 18 Jul 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod Team)

/*
   StaticMulti - a variant of func_static that can use a idPhys_StaticMulti
   				 for the clipmodel, e.g. has more than one clipmodel.
				 Used for entities with megamodels as rendermodel.
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: StaticMulti.cpp 4071 2010-07-18 13:57:08Z tels $", init_version);

#include "StaticMulti.h"

// if defined, draw debug output
// #define M_DEBUG 1

CLASS_DECLARATION( idStaticEntity, CStaticMulti )
	EVENT( EV_Activate,				CStaticMulti::Event_Activate )
END_CLASS

/*
===============
CStaticMulti::CStaticMulti
===============
*/
CStaticMulti::CStaticMulti( void )
{
	active = false;
	m_LOD = NULL;
	m_MegaModel = NULL;
	m_DistCheckTimeStamp = 0;
	m_DistCheckInterval = 0;
	m_fHideDistance = 0.0f;
	m_bDistCheckXYOnly = false;
}

CStaticMulti::~CStaticMulti()
{
	// mark as inactive and remove changes because the entity will be no longer existing
	if (m_MegaModel)
	{
		m_MegaModel->StopUpdating();
	}

	// no need to free these as they are just ptr to a copy
	m_LOD = NULL;
	m_MegaModel = NULL;

	// avoid freeing the already combined model (we can re-use it on respawn)
	renderEntity.hModel = NULL;

	// Avoid freeing the combined physics (clip)model
	SetPhysics(NULL);
}

/*
===============
CStaticMulti::Spawn
===============
*/
void CStaticMulti::Spawn( void )
{
	bool solid = spawnArgs.GetBool( "solid" );

	// ishtvan fix : Let clearing contents happen naturally on Hide instead of
	// checking hidden here and clearing contents prematurely
	if ( solid )
	{
		GetPhysics()->SetContents( CONTENTS_SOLID | CONTENTS_OPAQUE );
	}

	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );

	int d = (int) (1000.0f * spawnArgs.GetFloat( "dist_check_period", "0" ));
	if (d <= 0)
	{
		d = 0;
	}
	else
	{
		m_bDistCheckXYOnly = spawnArgs.GetBool( "dist_check_xy", "0" );
		m_DistCheckInterval = d;
		m_DistCheckTimeStamp = gameLocal.time - (int) (m_DistCheckInterval * (1.0f + gameLocal.random.RandomFloat()) );
		m_fHideDistance = spawnArgs.GetFloat( "hide_distance", "0.0" );
	}
}

/*
================
CStaticMulti::ThinkAboutLOD
================
*/
float CStaticMulti::ThinkAboutLOD( const lod_data_t *m_LOD, const float deltaSq ) 
{
	// ignored m_LOD

	// give our MegaModel a chance to update the rendermodel based on distance
	// to player

	//gameLocal.Warning("ThinkAboutLOD CStaticMulti called with m_LOD %p deltaSq %0.2f", m_LOD, deltaSq);

	return 1.0f;
}

/*
================
CStaticMulti::SetLODData

Store the data like our megamodel (the visible combined rendermodel including data how to
assemble it), and the LOD stages (contain the distance for each LOD stage)
================
*/
void CStaticMulti::SetLODData( CMegaModel* megaModel, lod_data_t *LOD)
{
	m_MegaModel = megaModel;
	m_LOD = LOD;
}

/*
================
CStaticMulti::Think
================
*/
void CStaticMulti::Think( void ) 
{
	// Distance dependence checks
	if ( (gameLocal.time - m_DistCheckTimeStamp) >= m_DistCheckInterval ) 
		{
		m_DistCheckTimeStamp = gameLocal.time;

		idVec3 delta = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		if( m_bDistCheckXYOnly )
		{
			idVec3 vGravNorm = GetPhysics()->GetGravityNormal();
			delta -= (vGravNorm * delta) * vGravNorm;
		}

		// multiply with the user LOD bias setting, and return the result:
		// floor the value to avoid inaccurancies leading to toggling when the player stands still:
		float deltaSq = idMath::Floor( delta.LengthSqr() / (cv_lod_bias.GetFloat() * cv_lod_bias.GetFloat()) );

		ThinkAboutLOD( NULL, deltaSq );
	}

	// Make ourselves visible and known:
	Present();

#ifdef M_DEBUG
	int num = GetPhysics()->GetNumClipModels();

   	idVec4 markerColor (0.3, 0.8, 1.0, 1.0);
   	idVec3 arrowLength (0.0, 0.0, 50.0);

	idPhysics *p = GetPhysics();

	// DEBUG draw arrows for each part of the physics object
	for (int i = 0; i < num; i++)
	{
		idVec3 org = p->GetOrigin( i );
	    gameRenderWorld->DebugArrow
			(
			markerColor,
			org + arrowLength,
			org,
			3,
	    	1 );
	}
#endif
}

void CStaticMulti::Save( idSaveGame *savefile ) const {

	// TODO: Save MegaModel, then remove this from the LODE
	savefile->WriteBool( active );
	savefile->WriteStaticObject( physics );
	savefile->WriteInt( m_DistCheckInterval );
	savefile->WriteInt( m_DistCheckTimeStamp );
	savefile->WriteBool( m_bDistCheckXYOnly );
}

void CStaticMulti::Restore( idRestoreGame *savefile ) {

	savefile->ReadBool( active );
	savefile->ReadStaticObject( physics );
	RestorePhysics( &physics );
	savefile->ReadInt( m_DistCheckInterval );
	savefile->ReadInt( m_DistCheckTimeStamp );
	savefile->ReadBool( m_bDistCheckXYOnly );
}

/*
================
CStaticMulti::Event_Activate
================
*/
void CStaticMulti::Event_Activate( idEntity *activator ) {

	int spawnTime = gameLocal.time;

	active = !active;

	const idKeyValue *kv = spawnArgs.FindKey( "hide" );
	if ( kv ) {
		if ( IsHidden() ) {
			Show();
		} else {
			Hide();
		}
	}

	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( spawnTime );
	renderEntity.shaderParms[5] = active;
	// this change should be a good thing, it will automatically turn on 
	// lights etc.. when triggered so that does not have to be specifically done
	// with trigger parms.. it MIGHT break things so need to keep an eye on it
	renderEntity.shaderParms[ SHADERPARM_MODE ] = ( renderEntity.shaderParms[ SHADERPARM_MODE ] ) ?  0.0f : 1.0f;
	BecomeActive( TH_UPDATEVISUALS );
}

