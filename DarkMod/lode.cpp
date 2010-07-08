// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod Team)

/*
Level Of Detail Entities - Manage other entities based on LOD (e.g. distance)

TODO: add console command to save all LODE entities as prefab?
TODO: take over LOD changes from entity
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "lode.h"

// maximum number of tries to place an entity
#define MAX_TRIES 16

// Avoid that we run out of entities:

// TODO: if the number of entities is higher then this,
// then we favour to spawn big(ger) entities over smaller ones
#define SPAWN_SMALL_LIMIT (MAX_GENTITIES - 500)

// if the number of entities is higher than this, then we no longer spawn entities
#define SPAWN_LIMIT (MAX_GENTITIES - 100)

const idEventDef EV_Deactivate( "deactivate", "e" );
const idEventDef EV_CullAll( "cullAll", "" );

/*
   Lode

   Entity that spawns/culls other entities, but is invisible on its own.

===============================================================================
*/

CLASS_DECLARATION( idStaticEntity, Lode )
	EVENT( EV_Activate,				Lode::Event_Activate )
	EVENT( EV_Deactivate,			Lode::Event_Deactivate )
	EVENT( EV_CullAll,				Lode::Event_CullAll )
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
	m_iOrgSeed = 3;
	m_iScore = 0;
	m_fLODBias = 0;

	m_bPrepared = false;
	m_Entities.Clear();
	m_Classes.Clear();
	m_Inhibitors.Clear();

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
	savefile->WriteInt( m_iOrgSeed );
	savefile->WriteInt( m_iScore );
	savefile->WriteInt( m_iNumEntities );
	savefile->WriteInt( m_iNumExisting );
	savefile->WriteInt( m_iNumVisible );
	savefile->WriteFloat( m_fAvgSize );
	savefile->WriteFloat( m_fLODBias );

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
		savefile->WriteFloat( m_Classes[i].bunching );
		savefile->WriteFloat( m_Classes[i].sink_min );
		savefile->WriteFloat( m_Classes[i].sink_max );
		savefile->WriteVec3( m_Classes[i].origin );
		savefile->WriteInt( m_Classes[i].nocollide );
		savefile->WriteInt( m_Classes[i].falloff );
		savefile->WriteBool( m_Classes[i].floor );
		savefile->WriteBool( m_Classes[i].stack );
		savefile->WriteBool( m_Classes[i].noinhibit );
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
	savefile->ReadInt( m_iOrgSeed );
	savefile->ReadInt( m_iScore );
	savefile->ReadInt( m_iNumEntities );
	savefile->ReadInt( m_iNumExisting );
	savefile->ReadInt( m_iNumVisible );
	savefile->ReadFloat( m_fAvgSize );
	savefile->ReadFloat( m_fLODBias );
	
    savefile->ReadInt( m_DistCheckTimeStamp );
	savefile->ReadInt( m_DistCheckInterval );
	savefile->ReadBool( m_bDistCheckXYOnly );

    savefile->ReadInt( num );
	m_Entities.Clear();
	m_Entities.SetNum( num );
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
	m_Classes.Clear();
	m_Classes.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		savefile->ReadString( m_Classes[i].classname );
		savefile->ReadString( m_Classes[i].skin );
		savefile->ReadInt( m_Classes[i].score );
		savefile->ReadFloat( m_Classes[i].cullDist );
		savefile->ReadFloat( m_Classes[i].spawnDist );
		savefile->ReadFloat( m_Classes[i].spacing );
		savefile->ReadFloat( m_Classes[i].bunching );
		savefile->ReadFloat( m_Classes[i].sink_min );
		savefile->ReadFloat( m_Classes[i].sink_max );
		savefile->ReadVec3( m_Classes[i].origin );
		savefile->ReadInt( m_Classes[i].nocollide );
		savefile->ReadInt( m_Classes[i].falloff );
		savefile->ReadBool( m_Classes[i].floor );
		savefile->ReadBool( m_Classes[i].stack );
		savefile->ReadBool( m_Classes[i].noinhibit );
		savefile->ReadVec3( m_Classes[i].size );
	}
    savefile->ReadInt( num );
	m_Inhibitors.Clear();
	m_Inhibitors.SetNum( num );
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
Lode::RandomFloatExp - Random float between 0 and 1 with exponential falloff (param lambda != 0)
===============
*/
ID_INLINE float Lode::RandomFloatExp( const float lambda ) {
	unsigned long i;
	float U;

	m_iSeed = 1664525L * m_iSeed + 1013904223L;
	i = Lode::IEEE_ONE | ( m_iSeed & Lode::IEEE_MASK );
	U = ( *(float *)&i ) - 1.0f;
	if (U <= 0)
	{
		gameLocal.Warning("RandomFloatExp: U is %0.2f\n", U);
		return 1.0;
	}
	U = - idMath::Log( U ) / lambda;
	// clamp to 0.0 .. 1.0
	if (U > 1.0) { U = 1.0; }
	return U;
}

/*
===============
Lode::RandomFloatSqr - Random float between 0 and 1 with squared falloff (param lambda != 0)
===============
*/
ID_INLINE float Lode::RandomFloatSqr( void ) {
	unsigned long i;
	float U;

	m_iSeed = 1664525L * m_iSeed + 1013904223L;
	i = Lode::IEEE_ONE | ( m_iSeed & Lode::IEEE_MASK );
	U = ( *(float *)&i ) - 1.0f;
	return U * U;
}

/*
===============
Lode::Spawn
===============
*/
void Lode::Spawn( void ) {

	idClipModel *clip = GetPhysics()->GetClipModel();
	idVec3 o = clip->GetOrigin();
	idVec3 s = clip->GetBounds().GetSize();
	idAngles a = clip->GetAxis().ToAngles();
	gameLocal.Printf( "LODE %s: Clipmodel origin %0.2f %0.2f %0.2f size %0.2f %0.2f %0.2f axis %s.\n", GetName(), o.x, o.y, o.z, s.x, s.y, s.z, a.ToString() );

//	idTraceModel *trace = GetPhysics()->GetClipModel()->GetTraceModel();
//	idVec3 o = trace->GetOrigin();
//	idVec3 s = trace->GetBounds().GetSize();
//	idAngles a = trace->GetAxis().ToAngles();

//	gameLocal.Printf( "LODE %s: Tracemodel origin %0.2f %0.2f %0.2f size %0.2f %0.2f %0.2f axis %s.\n", GetName(), o.x, o.y, o.z, s.x, s.y, s.z, a.ToString() );

	idBounds bounds = renderEntity.bounds;
	idVec3 size = bounds.GetSize();
	idAngles angles = renderEntity.axis.ToAngles();

	gameLocal.Printf( "LODE %s: Seed %i Size %0.2f %0.2f %0.2f Axis %s.\n", GetName(), m_iSeed, size.x, size.y, size.z, angles.ToString() );

/*
	// how to get the current model (rough outline)

   get number of surfaces:
	// NumBaseSurfaces will not count any overlays added to dynamic models
    virtual int                                     NumBaseSurfaces() const = 0;

   get all surfaces:
    // get a pointer to a surface
    virtual const modelSurface_t *Surface( int surfaceNum ) const = 0;

   for each surface, get all vertices

   then from these vertices, create a vertic-list and from this a trcemodel:

	idTraceModel trace = idTraceModel();
	trace.SetupPolygon( vertices, int );
*/

	// the Lode itself is sneaky and hides itself
	Hide();

	// And is nonsolid, too!
	GetPhysics()->SetContents( 0 );

	m_fLODBias = cv_lod_bias.GetFloat();

	active = true;

	m_DistCheckInterval = (int) (1000.0f * spawnArgs.GetFloat( "dist_check_period", "0.05" ));

	// do we have a cull range? (default is 150 units after the hide distance)
	m_fCullRange = spawnArgs.GetFloat( "cull_range", "150" );

	gameLocal.Printf (" LODE %s: cull range = %0.2f.\n", GetName(), m_fCullRange );

	m_bDistCheckXYOnly = spawnArgs.GetBool( "dist_check_xy", "0" );

	// Add some phase diversity to the checks so that they don't all run in one frame
	// make sure they all run on the first frame though, by initializing m_TimeStamp to
	// be at least one interval early.
	m_DistCheckTimeStamp = gameLocal.time - (int) (m_DistCheckInterval * (1.0f + gameLocal.random.RandomFloat()) );

	// Have to start thinking
	BecomeActive( TH_THINK );
}

/*
===============
Lode::addClassFromEntity - take an entity as template and add a class from it. Returns the size of this class
===============
*/
float Lode::addClassFromEntity( idEntity *ent, const int iEntScore )
{
	lode_class_t			LodeClass;
	idStr falloff;

	LodeClass.score = iEntScore;
	LodeClass.classname = ent->GetEntityDefName();
	LodeClass.skin = ent->spawnArgs.GetString( "skin", "" );
	// Do not use GetPhysics()->GetOrigin(), as the LOD system might have shifted
	// the entity already between spawning and us querying the info:
	LodeClass.origin = ent->spawnArgs.GetVector( "origin" );
	// these are ignored for pseudo classes (e.g. watch_breathren):
	LodeClass.floor = ent->spawnArgs.GetBool( "lode_floor", "0" );
	LodeClass.stack = ent->spawnArgs.GetBool( "lode_stack", "1" );
	LodeClass.noinhibit = ent->spawnArgs.GetBool( "lode_noinhibit", "0" );

	LodeClass.spacing = ent->spawnArgs.GetFloat( "lode_spacing", "0" );

	// to randomly sink entities into the floor
	LodeClass.sink_min = ent->spawnArgs.GetFloat( "lode_sink_min", spawnArgs.GetString( "sink_min", "0") );
	LodeClass.sink_max = ent->spawnArgs.GetFloat( "lode_sink_max", spawnArgs.GetString( "sink_max", "0") );
	if (LodeClass.sink_max < LodeClass.sink_min)
	{
		LodeClass.sink_max = LodeClass.sink_min;
	}

	LodeClass.falloff = -1;	// none
	falloff = ent->spawnArgs.GetString( "lode_falloff", spawnArgs.GetString( "falloff", "none") );
	if (falloff == "none")
	{
		LodeClass.falloff = 0;
	}
	if (falloff == "cutoff")
	{
		LodeClass.falloff = 1;
	}
	if (falloff == "square")
	{
		LodeClass.falloff = 2;
	}
	if (falloff == "exp")
	{
		LodeClass.falloff = 3;
	}
	if (LodeClass.falloff == -1)
	{
		gameLocal.Warning ("LODE %s: Invalid falloff %s, expect one of none, cutoff, square or exp.\n", GetName(), falloff.c_str() );
		LodeClass.falloff = 0;
	}

	LodeClass.bunching = ent->spawnArgs.GetFloat( "lode_bunching", spawnArgs.GetString( "bunching", "0") );
	if (LodeClass.bunching < 0 || LodeClass.bunching > 1.0)
	{
		gameLocal.Warning ("LODE %s: Invalid bunching value %0.2f, must be between 0 and 1.0.\n", GetName(), LodeClass.bunching );
		LodeClass.bunching = 0;
	}
	if (LodeClass.spacing > 0)
	{
		LodeClass.nocollide	= NOCOLLIDE_ATALL;
	}
	else
	{
		// don't collide with other existing statics, but collide with the autogenerated ones
		// TODO: parse from "lode_nocollide" "same, other, world, static"
		LodeClass.nocollide	= NOCOLLIDE_STATIC;
	}
	// set rotation of entity to 0, so we get the unrotated bounds size
	ent->SetAxis( mat3_identity );
	LodeClass.size = ent->GetRenderEntity()->bounds.GetSize();
	// gameLocal.Printf( "LODE %s: size of class %i: %0.2f %0.2f\n", GetName(), i, LodeClass.size.x, LodeClass.size.y );
	// TODO: use a projection along the "floor-normal"
	// TODO: multiply the average class size with the class score (so little used entities don't "use" more space)

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

	return (LodeClass.size.x + LodeClass.spacing) * (LodeClass.size.y + LodeClass.spacing);
}

/*
===============
Generate a scaling factor depending on the GUI setting
===============
*/
float Lode::LODBIAS ( void )
{
	// scale density with GUI setting
	// The GUI specifies: 0.5;0.75;1.0;1.5;2.0;3.0, but 0.5 and 3.0 are quite extreme,
	// so we scale the values first:
	float lod_bias = cv_lod_bias.GetFloat();
	if (lod_bias < 0.8)
	{
		lod_bias *= 1.2f;										// 0.5, 0.75 => 0.57, 0.87
	}
	else if (lod_bias > 1.0f)
	{
																// 1.5, 2, 3 => 1.16, 1.33, 1.5
		lod_bias = ( lod_bias > 2.0f ? 0.5f : 1.0f) + ((lod_bias - 1.0f) / 3.0f);
	}

	// 0.57, 0.87, 1.0, 1.16, 1.33, 1.5
	return lod_bias;
}

/*
===============
Create the places for all entities that we control so we can later spawn them.
===============
*/
void Lode::ComputeEntityCount( void )
{
	m_iNumEntities = spawnArgs.GetInt( "max_entities", "0" );
	if (m_iNumEntities == 0)
	{
		// compute entity count dynamically from area that we cover
		float fDensity = spawnArgs.GetFloat( "density", "1.0" );

		// Scaled by GUI setting?
		if (spawnArgs.GetBool( "lod_scale_density", "1"))
		{
			fDensity *= LODBIAS();
		}

		idBounds bounds = renderEntity.bounds;
		idVec3 size = bounds.GetSize();

		m_iNumEntities = fDensity * (size.x * size.y) / m_fAvgSize;		// naive asumption each entity covers on avg X units
		gameLocal.Printf( "LODE %s: Dynamic entity count: %0.2f * %0.2f / %0.2f = %i.\n", GetName(), size.x, size.y, m_fAvgSize, m_iNumEntities );

		// We do no longer impose a limit, as no more than SPAWN_LIMIT will be existing:
		/* if (m_iNumEntities > SPAWN_LIMIT)
		{
			m_iNumEntities = SPAWN_LIMIT;
		}
		*/
	}
    else
	{
		// scale the amount of entities that the mapper requested, but only if it's > 10 or what was requested:
		if (m_iNumEntities > spawnArgs.GetFloat( "lod_scaling_limit", "10" ))
		{
			m_iNumEntities *= LODBIAS();
		}
	}
}

/*
===============
Create the places for all entities that we control so we can later spawn them.
===============
*/
void Lode::Prepare( void )
{	
	if ( targets.Num() == 0 )
	{
		gameLocal.Warning( "LODE %s has no targets!", GetName() );
		BecomeInactive( TH_THINK );
		return;
	}

	lode_inhibitor_t LodeInhibitor;

	// Gather all targets and make a note of them, also summing their "lod_score" up
	m_iScore = 0;
	m_fAvgSize = 0;
	m_Classes.Clear();
	m_Inhibitors.Clear();

	for( int i = 0; i < targets.Num(); i++ )
	{
		idEntity *ent = targets[ i ].GetEntity();

	   	if ( ent )
		{
			// if this is a LODE inhibitor, add it to our "forbidden zones":
			if ( idStr( ent->GetEntityDefName() ) == "atdm:no_lode")
			{
				idBounds b = ent->GetRenderEntity()->bounds; 
				idVec3 s = b.GetSize();
				gameLocal.Printf( "LODE %s: Inhibitor size %0.2f %0.2f %0.2f\n", GetName(), s.x, s.y, s.z );

				LodeInhibitor.origin = ent->spawnArgs.GetVector( "origin" );
				// the "axis" part does not work, as DR simply rotates the brush model, but does not record an axis
				// or rotation spawnarg. Use clipmodel instead?
				LodeInhibitor.box = idBox( LodeInhibitor.origin, ent->GetRenderEntity()->bounds.GetSize() / 2, ent->GetPhysics()->GetAxis() );
				m_Inhibitors.Append ( LodeInhibitor );
				continue;
			}

			// If this entity wants us to watch over his brethren, add them to our list:
			if ( ent->spawnArgs.GetBool( "lode_watch_brethren", "0" ) )
			{
				gameLocal.Printf( "LODE %s: %s (%s) wants us to take care of his brethren.\n",
						GetName(), ent->GetName(), ent->GetEntityDefName() );

				// add a pseudo class and ignore the size returned
				addClassFromEntity( ent, 0 );

				// no more to do for this target
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
				m_fAvgSize += addClassFromEntity( ent, iEntScore );
			}
		}
	}

	// increase the avg by X% to allow some spacing (even if spacing = 0)
	m_fAvgSize *= 1.3f;

	// (32 * 32 + 64 * 64 ) / 2 => 50 * 50
	m_fAvgSize /= m_Classes.Num();

	// avoid values too small or even 0
	if (m_fAvgSize < 4)
	{
		m_fAvgSize = 4;
	}

	// set m_iNumEntities from spawnarg, or density, taking GUI setting into account
	ComputeEntityCount();

	if (m_iNumEntities <= 0)
	{
		gameLocal.Warning( "LODE %s: entity count is invalid: %i!\n", GetName(), m_iNumEntities );
	}

	gameLocal.Printf( "LODE %s: Overall score: %i. Max. entity count: %i\n", GetName(), m_iScore, m_iNumEntities );

	// Init the seed. 0 means random sequence, otherwise use the specified value
    // so that we get exactly the same sequence every time:
	m_iSeed = spawnArgs.GetInt( "seed", "0" );
    if (m_iSeed == 0)
	{
		// The randseed upon loading a map seems to be always 0, so 
		// gameLocal.random.RandomInt() always returns 1 hence it is unusable:
		time_t seconds = time (NULL);
	    m_iSeed = (int) (1664525L * (int) seconds + 1013904223L) & 0x7FFFFFFFL;
	}
	// to restart the same sequence, f.i. when the user changes level of detail in GUI
	m_iOrgSeed = m_iSeed;

	PrepareEntities();

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

	// remove ourself after spawn?
	if (spawnArgs.GetBool("remove","0"))
	{
		// spawn all entities
		gameLocal.Printf( "LODE %s: Spawning all %i entities and then removing myself.\n", GetName(), m_iNumEntities );

		// for each of our "entities", do the distance check
		for (int i = 0; i < m_Entities.Num(); i++)
		{
			spawnEntity(i, false);		// spawn as unmanaged
		}

		// clear out memory just to be sure
		m_Classes.Clear();
		m_Entities.Clear();
		m_iNumEntities = 0;

		active = false;
		BecomeInactive( TH_THINK );

		// post event to remove ourselfes
		PostEventMS( &EV_Remove, 0 );
	}
	else
	{
		m_bPrepared = true;
		if (m_Entities.Num() == 0)
		{
			// could not create any entities?
			gameLocal.Printf( "LODE %s: Have no entities to control, becoming inactive.\n", GetName() );
			// Tels: Does somehow not work, bouncing us again and again into this branch?
			BecomeInactive(TH_THINK);
		}
	}
}

void Lode::PrepareEntities( void )
{
	idVec3					origin;				// The center of the LODE
	lode_entity_t			LodeEntity;			// temp. storage
	idBounds				testBounds;			// to test whether the translated/rotated entity
   												// collides with another entity (fast check)
	idBox					testBox;			// to test whether the translated/rotated entity is inside the LODE
												// or collides with another entity (slow, but more precise)
	idBox					box;				// The oriented box of the LODE
	idList<idBounds>		LodeEntityBounds;	// precompute entity bounds for collision checks (fast)
	idList<idBox>			LodeEntityBoxes;	// precompute entity box for collision checks (slow, but thorough)

	idList< int >			ClassIndex;			// random shuffling of classes
	int						s;
	float					f;

	// Get our bounds (fast estimate) and the oriented box (slow, but thorough)
	idBounds bounds = renderEntity.bounds;
	idVec3 size = bounds.GetSize();
	// rotating the func-static in DR rotates the brush, but does not change the axis or
	// add a spawnarg, so this will not work properly:
	idMat3 axis = renderEntity.axis;

	idAngles angles = axis.ToAngles();		// debug
	origin = renderEntity.origin;

	box = idBox( origin, size, axis );

	float spacing = spawnArgs.GetFloat( "spacing", "0" );

	gameLocal.Printf( "LODE %s: Seed %i Size %0.2f %0.2f %0.2f Axis %s.\n", GetName(), m_iSeed, size.x, size.y, size.z, angles.ToString() );

	m_Entities.Clear();
	LodeEntityBounds.Clear();
	LodeEntityBoxes.Clear();

	m_iNumExisting = 0;
	m_iNumVisible = 0;

	// Compute a random order of classes, so that when the mapper specified m_iNumEntities well below
	// the limit (e.g. 1), he gets a random entity from a random class (and not just one from the first
	// class always):
	ClassIndex.Clear();			// random shuffling of classes
	for (int i = 0; i < m_Classes.Num(); i++)
	{
		ClassIndex.Append ( i );		// 1,2,3,...
	}

	// shuffle at least 3 times
	s = m_Classes.Num(); f = s; s *= 3;
	for (int i = 0; i < s; i++)
	{
		int first = (int)(RandomFloat() * f);
		int second = (int)(RandomFloat() * f);
		int temp = ClassIndex[first]; ClassIndex[first] = ClassIndex[second]; ClassIndex[second] = temp;
	}

	// default random rotate
	idStr rand_rotate_min = spawnArgs.GetString("rotate_min", "0 0 0");
	idStr rand_rotate_max = spawnArgs.GetString("rotate_max", "5 360 5");

	// Compute random positions for all entities that we want to spawn for each class
	for (int idx = 0; idx < m_Classes.Num(); idx++)
	{
		if (m_Entities.Num() >= m_iNumEntities)
		{
			// have enough entities, stop
			break;
		}

		// progress with random shuffled class
		int i = ClassIndex[idx];

		// ignore pseudo classes used for watching brethren only:
		if (m_Classes[i].score == 0)
		{
			continue;
		}

		int iEntities = m_iNumEntities * (static_cast<float>(m_Classes[i].score)) / m_iScore;	// sum 2, this one 1 => 50% of this class
		// try at least one from each class (so "select 1 from 4 classes" works correctly)
		if (iEntities == 0)
		{
			iEntities = 1;
		}
		gameLocal.Printf( "LODE %s: Creating %i entities of class %s (#%i index %i).\n", GetName(), iEntities, m_Classes[i].classname.c_str(), i, idx );

		// default to what the LODE says
		idAngles class_rotate_min = spawnArgs.GetAngles("lode_rotate_min", rand_rotate_min);
		idAngles class_rotate_max = spawnArgs.GetAngles("lode_rotate_max", rand_rotate_max);

		for (int j = 0; j < iEntities; j++)
		{
			int tries = 0;
			while (tries++ < MAX_TRIES)
			{
				// TODO: allow the "floor" direction be set via spawnarg

				// use bunching? (will always fail if bunching = 0.0)
				// can only use bunching if we have at least one other entity already placed
				if ( m_Entities.Num() > 0 && RandomFloat() < m_Classes[i].bunching )
				{
					// find a random already existing entity of the same class
					// TODO: allow bunching with other classes, too:

					idList <int> BunchEntities;

					// radius
					float distance = m_Classes[i].size.x * m_Classes[i].size.x + 
									 m_Classes[i].size.y * m_Classes[i].size.y; 

					distance = idMath::Sqrt(distance);

					// need minimum the spacing and use maximum 2 times the spacing
					// TODO: make max spacing a spawnarg
					distance += m_Classes[i].spacing * 2; 

					BunchEntities.Clear();
					// build list of all entities we can bunch up to
					for (int e = 0; e < m_Entities.Num(); e++)
					{
						if (m_Entities[e].classIdx == i)
						{
							// same class, try to snuggle up
							BunchEntities.Append(e);
						}
					}
					// select one at random
					int bunchTarget = (float)BunchEntities.Num() * RandomFloat();

					// minimum origin distance (or entity will stick inside the other) is 2 * distance
					// maximum bunch radius is 3 times (2 + 1) the radius
					// TODO: make bunch_size and bunch_min_distance a spawnarg
					LodeEntity.origin = idPolar3( 2 * distance + RandomFloat() * distance / 3, RandomFloat() * 360.0f, 0 ).ToVec3();
					gameLocal.Printf ("LODE %s: Random origin from distance (%0.2f) %0.2f %0.2f %0.2f (%i)\n",
							GetName(), distance, LodeEntity.origin.x, LodeEntity.origin.y, LodeEntity.origin.z, bunchTarget );
					// subtract the LODE origin, as m_Entities[ bunchTarget ].origin already contains it and we would
					// otherwise add it twice below:
					LodeEntity.origin += m_Entities[ bunchTarget ].origin - origin;
					gameLocal.Printf ("LODE %s: Random origin plus bunchTarget origin %0.2f %0.2f %0.2f\n",
							GetName(), LodeEntity.origin.x, LodeEntity.origin.y, LodeEntity.origin.z );

				}
				else
				// no bunching, just random placement
				{
        			if (m_Classes[i].falloff == 0)
					{
						// restrict to our AAB (unrotated)
						LodeEntity.origin = idVec3( (RandomFloat() - 0.5f) * size.x, (RandomFloat() - 0.5f) * size.y, 0 );
					}
					else
					{
						switch (m_Classes[i].falloff)
						{
						case 1:
							// cutoff - polar coordinates in the range (0..1.0, 0.360)
							// TODO: The area for the entities is smaller than for "none" (square vs. circle
							// 		 but same entity count), account for this?
							// TODO: The random polar coordinates make it occupy less space in the center
							// 		 than in the outer areas, but distrubute the entity distance equally.
							//		 This leads to the center getting ever so slightly more entities then
							//		 the outer areas, compensate for this in the random generator formular?
							LodeEntity.origin = idPolar3( RandomFloat(), RandomFloat() * 360.0f, 0 ).ToVec3();
							break;
						case 2:
							// square
							LodeEntity.origin = idPolar3( RandomFloatSqr(), RandomFloat() * 360.0f, 0 ).ToVec3();
							break;
						default:
							// exp
							LodeEntity.origin = idPolar3( RandomFloatExp( 2.0f ), RandomFloat() * 360.0f, 0 ).ToVec3();
							break;
						}
						// scale the result to our box size
						LodeEntity.origin.x *= size.x / 2;
						LodeEntity.origin.y *= size.y / 2;
					}
				}

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

				// compute a random sink value
				if (m_Classes[i].sink_max > 0)
				{
					// TODO: use a gravity normal
					float sink = m_Classes[i].sink_min + RandomFloat() * ( m_Classes[i].sink_max - m_Classes[i].sink_min );
					// modify the z-axis according to the sink-value
					LodeEntity.origin.z -= sink;
				}

				// LodeEntity.origin might now be outside of our oriented box, we check this later

				// randomly rotate
				// pitch, yaw, roll
				LodeEntity.angles = idAngles( 
						class_rotate_min.pitch + RandomFloat() * (class_rotate_max.pitch - class_rotate_min.pitch),
						class_rotate_min.yaw   + RandomFloat() * (class_rotate_max.yaw   - class_rotate_min.yaw  ),
						class_rotate_min.roll  + RandomFloat() * (class_rotate_max.roll  - class_rotate_min.roll ) );
				/*
				gameLocal.Printf ("LODE %s: rand rotate for (%0.2f %0.2f %0.2f) %0.2f %0.2f %0.2f => %s\n", GetName(),
						class_rotate_min.pitch,
						class_rotate_min.yaw,
						class_rotate_min.roll,
						class_rotate_min.pitch + RandomFloat() * (class_rotate_max.pitch - class_rotate_min.pitch),
						class_rotate_min.yaw   + RandomFloat() * (class_rotate_max.yaw   - class_rotate_min.yaw  ),
						class_rotate_min.roll  + RandomFloat() * (class_rotate_max.roll  - class_rotate_min.roll ), LodeEntity.angles.ToString() );
				*/

				// inside LODE bounds?
				// IntersectsBox() also includes touching, but we want the entity to be completely inside
				// so we just check that the origin is inside, which is also faster:
				// The entity can stick still outside, we need to "shrink" the testbox by half the class size
				if (box.ContainsPoint( LodeEntity.origin ))
				{
					//gameLocal.Printf( "LODE %s: Entity would be inside our box. Checking against inhibitors.\n", GetName() );

					testBox = idBox ( LodeEntity.origin, m_Classes[i].size, LodeEntity.angles.ToMat3() );

					// only if this class can be inhibited
					if (! m_Classes[i].noinhibit)
					{
						bool inhibited = false;
						for (int k = 0; k < m_Inhibitors.Num(); k++)
						{
							// TODO: do a faster bounds check first?
							// this test ensures that entities "peeking" into the inhibitor will be inhibited, too
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

					if (tries < MAX_TRIES)
					{
						gameLocal.Printf( "LODE %s: Found valid position for entity %i with %i tries.\n", GetName(), j, tries );
					}
					break;
				}
				else
				{
					gameLocal.Printf( "LODE %s: Test position outside our box, trying again.\n", GetName() );
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

			if (m_Entities.Num() >= m_iNumEntities)
			{
				// have enough entities, stop
				break;
			}
		}
	}

	// if we have requests for watch brethren, do add them now
	for (int i = 0; i < m_Classes.Num(); i++)
	{
		// only care for classes where score == 0 (meaning: watch)
		if (m_Classes[i].score != 0)
		{
			continue;
		}
		// go through all entities
		for (int j = 0; j < gameLocal.num_entities; j++)
		{
			idEntity *ent = gameLocal.entities[ j ];

			if (!ent)
			{
				continue;
			}
			idVec3 origin = ent->GetPhysics()->GetOrigin();

			// the class we should watch?
			if (( ent->GetEntityDefName() == m_Classes[i].classname) &&
				// and is this entity in our box?
				(box.ContainsPoint( origin )) )
			{
				gameLocal.Printf( "LODE %s: Watching over brethren %s at %02f %0.2f %0.2f.\n", GetName(), ent->GetName(), origin.x, origin.y, origin.z );
				// add this entity to our list
				LodeEntity.origin = origin;
				LodeEntity.angles = ent->GetPhysics()->GetAxis().ToAngles();
				LodeEntity.skin = ent->spawnArgs.GetString("skin","");
				// already exists
				LodeEntity.hidden = false;
				LodeEntity.exists = true;
				LodeEntity.entity = j;
				LodeEntity.classIdx = i;
				m_Entities.Append( LodeEntity );
			}
		}
	}
}

/*
================
Lode::spawnEntity - spawn the entity with the given index, returns true if it was spawned
================
*/

bool Lode::spawnEntity( const int idx, const bool managed )
{
	struct lode_entity_t* ent = &m_Entities[idx];
	struct lode_class_t*  lclass = &(m_Classes[ ent->classIdx ]);

	// spawn the entity and note its number
	gameLocal.Printf( "LODE %s: Spawning entity #%i (%s).\n", GetName(), idx, lclass->classname.c_str() );

	// avoid that we run out of entities during run time
	if (gameLocal.num_entities > SPAWN_LIMIT)
	{
		return false;
	}

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

		// TODO: set skin

		// TODO: spawn as hidden, then later unhide them via LOD code
		//args.Set("hide", "1");
		// disable LOD checks on entities (we take care of this)
		if (managed)
		{
			//args.Set("dist_check_period", "0");
		}

		gameLocal.SpawnEntityDef( args, &ent2 );
		if (ent2)
		{
			// TODO: check if the entity has been spawned for the first time and if so,
			// 		 also take control of any attachments it has? Or spawn it during build
			//		 and then parse the attachments as new class?

			//gameLocal.Printf( "LODE %s: Spawned entity #%i (%s) at  %0.2f, %0.2f, %0.2f.\n",
			//		GetName(), i, lclass->classname.c_str(), ent->origin.x, ent->origin.y, ent->origin.z );
			ent->exists = true;
			ent->hidden = false;
			ent->entity = ent2->entityNumber;
			// and rotate
			// TODO: Would it be faster to set this as spawnarg before spawn?
			ent2->SetAxis( ent->angles.ToMat3() );
			if (managed)
			{
				ent2->BecomeInactive( TH_THINK );
			}
			// TODO: activate physics for moveables

			// Tell the entity to disable LOD on all it's attachments, and handle
			// them ourselves.
			idStaticEntity *s_ent = static_cast<idStaticEntity*>( ent2 );
			/* TODO: Disable LOD for attachments, too, then manage them outselves.
			   Currently, if you spawn 100 entities with 1 attachement each, we
			   save thinking on 100 entities, but still have 100 attachements think
			   each other.
			if (s_ent)
			{
				s_ent->StopLOD( true );
			}
			*/
			m_iNumExisting ++;
			m_iNumVisible ++;
			return true;
		}
	}
	return false;
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
	}
	// GUI setting changed?
	if ( idMath::Fabs(cv_lod_bias.GetFloat() - m_fLODBias) > 0.1)
	{
		gameLocal.Printf ("LODE %s: GUI setting changed, recomputing.\n", GetName() );

		int cur_entities = m_iNumEntities;

		ComputeEntityCount();

		if (cur_entities != m_iNumEntities)
		{
			// TODO: We could keep the first N entities and only add the new ones if the quality
			// raises, and cull the last M entities if it sinks for more speed:
			Event_CullAll();

			// create same sequence again
			m_iSeed = m_iOrgSeed;

			gameLocal.Printf ("LODE %s: Have now %i entities.\n", GetName(), m_iNumEntities );
			PrepareEntities();
			// save the new value
		}
		m_fLODBias = cv_lod_bias.GetFloat();
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
			//		 so they can manage their attachment's LOD, too.

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
				if (spawnEntity( i, true ))
				{
					spawned ++;
				}
			}	
			else
			{
				// cull entities that are outside "hide_distance + fade_out_distance + m_fCullRange
				if (ent->exists && lclass->cullDist > 0 && deltaSq > lclass->cullDist)
				{
					// TODO: Limit number of entities to cull per frame

					// cull (remove) the entity
					idEntity *ent2 = gameLocal.entities[ ent->entity ];
					if (ent2)
					{
						// Before we remove the entity, save it's position and angles
						// That makes it work for moveables or anything else that
						// might have changed position (teleported away etc)
						ent->origin = ent2->GetPhysics()->GetOrigin();
						ent->angles = ent2->GetPhysics()->GetAxis().ToAngles();

						// TODO: SafeRemve?
						ent2->PostEventMS( &EV_Remove, 0 );
					}

					// gameLocal.Printf( "LODE %s: Culling entity #%i (%0.2f > %0.2f).\n", GetName(), i, deltaSq, lclass->cullDist );

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
			// the overall number seems to be the maximum number of entities that ever existed, so
			// to get the true real amount, one would probably go through gameLocal.entities[] and
			// count the valid ones:
			gameLocal.Printf( "%s: spawned %i, culled %i, existing: %i, visible: %i, overall: %i\n",
				GetName(), spawned, culled, m_iNumExisting, m_iNumVisible, gameLocal.num_entities );
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

