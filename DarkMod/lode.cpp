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

// maximum number of tries to place an entity
#define MAX_TRIES 16

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

	m_iNumEntities = 0;
	m_iNumExisting = 0;
	m_iNumVisible = 0;

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
	savefile->WriteInt( m_iNumExisting );
	savefile->WriteInt( m_iNumVisible );

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
		savefile->WriteFloat( m_Classes[i].spacing );
		savefile->WriteVec3( m_Classes[i].origin );
		savefile->WriteInt( m_Classes[i].nocollide );
		savefile->WriteBool( m_Classes[i].floor );
		savefile->WriteBool( m_Classes[i].stack );
		savefile->WriteVec3( m_Classes[i].size );
	}
	savefile->WriteInt( m_Inhibitors.Num() );
	for( int i = 0; i < m_Inhibitors.Num(); i++ )
	{
		savefile->WriteVec3( m_Inhibitors[i].origin );
		savefile->WriteBox( m_Inhibitors[i].box );
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
	savefile->ReadInt( m_iNumExisting );
	savefile->ReadInt( m_iNumVisible );
	
    savefile->ReadInt( m_DistCheckTimeStamp );
	savefile->ReadInt( m_DistCheckInterval );
	savefile->ReadBool( m_bDistCheckXYOnly );

    savefile->ReadInt( num );
	assert( num == m_Entities.Num() );
	for( int i = 0; i < num; i++ )
	{
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
	for( int i = 0; i < num; i++ )
	{
		savefile->ReadString( m_Classes[i].classname );
		savefile->ReadString( m_Classes[i].skin );
		savefile->ReadInt( m_Classes[i].score );
		savefile->ReadFloat( m_Classes[i].cullDist );
		savefile->ReadFloat( m_Classes[i].spawnDist );
		savefile->ReadFloat( m_Classes[i].spacing );
		savefile->ReadVec3( m_Classes[i].origin );
		savefile->ReadInt( m_Classes[i].nocollide );
		savefile->ReadBool( m_Classes[i].floor );
		savefile->ReadBool( m_Classes[i].stack );
		savefile->ReadVec3( m_Classes[i].size );
	}
    savefile->ReadInt( num );
	assert( num == m_Inhibitors.Num() );
	for( int i = 0; i < num; i++ )
	{
		savefile->ReadVec3( m_Inhibitors[i].origin );
		savefile->ReadBox( m_Inhibitors[i].box );
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

	// the Lode itself is sneaky and hides itself
	Hide();

	// And is nonsolid, too!
	GetPhysics()->SetContents( 0 );

	active = true;

	m_DistCheckInterval = (int) (1000.0f * spawnArgs.GetFloat( "dist_check_period", "0" ));

	// do we have a cull range? (default is 150 units after the hide distance)
	m_fCullRange = spawnArgs.GetFloat( "cull_range", "150" );

	gameLocal.Printf (" LODE %s: cull range = %0.2f.\n", GetName(), m_fCullRange );

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
	lode_inhibitor_t		LodeInhibitor;
	int						numEntities;
	float					f_avgSize;		// avg floorspace of all classes

	if ( targets.Num() == 0 )
	{
		gameLocal.Warning( "LODE %s has no targets!", GetName() );
		BecomeInactive( TH_THINK );
		return;
	}

	// Gather all targets and make a note of them, also summing their "lod_score" up
	m_iScore = 0;
	f_avgSize = 0;
	m_Classes.Clear();
	for( int i = 0; i < targets.Num(); i++ )
	{
		idEntity *ent = targets[ i ].GetEntity();

	   	if ( ent )
		{
			// TODO: if this is a LODE inhibitor, add it to our "forbidden zones":
			if ( idStr( ent->GetEntityDefName() ) == "atdm:no_lode")
			{
				idBounds b = ent->GetRenderEntity()->bounds; 
				idVec3 s = b.GetSize();
				// gameLocal.Printf( "LODE %s: Inhibitor size %0.2f %0.2f %0.2f\n", GetName(), s.x, s.y, s.z );

				LodeInhibitor.origin = ent->spawnArgs.GetVector( "origin" );
				// the "axis" part does not work, as DR simply rotates the brush model, but does not record an axis
				// or rotation spawnarg. Use clipmodel instead?
				LodeInhibitor.box = idBox( LodeInhibitor.origin, ent->GetRenderEntity()->bounds.GetSize() / 2, ent->GetPhysics()->GetAxis() );
				m_Inhibitors.Append ( LodeInhibitor );
				continue;
			}

			int iEntScore = ent->spawnArgs.GetInt( "lode_score", "1" );
			if (iEntScore < 0)
			{
				gameLocal.Warning( "LODE %s: Target %s has invalid lode_score %i!\n", GetName(), ent->GetName(), iEntScore );
			}
			else
			{
				// add a class based on this entity
				m_iScore += iEntScore;
				LodeClass.score = iEntScore;
				LodeClass.classname = ent->GetEntityDefName();
				LodeClass.skin = ent->spawnArgs.GetString( "skin", "" );
				// Do not use GetPhysics()->GetOrigin(), as the LOD system might have shifted
				// the entity already between spawning and us querying the info:
				LodeClass.origin = ent->spawnArgs.GetVector( "origin" );
				LodeClass.floor = ent->spawnArgs.GetBool( "lode_floor", "0" );
				LodeClass.stack = ent->spawnArgs.GetBool( "lode_stack", "1" );
				LodeClass.spacing = ent->spawnArgs.GetBool( "lode_spacing", "0" );
				LodeClass.nocollide	= NOCOLLIDE_ATALL;
				// set rotation of entity to 0, so we get the unrotated bounds size
				ent->SetAxis( mat3_identity );
				LodeClass.size = ent->GetRenderEntity()->bounds.GetSize();
				// TODO: use a projection along the "floor-normal"
				// TODO: multiply the average class size with the class score (so little used entities don't "use" more space)
				f_avgSize += (LodeClass.size.x + LodeClass.spacing) * (LodeClass.size.y + LodeClass.spacing);
				// gameLocal.Printf( "LODE %s: Entity class size %0.2f %0.2f %0.2f\n", GetName(), LodeClass.size.x, LodeClass.size.y, LodeClass.size.z );
				LodeClass.cullDist = 0;
				LodeClass.spawnDist = 0;
				float hideDist = ent->spawnArgs.GetFloat( "hide_distance", "0" );
				if (m_fCullRange > 0 && hideDist > 0)
				{
					LodeClass.cullDist = hideDist + m_fCullRange;
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

	// increase the avg by X% to allow some spacing (even if spacing = 0)
	f_avgSize *= 1.3;

	// (32 * 32 + 64 * 64 ) / 2 => 50 * 50
	f_avgSize /= m_Classes.Num();

	m_iNumEntities = spawnArgs.GetInt( "entity_count", "0" );
	if (m_iNumEntities == 0)
	{
		// compute entity count dynamically from area that we cover
		idBounds bounds = renderEntity.bounds;
		idVec3 size = bounds.GetSize();

		// avoid values too small or even 0
		if (f_avgSize < 4)
		{
			f_avgSize = 4;
		}
		m_iNumEntities = (size.x * size.y) / f_avgSize;		// naive asumption each entity covers on avg X units
		// TODO: remove the limit once culling works
		if (m_iNumEntities > 4000)
		{
			m_iNumEntities = 4000;
		}
	}

	if (m_iNumEntities <= 0)
	{
		gameLocal.Warning( "LODE %s: entity count is invalid: %i!\n", GetName(), m_iNumEntities );
	}

	gameLocal.Printf( "LODE %s: Overall score: %i. Max. entity count: %i\n", GetName(), m_iScore, m_iNumEntities );

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
	idBounds				testBounds;			// to test whether the translated/rotated entity
   												// collides with another entity (fast check)
	idBox					testBox;			// to test whether the translated/rotated entity is inside the LODE
												// or collides with another entity (slow, but more precise)
	idBox					box;				// LODE box
	idList<idBounds>		LodeEntityBounds;	// precompute entity bounds for collision checks (fast)
	idList<idBox>			LodeEntityBoxes;	// precompute entity box for collision checks (slow, but thorough)

	// Re-init the seed. 0 means random sequence, otherwise use the specified value
    // so that we get exactly the same sequence every time:
	m_iSeed = spawnArgs.GetFloat( "seed", "0" ) || gameLocal.random.RandomInt();

	// Get our bounds (fast estimate) and the oriented box (slow, but thorough)
	idBounds bounds = renderEntity.bounds;
	idVec3 size = bounds.GetSize();
	// rotating the func-static in DR rotates the brush, but does not change the axis or
	// add a spawnarg, so this will not work properly:
	idMat3 axis = renderEntity.axis;

	idAngles angles = axis.ToAngles();		// debug

	box = idBox( renderEntity.origin, size, axis );

	float spacing = spawnArgs.GetFloat( "spacing", "0" );

	gameLocal.Printf( "LODE %s: Seed %i Size %0.2f %0.2f %0.2f Axis %s.\n", GetName(), m_iSeed, size.x, size.y, size.z, angles.ToString() );

	m_Entities.Clear();
	LodeEntityBounds.Clear();
	LodeEntityBoxes.Clear();

	m_iNumExisting = 0;
	m_iNumVisible = 0;
	
	// Compute random positions for all entities that we want to spawn for each class
	origin = GetPhysics()->GetOrigin();
	for (int i = 0; i < m_Classes.Num(); i++)
	{
		int iEntities = m_iNumEntities * (static_cast<float>(m_Classes[i].score)) / m_iScore;	// sum 2, this one 1 => 50% of this class
		gameLocal.Printf( "LODE %s: Creating %i entities of class %s.\n", GetName(), iEntities, m_Classes[i].classname.c_str() );
		for (int j = 0; j < iEntities; j++)
		{
			int tries = 0;
			while (tries++ < MAX_TRIES)
			{
				// TODO: allow the "floor" direction be set via spawnarg

				// restrict to our AAB (unrotated)
				LodeEntity.origin = idVec3( RandomFloat() * size.x, RandomFloat() * size.y, 0 );

				// Rotate around our rotation axis (to support rotated LODE brushes)
				// for random placement, this does not matter, but f.i. for grid placement
				// we need to rotate the initialposition to rotate the grid
				LodeEntity.origin *= axis;

				// add origin of the LODE
				LodeEntity.origin += origin;

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

				// LodeEntity.origin might now be outside of our oriented box, we check this later

				// randomly rotate
				LodeEntity.angles = idAngles( 0, RandomFloat() * 359.99, 0 );

				// inside LODE bounds?
				testBox = idBox ( LodeEntity.origin, m_Classes[i].size, LodeEntity.angles.ToMat3() );
				if (testBox.IntersectsBox( box ))
				{
					//gameLocal.Printf( "LODE %s: Entity would be inside our box. Checking against inhibitors.\n", GetName() );

					bool inhibited = false;
					for (int k = 0; k < m_Inhibitors.Num(); k++)
					{
						if (testBox.IntersectsBox( m_Inhibitors[k].box ) )
						{
							// inside an inhibitor, skip
							gameLocal.Printf( "LODE %s: Entity inhibited by inhibitor %i. Trying again.\n", GetName(), k );
							inhibited = true;
							break;							
						}
					}

					if ( inhibited )
					{
						continue;
					}

					// check the min. spacing constraint
				 	float use_spacing = spacing;
					if (m_Classes[i].spacing != 0)
					{
						use_spacing = m_Classes[i].spacing;
					}

					// gameLocal.Printf( "LODE %s: Using spacing constraint %0.2f for entity %i.\n", GetName(), use_spacing, j );

					// check that the entity does not collide with any other entity
					if (m_Classes[i].nocollide > 0 || use_spacing > 0)
					{
						bool collides = false;

						// expand the testBounds and testBox with the spacing
						testBounds = (idBounds( m_Classes[i].size ) + LodeEntity.origin) * LodeEntity.angles.ToMat3();
						testBounds.ExpandSelf( use_spacing );
						testBox.ExpandSelf( use_spacing );

						for (int k = 0; k < m_Entities.Num(); k++)
						{
							// do a quick check on bounds first
							idBounds otherBounds = LodeEntityBounds[k];
							if (otherBounds.IntersectsBounds (testBounds))
							{
								//gameLocal.Printf( "LODE %s: Entity %i bounds collides with entity %i bounds, checking box.\n", GetName(), j, k );
								// do a thorough check against the box here

								idBox otherBox = LodeEntityBoxes[k];
								if (otherBox.IntersectsBox (testBox))
								{
									gameLocal.Printf( "LODE %s: Entity %i box collides with entity %i box, trying another place.\n", GetName(), j, k );
									collides = true;
									break;
								}
								// no collision, place is usable
							}
						}
						if (collides)
						{
							continue;
						}
					}

					gameLocal.Printf( "LODE %s: Found valid position for entity %i with %i tries.\n", GetName(), j, tries );
					break;
				}
			}
			// couldn't place entity even after 10 tries?
			if (tries >= MAX_TRIES) continue;

			// TODO: choose skin randomly
			LodeEntity.skin = m_Classes[i].skin;
			// will be automatically spawned when we are in range
			LodeEntity.hidden = true;
			LodeEntity.exists = false;
			LodeEntity.entity = 0;
			LodeEntity.classIdx = i;
			// precompute bounds for a fast collision check
			LodeEntityBounds.Append( (idBounds (m_Classes[i].size ) + LodeEntity.origin) * LodeEntity.angles.ToMat3() );
			// precompute box for slow collision check
			LodeEntityBoxes.Append( idBox ( LodeEntity.origin, m_Classes[i].size, LodeEntity.angles.ToMat3() ) );
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
	int culled = 0;
	int spawned = 0;

	// tels: seems unneccessary.
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

	// Distance dependence checks
	if( (gameLocal.time - m_DistCheckTimeStamp) > m_DistCheckInterval )
	{
		m_DistCheckTimeStamp = gameLocal.time;

		// cache these values for speed
		idVec3 playerOrigin = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
		idVec3 vGravNorm = GetPhysics()->GetGravityNormal();
		float lodBias = cv_lod_bias.GetFloat();

		// square to avoid taking the square root from the distance
		lodBias *= lodBias;

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

				// TODO: Limit number of entities to spawn per frame

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
						//gameLocal.Printf( "LODE %s: Spawned entity #%i (%s) at  %0.2f, %0.2f, %0.2f.\n",
						//		GetName(), i, lclass->classname.c_str(), ent->origin.x, ent->origin.y, ent->origin.z );
						ent->exists = true;
						ent->entity = ent2->entityNumber;
						// and rotate
						ent2->SetAxis( ent->angles.ToMat3() );
						ent2->BecomeInactive( TH_THINK );
						//ent2->DisableLOD();
						m_iNumExisting ++;
						m_iNumVisible ++;
						spawned++;
					}
				}
			}	
			else
			{
				// cull entities that are outside "hide_distance + fade_out_distance + m_fCullRange
				if (ent->exists && deltaSq > lclass->cullDist)
				{
					// TODO: Limit number of entities to cull per frame

					// cull (remove) the entity
					idEntity *ent2 = gameLocal.entities[ ent->entity ];
					if (ent2)
					{
						// TODO: SafeRemve?
						ent2->PostEventMS( &EV_Remove, 0 );
					}

					//gameLocal.Printf( "LODE %s: Culling entity #%i.\n", GetName(), i );
					m_iNumExisting --;
					m_iNumVisible --;
					culled++;
					ent->exists = false;
				}
				// TODO: Normal LOD code here (replicate from entity)
			}
		}
		if (spawned > 0 || culled > 0)
		{
			gameLocal.Printf( "LODE %s: Spawned %i, culled %i, now existing: %i, now visible: %i\n",
				GetName(), spawned, culled, m_iNumExisting, m_iNumVisible );
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

