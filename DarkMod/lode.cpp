// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3981 $
 * $Date: 2010-06-25 07:27:18 +0200 (Fri, 25 Jun 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod Team)

/*
Level Of Detail Entities
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: lode.cpp 3981 2010-06-25 05:27:18Z tels $", init_version);

#include "../game/game_local.h"
#include "lode.h"

const idEventDef EV_Deactivate( "deactivate", "e" );
const idEventDef EV_CullAll( "cullAll", "" );

/*
   Lode

   Entity that spawns/culls other entities, but is invisible on its own.

===============================================================================
*/

CLASS_DECLARATION( idStaticEntity, Lode )
	EVENT( EV_Activate,				Lode::Event_Activate )
END_CLASS

/*
===============
Lode::Lode
===============
*/
Lode::Lode( void ) {

	active = false;

	m_fCullRange = 0.0f;
	m_iSeed = 3;
	m_iScore = 0;

	m_bPrepared = false;
	m_Entities.Clear();

	m_DistCheckTimeStamp = 0;
	m_DistCheckInterval = 0.5f;
	m_bDistCheckXYOnly = false;
}

/*
===============
Lode::Save
===============
*/
void Lode::Save( idSaveGame *savefile ) const {

	savefile->WriteBool( active );

	savefile->WriteFloat( m_fCullRange );
	savefile->WriteInt( m_iSeed );
	savefile->WriteInt( m_iScore );
	savefile->WriteInt( m_iNumEntities );

    savefile->WriteInt( m_DistCheckTimeStamp );
	savefile->WriteInt( m_DistCheckInterval );
	savefile->WriteBool( m_bDistCheckXYOnly );

	savefile->WriteInt( m_Entities.Num() );
	for( int i = 0; i < m_Entities.Num(); i++ )
	{
		savefile->WriteString( m_Entities[i].skin );
		savefile->WriteVec3( m_Entities[i].origin );
		savefile->WriteAngles( m_Entities[i].angles );
		savefile->WriteBool( m_Entities[i].hidden );
		savefile->WriteBool( m_Entities[i].exists );
		savefile->WriteInt( m_Entities[i].entity );
		savefile->WriteInt( m_Entities[i].classIdx );
	}

	savefile->WriteInt( m_Classes.Num() );
	for( int i = 0; i < m_Classes.Num(); i++ )
	{
		savefile->WriteString( m_Classes[i].classname );
		savefile->WriteInt( m_Classes[i].score );
		savefile->WriteFloat( m_Classes[i].cullDist );
		savefile->WriteFloat( m_Classes[i].spawnDist );
		savefile->WriteVec3( m_Classes[i].origin );
		savefile->WriteInt( m_Classes[i].nocollide );
		savefile->WriteBool( m_Classes[i].floor );
		savefile->WriteBool( m_Classes[i].stack );
		savefile->WriteBounds( m_Classes[i].bounds );
	}
}

/*
===============
Lode::Restore
===============
*/
void Lode::Restore( idRestoreGame *savefile ) {
	int num;

	savefile->ReadBool( active );

	savefile->ReadFloat( m_fCullRange );
	savefile->ReadInt( m_iSeed );
	savefile->ReadInt( m_iScore );
	savefile->ReadInt( m_iNumEntities );
	
    savefile->ReadInt( m_DistCheckTimeStamp );
	savefile->ReadInt( m_DistCheckInterval );
	savefile->ReadBool( m_bDistCheckXYOnly );

    savefile->ReadInt( num );
	assert( num == m_Entities.Num() );
	for( int i = 0; i < num; i++ ) {
		savefile->ReadString( m_Entities[i].skin );
		savefile->ReadVec3( m_Entities[i].origin );
		savefile->ReadAngles( m_Entities[i].angles );
		savefile->ReadBool( m_Entities[i].hidden );
		savefile->ReadBool( m_Entities[i].exists );
		savefile->ReadInt( m_Entities[i].entity );
		savefile->ReadInt( m_Entities[i].classIdx );
	}

    savefile->ReadInt( num );
	assert( num == m_Classes.Num() );
	for( int i = 0; i < num; i++ ) {
		savefile->ReadString( m_Classes[i].classname );
		savefile->ReadString( m_Classes[i].skin );
		savefile->ReadInt( m_Classes[i].score );
		savefile->ReadFloat( m_Classes[i].cullDist );
		savefile->ReadFloat( m_Classes[i].spawnDist );
		savefile->ReadVec3( m_Classes[i].origin );
		savefile->ReadInt( m_Classes[i].nocollide );
		savefile->ReadBool( m_Classes[i].floor );
		savefile->ReadBool( m_Classes[i].stack );
		savefile->ReadBounds( m_Classes[i].bounds );
	}
}

/*
===============
Lode::RandomFloat
===============
*/
ID_INLINE float Lode::RandomFloat( void ) {
	unsigned long i;
	m_iSeed = 1664525L * m_iSeed + 1013904223L;
	i = Lode::IEEE_ONE | ( m_iSeed & Lode::IEEE_MASK );
	return ( ( *(float *)&i ) - 1.0f );
}

/*
===============
Lode::Spawn
===============
*/
void Lode::Spawn( void ) {
	// the Lode itself is sneaky and hides itsel
	Hide();

	// And is nonsolid, too!
	GetPhysics()->SetContents( 0 );

	active = true;

	m_DistCheckInterval = (int) (1000.0f * spawnArgs.GetFloat( "dist_check_period", "0" ));

	// do we have a cull range?
	m_fCullRange = spawnArgs.GetFloat( "lod_cull_range", "0.0" );

	gameLocal.Printf (" Lode %s: cull range = %0.2f.\n", GetName(), m_fCullRange );

	m_bDistCheckXYOnly = spawnArgs.GetBool( "dist_check_xy", "0" );

	// add some phase diversity to the checks so that they don't all run in one frame
	// make sure they all run on the first frame though, by initializing m_TimeStamp to
	// be at least one interval early.
	m_DistCheckTimeStamp = gameLocal.time - (int) (m_DistCheckInterval * (1.0f + gameLocal.random.RandomFloat()) );

	// Have to start thinking
	BecomeActive( TH_THINK );
}

/*
===============
Create the places for all entities that we control
===============
*/
void Lode::Prepare( void ) {
	lode_class_t			LodeClass;
	int						numEntities;

	if ( targets.Num() == 0 )
	{
		gameLocal.Warning( "LODE %s has no targets!", GetName() );
		BecomeInactive( TH_THINK );
		return;
	}

	// Gather all targets and make a note of them, also summing their "lod_score" up
	m_iScore = 0;
	m_Classes.Clear();
	for( int i = 0; i < targets.Num(); i++ )
	{
		idEntity *ent = targets[ i ].GetEntity();

	   	if ( ent )
		{
			int iEntScore = ent->spawnArgs.GetInt( "lod_score", "1" );
			if (iEntScore < 0)
			{
				gameLocal.Warning( "LODE %s: Target %s has invalid lod_score %i!\n", GetName(), ent->GetName(), iEntScore );
			}
			else
			{
				// add this entity
				m_iScore += iEntScore;
				LodeClass.score = iEntScore;
				LodeClass.classname = ent->GetEntityDefName();
				LodeClass.skin = ent->spawnArgs.GetString( "skin", "" );
				LodeClass.spawnDist = 0;
				// Do not use GetPhysics()->GetOrigin(), as the LOD system might have shifted
				// the entity already between spawning and us querying the info:
				LodeClass.origin = ent->spawnArgs.GetVector( "origin" );
				LodeClass.floor = ent->spawnArgs.GetBool( "lode_floor", "0" );
				LodeClass.stack = ent->spawnArgs.GetBool( "lode_stack", "1" );
				LodeClass.nocollide	= NOCOLLIDE_ATALL;
				// TODO: place at origin
				LodeClass.bounds = ent->GetRenderEntity()->bounds;
				LodeClass.cullDist = ent->spawnArgs.GetFloat( "hide_distance", "0" );
				if (LodeClass.cullDist > 0)
				{
					LodeClass.cullDist += m_fCullRange;
					LodeClass.spawnDist = LodeClass.cullDist - (m_fCullRange / 2);
					// square for easier compare
					LodeClass.cullDist *= LodeClass.cullDist;
					LodeClass.spawnDist *= LodeClass.spawnDist;
				}
				m_Classes.Append ( LodeClass );
				gameLocal.Printf( "LODE %s: Adding class %s.\n", GetName(), LodeClass.classname.c_str() );
			}
		}
	}

	m_iNumEntities = spawnArgs.GetInt( "entity_count", "100" );

	if (m_iNumEntities <= 0)
	{
		gameLocal.Warning( "LODE %s: entity count is invalid: %i!\n", GetName(), m_iNumEntities );
		// TODO: compute entity count dynamically from area that we cover
	}

	gameLocal.Printf( "LODE %s: Overall score: %i.\n", GetName(), m_iScore );

	// remove all our targets from the game
	for( int i = 0; i < targets.Num(); i++ )
	{
		idEntity *ent = targets[ i ].GetEntity();
		if (ent)
		{
			// TODO: SafeRemove?
			ent->PostEventMS( &EV_Remove, 50 );
		}
	}
	targets.Clear();

	PrepareEntities();
}

void Lode::PrepareEntities( void )
{
	idVec3					origin;
	lode_entity_t			LodeEntity;
	idBounds				testBounds;		// to test whether the translated/rotated entity is inside the LODE

	m_Entities.Clear();

	// re-init the seed so that we get exactly the same sequence every time
	m_iSeed = spawnArgs.GetFloat( "seed", "3" );

	// To compute positions
	idBounds bounds = renderEntity.bounds;
	// Use a sphere with that radious to find random position, then clip
	// any outside our bounds, this makes it also worked with rotated LODE:
	float radius = bounds.GetRadius() + 0.01f;

	// Compute random positions for all entities that we want to spawn
	// for each class
	origin = GetPhysics()->GetOrigin();
	for (int i = 0; i < m_Classes.Num(); i++)
	{
		int iEntities = m_iNumEntities * (static_cast<float>(m_Classes[i].score)) / m_iScore;	// sum 2, this one 1 => 50% of this class
		gameLocal.Printf( "LODE %s: Creating %i entities of class %s.\n", GetName(), iEntities, m_Classes[i].classname.c_str() );
		for (int j = 0; j < iEntities; j++)
		{
			int tries = 0;
			while (tries++ < 10)
			{
				// TODO: allow the "floor" direction be set via spawnarg
				// TODO: use bounds of LODE entity
				LodeEntity.origin = origin + idVec3( RandomFloat() * radius, RandomFloat() * radius, 0 );
				if (m_Classes[i].floor)
				{
					// TODO: spawn entity, use GetFloorPos(); reposition, then hide?
					gameLocal.Printf( "LODE %s: Flooring entity %i.\n", GetName(), j );
				}
				else
				{
					// just use the Z axis from the editor pos
					LodeEntity.origin.z = m_Classes[i].origin.z;
				}
				// randomly rotate
				LodeEntity.angles = idAngles( 0, RandomFloat() * 359.9, 0 );

				// inside LODE bounds?
				testBounds = (m_Classes[i].bounds + LodeEntity.origin) * LodeEntity.angles.ToMat3();
				//if (bounds.IntersectsBounds( testBounds ))
				if (3 < 5)
				{
					gameLocal.Printf( "LODE %s: Found valid position for entity %i.\n", GetName(), j );
					break;
				}
			}
			// couldn't place entity even after 10 tries?
			if (tries >= 10) continue;

			// check that the entity does not collide with any other entity
			if (m_Classes[i].nocollide > 0)
			{
				/*
				for (int k = 0; k < iEntities; k++)
				{
					// TODO: take the bounds of the entity class, translate/rotate it to the
					// position of iEntites[k] and call IntersectsBounds( )
					testBounds = (m_Classes[i].bounds + m_Entities[k].origin) * m_Entities[k].angles.ToMat3();
				}
				*/

			}

			// TODO: choose skin randomly
			LodeEntity.skin = m_Classes[i].skin;
			// will be automatically spawned when we are in range
			LodeEntity.hidden = true;
			LodeEntity.exists = false;
			LodeEntity.entity = 0;
			LodeEntity.classIdx = i;
			m_Entities.Append( LodeEntity );
		}
	}
}

/*
================
Lode::Think
================
*/
void Lode::Think( void ) 
{
	struct lode_entity_t* ent;
	struct lode_class_t*  lclass;

	// tels: neccessary?
	// idEntity::Think();

	// haven't initialized entities yet?
	if (!m_bPrepared)
	{
		Prepare();
		m_bPrepared = true;
		if (m_Entities.Num() == 0)
		{
			// could not create any entities?
			gameLocal.Printf( "LODE %s: Have no entities to control, becoming inactive.\n", GetName() );
			// Tels: Does somehow not work, bouncing us again and again into this branch?
			BecomeInactive(TH_THINK);
			return;
		}
	}

	idVec3 playerOrigin = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	idVec3 vGravNorm = GetPhysics()->GetGravityNormal();
	float lodBias = cv_lod_bias.GetFloat();

	// square to avoid taking the square root from the distance
	lodBias *= lodBias;

	// Distance dependence checks
	if( (gameLocal.time - m_DistCheckTimeStamp) > m_DistCheckInterval )
	{

		m_DistCheckTimeStamp = gameLocal.time;

		// for each of our "entities", do the distance check
		for (int i = 0; i < m_Entities.Num(); i++)
		{

			// TODO: let all auto-generated entities know about their new distance

			// TODO: What to do about player looking thru spyglass?
			idVec3 delta = playerOrigin - m_Entities[i].origin;

			if( m_bDistCheckXYOnly )
			{
				delta -= (delta * vGravNorm) * vGravNorm;
			}

			// multiply with the user LOD bias setting, and cache that result:
			float deltaSq = delta.LengthSqr() / lodBias;

			// normal distance checks now

			ent = &m_Entities[i];
			lclass = &(m_Classes[ ent->classIdx ]);
			if (!ent->exists && deltaSq < lclass->spawnDist)
			{
				// spawn the entity and note its number
				gameLocal.Printf( "LODE %s: Spawning entity #%i (%s).\n", GetName(), i, lclass->classname.c_str() );

				const char* pstr_DefName = lclass->classname.c_str();
				const idDict *p_Def = gameLocal.FindEntityDefDict( pstr_DefName, false );
				if( p_Def )
				{
					idEntity *ent2;
					idDict args;

					args.Set("classname", lclass->classname);
					// move to right place
					args.SetVector("origin", ent->origin );
					// TODO: spawn as hidden, then later unhide them via LOD code
					//args.Set("hide", "1");
					// disable LOD checks on entities (we take care of this)
					args.Set("dist_check_period", "0");

					gameLocal.SpawnEntityDef( args, &ent2 );
					if (ent2)
					{
						gameLocal.Printf( "LODE %s: Spawned entity #%i (%s) at  %0.2f, %0.2f, %0.2f.\n",
								GetName(), i, lclass->classname.c_str(), ent->origin.x, ent->origin.y, ent->origin.z );
						ent->exists = true;
						ent->entity = ent2->entityNumber;
						// and rotate
						ent2->SetAxis( ent->angles.ToMat3() );
						ent2->BecomeInactive( TH_THINK );
						//ent2->DisableLOD();
					}
				}
			}	
			else
			{
				// cull entities that are outside "hide_distance + fade_out_distance + m_fCullRange
				if (ent->exists && deltaSq > lclass->cullDist)
				{
					idEntity *ent2 = gameLocal.entities[ ent->entity ];
					if (ent2)
					{
						// TODO: SafeRemve?
						ent2->PostEventMS( &EV_Remove, 0 );
					}

					// cull (remove) the entity
					gameLocal.Printf( "LODE %s: Culling entity #%i.\n", GetName(), i );
					ent->exists = false;
				}
			}
		}
	}
}

/*
================
Lode::Event_Activate
================
*/
void Lode::Event_Activate( idEntity *activator ) {

	active = true;
	BecomeActive(TH_THINK);
}

/*
================
Lode::Event_Deactivate
================
*/
void Lode::Event_Deactivate( idEntity *activator ) {

	active = false;
	BecomeInactive(TH_THINK);
}

/*
================
Lode::Event_CullAll
================
*/
void Lode::Event_CullAll( void ) {
	struct lode_entity_t* ent;

	for (int i = 0; i < m_Entities.Num(); i++)
	{
		ent = &m_Entities[i];
		if (ent->exists)
		{
			idEntity *ent2 = gameLocal.entities[ ent->entity ];
			if (ent2)
			{
				// TODO: SafeRemve?
				ent2->PostEventMS( &EV_Remove, 0 );
			}

			// cull (remove) the entity
			gameLocal.Printf( "LODE %s: Culling entity #%i.\n", GetName(), i );
			ent->exists = false;
		}
	}	
}

