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

	m_Changes.Clear();
	m_Offsets = NULL;
	m_hModel = NULL;
	m_modelName = "";

	m_iMaxChanges = 25;
	
	m_DistCheckTimeStamp = 0;
	m_DistCheckInterval = 0;
	m_fHideDistance = 0.0f;
	m_bDistCheckXYOnly = false;
}

CStaticMulti::~CStaticMulti()
{
	// no need to free these as they are just ptr to a copy
	m_LOD = NULL;

	// Avoid freeing the combined physics (clip)model as we can re-use it on next respawn:
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
CStaticMulti::SetLODData

Store the data like our megamodel (the visible combined rendermodel including data how to
assemble it), and the LOD stages (contain the distance for each LOD stage)
================
*/
void CStaticMulti::SetLODData( lod_data_t *LOD, idStr modelName, idList<model_ofs_t>* offsets, idStr materialName, const idRenderModel* hModel )
{
	active = true;

	m_LOD = LOD;
	m_Offsets = offsets;
	m_MaterialName = materialName;

	// if we need to combine from a func_static, store a ptr to it's renderModel
	m_hModel = hModel;

	// in case it doesn't have LOD
	m_modelName = modelName;

	m_Changes.Clear();
	// avoid frequent resizes
	m_Changes.SetGranularity(32);

	UpdateRenderModel(true);
}

/*
===============
CStaticMulti::UpdateRenderModel

Updates the rendermodel if nec. and returns true if it was updated.
===============
*/
bool CStaticMulti::UpdateRenderModel( const bool force )
{
	if ( !force && 
		(active == false ||
		m_Changes.Num() < m_iMaxChanges ) )
	{
		// no update nec. yet:
		return false;
	}

	idVec3 origin = GetPhysics()->GetOrigin();
	gameLocal.Printf("%s updating renderModel at %s.\n", GetName(), origin.ToString());

	int n = m_Changes.Num();

	// apply all our changes to the offsets list
	for (int i = 0; i < n; i++)
	{
		m_Offsets->Ptr()[ m_Changes[i].entity ].lod = m_Changes[i].newLOD;
	}

	// now clear the changes
	m_Changes.Clear();

	// compute a list of rendermodels
	idList< const idRenderModel*> LODs;
	// default model
	if (m_hModel)
	{
		LODs.Append(m_hModel);
	}
	else
	{
		idStr m = m_modelName;
		if (m_LOD)
		{
			m = m_LOD->ModelLOD[0];
		}
		const idRenderModel* hModel = renderModelManager->FindModel( m );
		LODs.Append(hModel);
	}

	for (int i = 0; i < LOD_LEVELS; i ++)
	{
		// func_static as source?
		if (m_hModel)
		{
			LODs.Append(m_hModel);
			continue;
		}
		idStr m = m_modelName;
		if (m_LOD)
		{
			m = m_LOD->ModelLOD[0];
		}
		const idRenderModel* hModel = renderModelManager->FindModel( m );
		LODs.Append(hModel);
	}
	const idList< const idRenderModel*> *l = &LODs;

	const idMaterial* m = NULL;
	if (!m_MaterialName.IsEmpty() )
	{
		declManager->FindMaterial( m_MaterialName, false );
	}

	// TODO: this might not work?
	if (renderEntity.hModel)
	{
		renderModelManager->FreeModel( renderEntity.hModel );
	}

	renderEntity.hModel = gameLocal.m_ModelGenerator->DuplicateLODModels( l, "megamodel", m_Offsets, &origin, m);

	Present();

	return true;
}

/*
================
CStaticMulti::Think
================
*/
void CStaticMulti::Think( void ) 
{
	// Distance dependence checks
	if ( active && (gameLocal.time - m_DistCheckTimeStamp) >= m_DistCheckInterval ) 
	{
		m_DistCheckTimeStamp = gameLocal.time;

		idVec3 origin = GetPhysics()->GetOrigin();

//		gameLocal.Printf("%s thinking at %s.\n", GetName(), origin.ToString());

		// TODO: skip this if the distance to the player has not changed

		// Calculate the offset for each model position alone, so precompute certain constants:
		float lod_bias = cv_lod_bias.GetFloat(); lod_bias *= lod_bias;
		idVec3 playerOrigin = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
		idVec3 gravityNormal = GetPhysics()->GetGravityNormal();

		// TODO: go through all offsets and calculate the new LOD stage
		int num = m_Offsets->Num();
		for (int i = 0; i < num; i++)
		{
		}

		// update the render model if nec.
		UpdateRenderModel();
	}

	// Make ourselves visible and known:
	// TODO: Do we need to do this every Think()?
	Present();

#ifdef M_DEBUG
	idPhysics *p = GetPhysics();
   	idVec4 markerColor (0.3, 0.8, 1.0, 1.0);
   	idVec3 arrowLength (0.0, 0.0, 50.0);

	// our center
	idVec3 org = renderEntity.origin;
    gameRenderWorld->DebugArrow
			(
			markerColor,
			org + arrowLength,
			org,
			3,
	    	1 );
	}
	int num = p->GetNumClipModels();

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

	savefile->WriteBool( active );
	savefile->WriteStaticObject( physics );
	savefile->WriteInt( m_DistCheckInterval );
	savefile->WriteInt( m_DistCheckTimeStamp );
	savefile->WriteBool( m_bDistCheckXYOnly );
	savefile->WriteString( m_MaterialName );
	savefile->WriteString( m_modelName );

	// TODO: is it faster to Save/Restore this, or just recreate it from
	// scratch? Saving it also might waste a lot of space in the savegame:
//	savefile->WriteModel( m_hModel );

	// Don't need to save the offsets, they are saved with the PseudoClass
/*	savefile->WriteInt( m_Offsets.Num() );
	for (int i = 0; i < m_Offsets.Num(); i++ )
	{
		savefile->WriteVec3( m_Offsets[i].offset );
		savefile->WriteVec3( m_Offsets[i].scale );
		savefile->WriteAngles( m_Offsets[i].angles );
		savefile->WriteInt( m_Offsets[i].lod );
		savefile->WriteInt( m_Offsets[i].flags );
		savefile->WriteUnsignedInt( m_Offsets[i].color );
	}
*/
	savefile->WriteInt( m_Changes.Num() );
	for (int i = 0; i < m_Changes.Num(); i++ )
	{
		savefile->WriteInt( m_Changes[i].entity );
		savefile->WriteInt( m_Changes[i].oldLOD );
		savefile->WriteInt( m_Changes[i].newLOD );
	}
}

void CStaticMulti::Restore( idRestoreGame *savefile )
{
	int n;

	savefile->ReadBool( active );
	savefile->ReadStaticObject( physics );
	RestorePhysics( &physics );
	savefile->ReadInt( m_DistCheckInterval );
	savefile->ReadInt( m_DistCheckTimeStamp );
	savefile->ReadBool( m_bDistCheckXYOnly );
	savefile->ReadString( m_MaterialName );
	savefile->ReadString( m_modelName );

//	savefile->ReadModel( m_hModel );

/*	m_Offsets.Clear();

	savefile->ReadInt( n );
	m_Offsets.SetGranularity(64);
	m_Offsets.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadVec3( m_Offsets[i].offset );
		savefile->ReadVec3( m_Offsets[i].scale );
		savefile->ReadAngles( m_Offsets[i].angles );
		savefile->ReadInt( m_Offsets[i].lod );
		savefile->ReadInt( m_Offsets[i].flags );
		savefile->ReadUnsignedInt( m_Offsets[i].color );
	}
*/

	// need a way to restore this and the rendermodel!
	m_Offsets = NULL;

	m_Changes.Clear();
	savefile->ReadInt( n );
	m_Changes.SetGranularity(64);
	m_Changes.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadInt( m_Changes[i].entity );
		savefile->ReadInt( m_Changes[i].oldLOD );
		savefile->ReadInt( m_Changes[i].newLOD );
	}

	// recompute our combined model
	UpdateRenderModel( true);

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

/*
===============
CStaticMulti::AddChange
===============
*/
void CStaticMulti::AddChange( const int entity, const int newLOD ) {

	// go through our changes and see if we already have one for this entity
	int n = m_Changes.Num();

	for (int i = 0; i < n; i++)
	{
		if (m_Changes[i].entity == entity)
		{
			// If the new change changes the model back to what it already is, remove
			// the change set:
			if ( m_Changes[i].oldLOD == newLOD )
			{
				// TODO: m_Changes.RemoveIndex(i,false);
				m_Changes.RemoveIndex(i);
			}
			else
			{
				// keep the change set with the new value
				m_Changes[i].newLOD = newLOD;
			}
			// TODO: move changed change sets to the front of the list, based
			// on the assumption they might change back again when the player
			// moves forth and back?

			// done
			return;
		}
	}

	// found no change set, add one
	model_changeinfo_t change;

	change.entity = entity;
	change.oldLOD = m_Offsets->Ptr()[ entity ].lod;
	change.newLOD = newLOD;

	m_Changes.Append( change );

	// done	
}

