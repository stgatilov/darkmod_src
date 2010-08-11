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
TODO: turn "exists" and "hidden" into flags field, add there a "pseudoclass" bit so
	  we can use much smaller structs for pseudo classes (we might have thousands
	  of pseudoclass structs due to each having a different hmodel)
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "../game/game_local.h"
#include "../idlib/containers/list.h"
#include "lode.h"

// maximum number of tries to place an entity
#define MAX_TRIES 8

// the name of the class where to look for shared models
#define FUNC_STATIC "func_static"
// the name of the dummy func static with a visual model
// Used because I could not get it to work with spawning an
// empty func_static, then adding the model (modelDefHandle is protected)
#define FUNC_DUMMY "atdm:lode_dummy_static"

// Avoid that we run out of entities:

// TODO: if the number of entities is higher then this,
// then we favour to spawn big(ger) entities over smaller ones
#define SPAWN_SMALL_LIMIT (MAX_GENTITIES - 500)

// if the number of entities is higher than this, we no longer spawn entities
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

	m_iSeed = 3;
	m_iSeed_2 = 7;
	m_iOrgSeed = 7;
	m_iScore = 0;
	m_fLODBias = 0;

	m_iDebug = 0;
	m_bDebugColors = false;

	m_bWaitForTrigger = false;

	m_bPrepared = false;
	m_Entities.Clear();
	m_Classes.Clear();
	m_Inhibitors.Clear();

	// always put the empty skin into the list so it has index 0
	m_Skins.Clear();
	m_Skins.Append ( idStr("") );

	m_iNumEntities = 0;
	m_iNumExisting = 0;
	m_iNumVisible = 0;

	m_iNumPVSAreas = 0;
	m_iThinkCounter = 0;

	m_DistCheckTimeStamp = 0;
	m_DistCheckInterval = 0.5f;
	m_bDistCheckXYOnly = false;
}

/*
===============
Lode::~Lode
===============
*/
Lode::~Lode(void) {

	//gameLocal.Warning ("LODE %s: Shutdown.\n", GetName() );
	ClearClasses();
}
/*
===============
Lode::Save
===============
*/
void Lode::Save( idSaveGame *savefile ) const {

	savefile->WriteBool( active );
	savefile->WriteBool( m_bWaitForTrigger );

	savefile->WriteInt( m_iDebug );
	savefile->WriteBool( m_bDebugColors );

	savefile->WriteInt( m_iSeed );
	savefile->WriteInt( m_iSeed_2 );
	savefile->WriteInt( m_iOrgSeed );
	savefile->WriteInt( m_iScore );
	savefile->WriteInt( m_iNumEntities );
	savefile->WriteInt( m_iNumExisting );
	savefile->WriteInt( m_iNumVisible );
	savefile->WriteInt( m_iThinkCounter );
	savefile->WriteFloat( m_fAvgSize );
	savefile->WriteFloat( m_fLODBias );

    savefile->WriteInt( m_DistCheckTimeStamp );
	savefile->WriteInt( m_DistCheckInterval );
	savefile->WriteBool( m_bDistCheckXYOnly );

	savefile->WriteInt( m_Entities.Num() );
	for( int i = 0; i < m_Entities.Num(); i++ )
	{
		savefile->WriteInt( m_Entities[i].skinIdx );
		savefile->WriteVec3( m_Entities[i].origin );
		savefile->WriteAngles( m_Entities[i].angles );
		savefile->WriteVec3( m_Entities[i].color );
		savefile->WriteBool( m_Entities[i].hidden );
		savefile->WriteBool( m_Entities[i].exists );
		savefile->WriteInt( m_Entities[i].entity );
		savefile->WriteInt( m_Entities[i].classIdx );
	}

	savefile->WriteInt( m_Classes.Num() );
	for( int i = 0; i < m_Classes.Num(); i++ )
	{
		savefile->WriteString( m_Classes[i].classname );
		savefile->WriteString( m_Classes[i].modelname );
		savefile->WriteInt( m_Classes[i].score );
		savefile->WriteFloat( m_Classes[i].cullDist );
		savefile->WriteFloat( m_Classes[i].spawnDist );
		savefile->WriteFloat( m_Classes[i].spacing );
		savefile->WriteFloat( m_Classes[i].bunching );
		savefile->WriteFloat( m_Classes[i].sink_min );
		savefile->WriteFloat( m_Classes[i].sink_max );
		savefile->WriteVec3( m_Classes[i].origin );
		savefile->WriteInt( m_Classes[i].nocollide );
		savefile->WriteBool( m_Classes[i].nocombine );
		savefile->WriteInt( m_Classes[i].falloff );
		savefile->WriteBool( m_Classes[i].floor );
		savefile->WriteBool( m_Classes[i].stack );
		savefile->WriteBool( m_Classes[i].noinhibit );
		savefile->WriteVec3( m_Classes[i].size );
		savefile->WriteVec3( m_Classes[i].color_min );
		savefile->WriteVec3( m_Classes[i].color_max );

		savefile->WriteFloat( m_Classes[i].defaultProb );

		savefile->WriteInt( m_Classes[i].skins.Num() );
		for( int j = 0; j < m_Classes[i].skins.Num(); j++ )
		{
			savefile->WriteInt( m_Classes[i].skins[j] );
		}

		savefile->WriteInt( m_Classes[i].materials.Num() );
		for( int j = 0; j < m_Classes[i].materials.Num(); j++ )
		{
			savefile->WriteString( m_Classes[i].materials[j].name );
			savefile->WriteFloat( m_Classes[i].materials[j].probability );
		}

		// only save these if they are used
		if ( m_Classes[i].falloff == 4)
		{
			savefile->WriteFloat( m_Classes[i].func_x );
			savefile->WriteFloat( m_Classes[i].func_y );
			savefile->WriteFloat( m_Classes[i].func_s );
			savefile->WriteFloat( m_Classes[i].func_a );
			savefile->WriteInt( m_Classes[i].func_Xt );
			savefile->WriteInt( m_Classes[i].func_Yt );
			savefile->WriteInt( m_Classes[i].func_f );
			savefile->WriteFloat( m_Classes[i].func_min );
			savefile->WriteFloat( m_Classes[i].func_max );
		}
		// image based distribution
		savefile->WriteString( m_Classes[i].map );

		// only write the rendermodel if it is used
		if ( NULL != m_Classes[i].hModel)
		{
			savefile->WriteBool( true );
			savefile->WriteModel( m_Classes[i].hModel );
		}
		else
		{
			savefile->WriteBool( false );
		}
		// only write the clipmodel if it is used
		if ( NULL != m_Classes[i].physicsObj)
		{
			savefile->WriteBool( true );
			m_Classes[i].physicsObj->Save( savefile );
		}
		else
		{
			savefile->WriteBool( false );
		}
	}
	savefile->WriteInt( m_Inhibitors.Num() );
	for( int i = 0; i < m_Inhibitors.Num(); i++ )
	{
		savefile->WriteVec3( m_Inhibitors[i].origin );
		savefile->WriteBox( m_Inhibitors[i].box );
	}
	savefile->WriteInt( m_Skins.Num() );
	for( int i = 0; i < m_Skins.Num(); i++ )
	{
		savefile->WriteString( m_Skins[i] );
	}

	savefile->WriteInt( m_iNumPVSAreas );
	for( int i = 0; i < m_iNumPVSAreas; i++ )
	{
		savefile->WriteInt( m_iPVSAreas[i] );
	}
}

/*
===============
Lode::ClearClasses

Free memory from render models and CImages
===============
*/
void Lode::ClearClasses( void )
{
	int n = m_Classes.Num();
	for(int i = 0; i < n; i++ )
	{
		if (NULL != m_Classes[i].hModel)
		{
			if (m_Classes[i].pseudo)
			{
				renderModelManager->FreeModel( m_Classes[i].hModel );
			}
			m_Classes[i].hModel = NULL;
		}
		if (NULL != m_Classes[i].physicsObj)
		{
			delete m_Classes[i].physicsObj;
			m_Classes[i].physicsObj = NULL;
		}
		if (NULL != m_Classes[i].megamodel)
		{
			delete m_Classes[i].megamodel;
			m_Classes[i].megamodel = NULL;
		}
		if (NULL != m_Classes[i].img)
		{
			m_Classes[i].img->Unload(true);
			delete m_Classes[i].img;
			m_Classes[i].img = NULL;
		}
	}
	m_Classes.Clear();
}

/*
===============
Lode::Restore
===============
*/
void Lode::Restore( idRestoreGame *savefile ) {
	int num;
	bool bHaveModel;

	savefile->ReadBool( active );
	savefile->ReadBool( m_bWaitForTrigger );

	savefile->ReadInt( m_iDebug );
	savefile->ReadBool( m_bDebugColors );

	savefile->ReadInt( m_iSeed );
	savefile->ReadInt( m_iSeed_2 );
	savefile->ReadInt( m_iOrgSeed );
	savefile->ReadInt( m_iScore );
	savefile->ReadInt( m_iNumEntities );
	savefile->ReadInt( m_iNumExisting );
	savefile->ReadInt( m_iNumVisible );
	savefile->ReadInt( m_iThinkCounter );
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
		savefile->ReadInt( m_Entities[i].skinIdx );
		savefile->ReadVec3( m_Entities[i].origin );
		savefile->ReadAngles( m_Entities[i].angles );
		savefile->ReadVec3( m_Entities[i].color );
		savefile->ReadBool( m_Entities[i].hidden );
		savefile->ReadBool( m_Entities[i].exists );
		savefile->ReadInt( m_Entities[i].entity );
		savefile->ReadInt( m_Entities[i].classIdx );
	}

    savefile->ReadInt( num );
	// clear m_Classes and free any models in it, too
	ClearClasses();
	m_Classes.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		savefile->ReadString( m_Classes[i].classname );
		savefile->ReadString( m_Classes[i].modelname );
		savefile->ReadInt( m_Classes[i].score );
		savefile->ReadFloat( m_Classes[i].cullDist );
		savefile->ReadFloat( m_Classes[i].spawnDist );
		savefile->ReadFloat( m_Classes[i].spacing );
		savefile->ReadFloat( m_Classes[i].bunching );
		savefile->ReadFloat( m_Classes[i].sink_min );
		savefile->ReadFloat( m_Classes[i].sink_max );
		savefile->ReadVec3( m_Classes[i].origin );
		savefile->ReadInt( m_Classes[i].nocollide );
		savefile->ReadBool( m_Classes[i].nocombine );
		savefile->ReadInt( m_Classes[i].falloff );
		savefile->ReadBool( m_Classes[i].floor );
		savefile->ReadBool( m_Classes[i].stack );
		savefile->ReadBool( m_Classes[i].noinhibit );
		savefile->ReadVec3( m_Classes[i].size );
		savefile->ReadVec3( m_Classes[i].color_min );
		savefile->ReadVec3( m_Classes[i].color_max );

		savefile->ReadFloat( m_Classes[i].defaultProb );

		savefile->ReadInt( num );
		m_Classes[i].skins.Clear();
		m_Classes[i].skins.SetNum( num );
		for( int j = 0; j < num; j++ )
		{
			savefile->ReadInt( m_Classes[i].skins[j] );
		}

		savefile->ReadInt( num );
		m_Classes[i].materials.Clear();
		m_Classes[i].materials.SetNum( num );
		for( int j = 0; j < num; j++ )
		{
			savefile->ReadString( m_Classes[i].materials[j].name );
			savefile->ReadFloat( m_Classes[i].materials[j].probability );
		}

		// only restore these if they are used
		if ( m_Classes[i].falloff == 4)
		{
			savefile->ReadFloat( m_Classes[i].func_x );
			savefile->ReadFloat( m_Classes[i].func_y );
			savefile->ReadFloat( m_Classes[i].func_s );
			savefile->ReadFloat( m_Classes[i].func_a );
			savefile->ReadInt( m_Classes[i].func_Xt );
			savefile->ReadInt( m_Classes[i].func_Yt );
			savefile->ReadInt( m_Classes[i].func_f );
			savefile->ReadFloat( m_Classes[i].func_min );
			savefile->ReadFloat( m_Classes[i].func_max );
		}
		savefile->ReadString( m_Classes[i].map );
		m_Classes[i].img = NULL;
	    if (!m_Classes[i].map.IsEmpty())
		{
			// image based distribution
			m_Classes[i].img = new CImage();
			m_Classes[i].img->LoadImage( m_Classes[i].map );
			m_Classes[i].img->InitImageInfo();
		}

		// TODO: read megamodel struct

		savefile->ReadBool( bHaveModel );
		m_Classes[i].hModel = NULL;
		// only read the model if it is actually used
		if ( bHaveModel )
		{
			savefile->ReadModel( m_Classes[i].hModel );
		}

		savefile->ReadBool( bHaveModel );
		m_Classes[i].physicsObj = NULL;
		// only read the model if it is actually used
		if ( bHaveModel )
		{
			m_Classes[i].physicsObj = new idPhysics_StaticMulti;
			m_Classes[i].physicsObj->Restore( savefile );
		}
	}

	savefile->ReadInt( num );
	m_Inhibitors.Clear();
	m_Inhibitors.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		savefile->ReadVec3( m_Inhibitors[i].origin );
		savefile->ReadBox( m_Inhibitors[i].box );
	}

	savefile->ReadInt( num );
	m_Skins.Clear();
	m_Skins.SetNum( num );
	for( int i = 0; i < num; i++ )
	{
		savefile->ReadString( m_Skins[i] );
	}

	savefile->ReadInt( m_iNumPVSAreas );
	for( int i = 0; i < m_iNumPVSAreas; i++ )
	{
		savefile->ReadInt( m_iPVSAreas[i] );
	}
}

/*
===============
Lode::RandomSeed

Implement our own, independent random generator with our own seed, so we are
independent from the seed in gameLocal and the one used in RandomFloat. This
one is used to calculate the seeds per-class:
===============
*/
ID_INLINE int Lode::RandomSeed( void ) {
	m_iSeed_2 = 1103515245L * m_iSeed_2 + 12345L;
	return m_iSeed_2 & 0x7FFFFFF;
}

/*
===============
Lode::RandomFloat

Implement our own random generator with our own seed, so we are independent
from the seed in gameLocal:
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
Lode::RandomFloatSqr - Random float between 0 and 1 with squared falloff
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

	gameLocal.Printf( "LODE %s: Sizes: lode_entity_t %i, lode_class_t %i, lod_data_t %i, idEntity %i, idStaticEntity %i.\n", 
			GetName(), sizeof(lode_entity_t), sizeof(lode_class_t), sizeof(lod_data_t), sizeof(idEntity), sizeof(idStaticEntity) );

//	idTraceModel *trace = GetPhysics()->GetClipModel()->GetTraceModel();
//	idVec3 o = trace->GetOrigin();
//	idVec3 s = trace->GetBounds().GetSize();
//	idAngles a = trace->GetAxis().ToAngles();

//	gameLocal.Printf( "LODE %s: Tracemodel origin %0.2f %0.2f %0.2f size %0.2f %0.2f %0.2f axis %s.\n", GetName(), o.x, o.y, o.z, s.x, s.y, s.z, a.ToString() );

	idBounds bounds = renderEntity.bounds;
	idVec3 size = bounds.GetSize();
	idAngles angles = renderEntity.axis.ToAngles();


	// cache in which PVS(s) we are, so we can later check if we are in Player PVS
	idBounds modelAbsBounds;
    modelAbsBounds.FromTransformedBounds( bounds, renderEntity.origin, renderEntity.axis );
	m_iNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, m_iPVSAreas, sizeof( m_iPVSAreas ) / sizeof( m_iPVSAreas[0] ) );

	gameLocal.Printf( "LODE %s: Seed %i Size %0.2f %0.2f %0.2f Axis %s, PVS count %i.\n", GetName(), m_iSeed, size.x, size.y, size.z, angles.ToString(), m_iNumPVSAreas );

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

	m_iDebug = spawnArgs.GetInt( "debug", "0" );
	m_bDebugColors = spawnArgs.GetBool( "debug_colors", "0" );

	m_bWaitForTrigger = spawnArgs.GetBool("wait_for_trigger", "0");

	m_DistCheckInterval = (int) (1000.0f * spawnArgs.GetFloat( "dist_check_period", "0.05" ));

	float cullRange = spawnArgs.GetFloat( "cull_range", "150" );
	gameLocal.Printf ("LODE %s: cull range = %0.2f.\n", GetName(), cullRange );

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
Lode::AddSkin - add one skin name to the skins list (if it isn't in there already), and return the index
===============
*/
int Lode::AddSkin( const idStr *skin )
{
	for( int i = 0; i < m_Skins.Num(); i++ )
	{
		if (m_Skins[i] == *skin)
		{
			return i;
		}
	}

	// not yet in list
	m_Skins.Append (*skin);
	return m_Skins.Num() - 1;
}

/*
===============
Lode::AddClassFromEntity - take an entity as template and add a class from it. Returns the size of this class
===============
*/
float Lode::AddClassFromEntity( idEntity *ent, const int iEntScore )
{
	lode_class_t			LodeClass;
	lode_material_t			LodeMaterial;
	idStr falloff;
	const idKeyValue *kv;

	LodeClass.pseudo = false;		// this is a true entity class
	LodeClass.score = iEntScore;
	LodeClass.classname = ent->GetEntityDefName();
	LodeClass.modelname = ent->spawnArgs.GetString("model","");
	LodeClass.megamodel = NULL;
	
	LodeClass.nocombine = ent->spawnArgs.GetBool("lode_combine","1") ? false : true;

	// never combine moveables
	if ( ent->IsType( idMoveable::Type ) )
	{
		LodeClass.nocombine = false;
	}

	// only for pseudo classes
	LodeClass.physicsObj = NULL;
	
	// get all "skin" and "skin_xx" spawnargs

	LodeClass.skins.Clear();
	// if no skin spawnarg exists, add the empty skin so we at least have one entry
	if ( ! ent->spawnArgs.FindKey("skin") )
	{
		LodeClass.skins.Append ( 0 );
	}
   	kv = ent->spawnArgs.MatchPrefix( "skin", NULL );
	while( kv )
	{
		// find the proper skin index
		idStr skin = kv->GetValue();
		int skinIdx = AddSkin( &skin );
		gameLocal.Printf( "LODE %s: Adding skin '%s' (idx %i) to class.\n", GetName(), skin.c_str(), skinIdx );
		LodeClass.skins.Append ( skinIdx );
		kv = ent->spawnArgs.MatchPrefix( "skin", kv );
	}

	// Do not use GetPhysics()->GetOrigin(), as the LOD system might have shifted
	// the entity already between spawning and us querying the info:
	LodeClass.origin = ent->spawnArgs.GetVector( "origin" );
	// these are ignored for pseudo classes (e.g. watch_breathren):
	LodeClass.floor = ent->spawnArgs.GetBool( "lode_floor", spawnArgs.GetString( "floor", "0") );
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
	else
	{
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
	}

	LodeClass.func_x = 0;
	LodeClass.func_y = 0;
	LodeClass.func_s = 0;
	LodeClass.func_a = 0;
	LodeClass.func_Xt = 0;
	LodeClass.func_Yt = 0;
	LodeClass.func_f = 0;
	if (falloff == "func")
	{
		LodeClass.falloff = 4;
		// default is 0.5 * (x + y + 0)
		LodeClass.func_a = ent->spawnArgs.GetFloat( "lode_func_a", spawnArgs.GetString( "func_a", "0") );
		LodeClass.func_s = ent->spawnArgs.GetFloat( "lode_func_s", spawnArgs.GetString( "func_s", "0.5") );
		LodeClass.func_Xt = 1;			// 1 - X, 2 -> X * X
		idStr x = ent->spawnArgs.GetString( "lode_func_Xt", spawnArgs.GetString( "func_Xt", "X") );
		if (x == "X*X")
		{
			LodeClass.func_Xt = 2;		// 1 - X, 2 -> X * X
		}
		LodeClass.func_Yt = 1;			// 1 - X, 2 -> X * X
		x = ent->spawnArgs.GetString( "lode_func_Yt", spawnArgs.GetString( "func_Yt", "Y") );
		if (x == "Y*Y")
		{
			LodeClass.func_Yt = 2;		// 1 - Y, 2 -> Y * Y
		}
		LodeClass.func_x = ent->spawnArgs.GetFloat( "lode_func_x", spawnArgs.GetString( "func_x", "1") );
		LodeClass.func_y = ent->spawnArgs.GetFloat( "lode_func_y", spawnArgs.GetString( "func_y", "1") );
		LodeClass.func_min = ent->spawnArgs.GetFloat( "lode_func_min", spawnArgs.GetString( "func_min", "0") );
		LodeClass.func_max = ent->spawnArgs.GetFloat( "lode_func_max", spawnArgs.GetString( "func_max", "1.0") );
		if (LodeClass.func_min < 0.0f)
		{
			gameLocal.Warning ("LODE %s: func_min %0.2f < 0, setting it to 0.\n", GetName(), LodeClass.func_min );
			LodeClass.func_min = 0.0f;
		}
		if (LodeClass.func_max > 1.0f)
		{
			gameLocal.Warning ("LODE %s: func_max %0.2f < 1.0, setting it to 1.0.\n", GetName(), LodeClass.func_max );
			LodeClass.func_max = 1.0f;
		}
		if (LodeClass.func_min > LodeClass.func_max)
		{
			gameLocal.Warning ("LODE %s: func_min %0.2f > func_max %0.2f, setting it to 0.\n", GetName(), LodeClass.func_min, LodeClass.func_max );
			LodeClass.func_min = 0.0f;
		}

		x = ent->spawnArgs.GetString( "lode_func_f", spawnArgs.GetString( "func_f", "clamp") );
		if (x == "clamp")
		{
			LodeClass.func_f = 1;
		}
		else if (x != "zeroclamp")
		{
			gameLocal.Error ("LODE %s: func_clamp is invalid, expected 'clamp' or 'zeroclamp', found '%s'\n", GetName(), x.c_str() );
		}
		gameLocal.Warning ("LODE %s: Using falloff func p = %s( %0.2f, %0.2f, %0.2f * ( %s * %0.2f + %s * %0.2f + %0.2f) )\n", 
				GetName(), x.c_str(), LodeClass.func_min, LodeClass.func_max, LodeClass.func_s, LodeClass.func_Xt == 1 ? "X" : "X*X", LodeClass.func_x, 
				LodeClass.func_Yt == 1 ? "Y" : "Y*Y", LodeClass.func_y, LodeClass.func_a );
	}
	if (LodeClass.falloff == -1)
	{
		gameLocal.Warning ("LODE %s: Invalid falloff %s, expect one of 'none', 'cutoff', 'square', 'exp' or 'func'.\n", GetName(), falloff.c_str() );
		LodeClass.falloff = 0;
	}

	// image based map?
	LodeClass.map = ent->spawnArgs.GetString( "lode_falloff_map", spawnArgs.GetString( "falloff_map", "") );
	LodeClass.img = NULL;
	// starts with "textures" => image based map
	if ( ! LodeClass.map.IsEmpty())
	{
		LodeClass.img = new CImage();
		LodeClass.img->LoadImage( LodeClass.map );
		LodeClass.img->InitImageInfo();

		gameLocal.Printf("LODE %s: Loaded %i x %i pixel image with %i bpp = %li bytes.\n", 
				GetName(), LodeClass.img->m_Width, LodeClass.img->m_Height, LodeClass.img->m_Bpp, LodeClass.img->GetBufferLen() );

		if (LodeClass.img->m_Bpp != 1)
		{
			gameLocal.Error("LODE %s: Bytes per pixel must be 1 but is %i!\n", GetName(), LodeClass.img->m_Bpp );
		}
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

	if (LodeClass.size.x == 0)
	{
			gameLocal.Warning( "LODE %s: Size == 0 for class.\n", GetName() );
	}
	// gameLocal.Printf( "LODE %s: size of class %i: %0.2f %0.2f\n", GetName(), i, LodeClass.size.x, LodeClass.size.y );
	// TODO: use a projection along the "floor-normal"
	// TODO: multiply the average class size with the class score (so little used entities don't "use" more space)

	// gameLocal.Printf( "LODE %s: Entity class size %0.2f %0.2f %0.2f\n", GetName(), LodeClass.size.x, LodeClass.size.y, LodeClass.size.z );
	LodeClass.cullDist = 0;
	LodeClass.spawnDist = 0;
	float hideDist = ent->spawnArgs.GetFloat( "hide_distance", "0" );
	float cullRange = ent->spawnArgs.GetFloat( "lode_cull_range", spawnArgs.GetString( "cull_range", "150" ) );
	if (cullRange > 0 && hideDist > 0)
	{
		LodeClass.cullDist = hideDist + cullRange;
		LodeClass.spawnDist = hideDist + (cullRange / 2);
		// square for easier compare
		LodeClass.cullDist *= LodeClass.cullDist;
		LodeClass.spawnDist *= LodeClass.spawnDist;
	}

	LodeClass.materials.Clear();

	// The default probability for all materials not matching anything in materials:
	LodeClass.defaultProb = ent->spawnArgs.GetFloat( "lode_probability", spawnArgs.GetString( "probability", "1.0" ) );

	// all probabilities for the different materials
	kv = ent->spawnArgs.MatchPrefix( "lode_material_", NULL );
	while( kv ) {
		// "lode_material_grass" => "grass"
		LodeMaterial.name = kv->GetKey().Mid( 14, kv->GetKey().Length() - 14 );
		// "lode_material_grass" "1.0" => 1.0
		LodeMaterial.probability = ent->spawnArgs.GetFloat( kv->GetKey(), "1.0");
		if (LodeMaterial.probability < 0 || LodeMaterial.probability > 1.0)
		{
			gameLocal.Warning( "LODE %s: Invalid probability %0.2f (should be 0 .. 1.0) for material %s, ignoring it.\n",
					GetName(), LodeMaterial.probability, LodeMaterial.name.c_str() );
		}
		else
		{
			//gameLocal.Warning( "LODE %s: Using material %s, probability %0.2f (%s)\n",
			//		GetName(), LodeMaterial.name.c_str(), LodeMaterial.probability, kv->GetKey().c_str() );
			LodeClass.materials.Append( LodeMaterial );
		}
		kv = ent->spawnArgs.MatchPrefix( "lode_material_", kv );
	}

	// has a model with shared data?
	LodeClass.hModel = NULL;
	if (LodeClass.classname == FUNC_STATIC)
	{
		// simply point to the already existing model:
		LodeClass.hModel = ent->GetRenderEntity()->hModel;
		// prevent a double free
		ent->GetRenderEntity()->hModel = NULL;
		LodeClass.classname = FUNC_DUMMY;
	}

	// uses color variance?
	// fall back to LODE "color_mxx", if not set, fall back to entity color, if this is unset, use 1 1 1
	LodeClass.color_min  = ent->spawnArgs.GetVector("lode_color_min", spawnArgs.GetString("color_min", ent->spawnArgs.GetString("_color", "1 1 1")));
	LodeClass.color_max  = ent->spawnArgs.GetVector("lode_color_max", spawnArgs.GetString("color_max", ent->spawnArgs.GetString("_color", "1 1 1")));

    LodeClass.color_min.Clamp( idVec3(0,0,0), idVec3(1,1,1) );
    LodeClass.color_max.Clamp( LodeClass.color_min, idVec3(1,1,1) );

	// all data setup, append to the list
	m_Classes.Append ( LodeClass );

	gameLocal.Printf( "LODE %s: Adding class %s.\n", GetName(), LodeClass.classname.c_str() );

	// if falloff != none, correct the density, because the ellipse-shape is smaller then the rectangle
	float size = (LodeClass.size.x + LodeClass.spacing) * (LodeClass.size.y + LodeClass.spacing);

	if ( LodeClass.falloff >= 1 && LodeClass.falloff <= 3)
	{
		// Rectangle is W * H, ellipse is W/2 * H/2 * PI. When W = H = 1, then the rectangle
		// area is 1.0, and the ellipse 0.785398, so correct for 1 / 0.785398, this will
		// reduce the density, and thus the entity count:
		size *= 1.2732f; 
	}

	// scale the per-class size by the per-class density
	float fDensity = ent->spawnArgs.GetFloat( "lode_density", "1.0" );
	if (fDensity <= 0.0001)
	{
		fDensity = 0.0001;
	}

	return size / fDensity;
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
		if (lod_bias < 0.7)
		{
			lod_bias *= 1.4f;									// 0.5 => 0.7
		}
		else
		{
			lod_bias *= 1.2f;									// 0.75 => 0.90
		}
	}
	else if (lod_bias > 1.0f)
	{
																// 1.5, 2, 3 => 1.13, 1.25, 1.4
		lod_bias = ( lod_bias > 2.0f ? 0.9f : 1.0f) + ((lod_bias - 1.0f) / 4.0f);
	}

	// 0.7, 0.9, 1.0, 1.13, 1.25, 1.4
	return lod_bias;
}

/*
===============
Compute the max. number of entities that we manage
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

		if (fDensity < 0.00001)
		{
			fDensity = 0.00001;
		}

		// m_fAvgSize is corrected for "per-class" falloff already
		m_iNumEntities = fDensity * (size.x * size.y) / m_fAvgSize;		// naive asumption each entity covers on avg X units
		gameLocal.Printf( "LODE %s: Dynamic entity count: %0.2f * %0.2f * %0.2f / %0.2f = %i.\n", GetName(), fDensity, size.x, size.y, m_fAvgSize, m_iNumEntities );

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
	lode_inhibitor_t LodeInhibitor;

/*	if ( targets.Num() == 0 )
	{
		gameLocal.Warning( "LODE %s has no targets!", GetName() );
		// TODO: somehow does not work?
		BecomeInactive( TH_THINK );
		// early out
		m_iNumEntities = -1;
		return;
	}
*/

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
				// or rotation spawnarg. Use clipmodel instead? Note: Unrotating the entity, but adding an "axis"
				// spawnarg works.
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
				AddClassFromEntity( ent, 0 );

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
				m_fAvgSize += AddClassFromEntity( ent, iEntScore );
			}
		}
	}

	// the same, but this time for the "spawn_class/spawn_count/spawn_skin" spawnargs:
	
	idVec3 origin = GetPhysics()->GetOrigin();

	const idKeyValue *kv = spawnArgs.MatchPrefix( "spawn_class", NULL );
	while( kv )
	{
		idStr entityClass = kv->GetValue();

		// spawn an entity of this class so we can copy it's values
		// TODO: avoid the spawn for speed reasons?

		const char* pstr_DefName = entityClass.c_str();
		const idDict *p_Def = gameLocal.FindEntityDefDict( pstr_DefName, false );
		if( p_Def )
		{
			idEntity *ent;
			idDict args;

			args.Set("classname", entityClass);
			// move to origin of ourselfs
			args.SetVector("origin", origin);

			// want it floored
			args.Set("lode_floor", "1");

			// set previously defined (possible random) skin
			// spawn_classX => spawn_skinX
			idStr skin = idStr("spawn_skin") + kv->GetKey().Mid( 11, kv->GetKey().Length() - 11 );

			// spawn_classX => "abc, def, '', abc"
			skin = spawnArgs.GetString( skin, "" );
			// select one at random
			skin = skin.RandomPart();

			//gameLocal.Printf("Using random skin '%s'.\n", skin.c_str() );
			args.Set( "skin", skin );

			// TODO: if the entity contains a "random_skin", too, use the info from there, then remove it
			args.Set( "random_skin", "" );

			gameLocal.SpawnEntityDef( args, &ent );
			if (ent)
			{
				int iEntScore = ent->spawnArgs.GetInt( "lode_score", "1" );
				if (iEntScore < 0)
				{
					gameLocal.Warning( "LODE %s: Target %s has invalid lode_score %i!\n", GetName(), ent->GetName(), iEntScore );
				}
				else
				{
					// add a class based on this entity
					m_iScore += iEntScore;
					m_fAvgSize += AddClassFromEntity( ent, iEntScore );
				}
				// remove the temp. entity 
				ent->PostEventMS( &EV_Remove, 0 );
			}
			else
			{
				gameLocal.Warning("LODE %s: Could not spawn entity from class %s to add it as my target.\n", 
						GetName(), entityClass.c_str() );
			}
		}
		else
		{
				gameLocal.Warning("LODE %s: Could not find entity def for class %s to add it as my target.\n", 
						GetName(), entityClass.c_str() );
		}

		// next one please
		kv = spawnArgs.MatchPrefix( "spawn_class", kv );
	}

	// increase the avg by X% to allow some spacing (even if spacing = 0)
	m_fAvgSize *= 1.3f;

	// (32 * 32 + 64 * 64 ) / 2 => 50 * 50
	m_fAvgSize /= m_Classes.Num();

	// avoid values too small or even 0
	if (m_fAvgSize < 2)
	{
		m_fAvgSize = 2;
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
	m_iSeed_2 = spawnArgs.GetInt( "seed", "0" );
    if (m_iSeed_2 == 0)
	{
		// The randseed upon loading a map seems to be always 0, so 
		// gameLocal.random.RandomInt() always returns 1 hence it is unusable:
		// add the entity number so that different lodes spawned in the same second
		// don't display the same pattern
		unsigned long seconds = (unsigned long) time (NULL) + (unsigned long) entityNumber;
	    m_iSeed_2 = (int) (1664525L * seconds + 1013904223L) & 0x7FFFFFFFL;
	}

	// to restart the same sequence, f.i. when the user changes level of detail in GUI
	m_iOrgSeed = m_iSeed_2;

	PrepareEntities();

	// remove all our targets from the game
	for( int i = 0; i < targets.Num(); i++ )
	{
		idEntity *ent = targets[ i ].GetEntity();
		if (ent)
		{
			// TODO: SafeRemove?
			ent->PostEventMS( &EV_Remove, 0 );
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
			SpawnEntity(i, false);		// spawn as unmanaged
		}

		// clear out memory just to be sure
		ClearClasses();
		m_Entities.Clear();
		m_iNumEntities = -1;

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
			m_iNumEntities = -1;
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

	int start = (int) time (NULL);

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

	gameLocal.Printf( "LODE %s: Seed %i Size %0.2f %0.2f %0.2f Axis %s.\n", GetName(), m_iSeed_2, size.x, size.y, size.z, angles.ToString() );

	m_Entities.Clear();
	if (m_iNumEntities > 100)
	{
		// TODO: still O(N*N) time, tho
		m_Entities.SetGranularity( 64 );	// we append potentially thousands of entries, and every $granularity step
											// the entire list is re-allocated and copied over again, so avoid this
	}
	LodeEntityBounds.Clear();
	LodeEntityBoxes.Clear();

	m_iNumExisting = 0;
	m_iNumVisible = 0;

	// Compute a random order of classes, so that when the mapper specified m_iNumEntities well below
	// the limit (e.g. 1), he gets a random entity from a random class (and not just one from the first
	// class always):

	// remove pseudo classes as we start over fresh
	idList< lode_class_t > newClasses;

	for (int i = 0; i < m_Classes.Num(); i++)
	{
		if (m_Classes[i].pseudo)
		{
			renderModelManager->FreeModel( m_Classes[i].hModel );
			m_Classes[i].hModel = NULL;
			continue;
		}
		newClasses.Append ( m_Classes[i] );
	}
	m_Classes.Swap( newClasses );		// copy over
	newClasses.Clear();					// remove

	// random shuffle the class indexes around
	// also calculate the per-class seed:
	ClassIndex.Clear();			// random shuffling of classes
	for (int i = 0; i < m_Classes.Num(); i++)
	{
		ClassIndex.Append ( i );				// 1,2,3,...
		m_Classes[i].seed = RandomSeed();		// random generator 2 inits the random generator 1
	}

	// shuffle all entries, but use the second generator for a "predictable" class sequence
	// that does not change when the menu changes
	m_iSeed = RandomSeed();
	s = m_Classes.Num();
	for (int i = 0; i < s; i++)
	{
		int second = (int)(RandomFloat() * s);
		int temp = ClassIndex[i]; ClassIndex[i] = ClassIndex[second]; ClassIndex[second] = temp;
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

		m_iSeed = m_Classes[i].seed;		// random generator 2 inits the random generator 1

		int iEntities = m_iNumEntities * (static_cast<float>(m_Classes[i].score)) / m_iScore;	// sum 2, this one 1 => 50% of this class
		// try at least one from each class (so "select 1 from 4 classes" works correctly)
		if (iEntities == 0)
		{
			iEntities = 1;
		}
		gameLocal.Printf( "LODE %s: Creating %i entities of class %s (#%i index %i, seed %i).\n", GetName(), iEntities, m_Classes[i].classname.c_str(), i, idx, m_iSeed );

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
					switch (m_Classes[i].falloff)
					{
					case 0:
					case 4:
						// restrict to our AAB (unrotated)
						LodeEntity.origin = idVec3( (RandomFloat() - 0.5f) * size.x, (RandomFloat() - 0.5f) * size.y, 0 );
						break;
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
						// 3 - exp
						LodeEntity.origin = idPolar3( RandomFloatExp( 2.0f ), RandomFloat() * 360.0f, 0 ).ToVec3();
						break;
					}
					if (m_Classes[i].falloff > 0 && m_Classes[i].falloff < 4)
					{
						// cutoff, square and exp: scale the result to our box size
						LodeEntity.origin.x *= size.x / 2;
						LodeEntity.origin.y *= size.y / 2;
					}
				}

				// what is the probability it will appear here?
				float probability = 1.0f;	// start with "always"

				// if falloff == 4, compute the falloff probability
       			if (m_Classes[i].falloff == 4)
				{
					// p = s * (Xt * x + Yt * y + a)
					float x = (LodeEntity.origin.x / size.x) + 0.5f;		// 0 .. 1.0
					if (m_Classes[i].func_Xt == 2)
					{
						x *= x;							// 2 => X*X
					}

					float y = (LodeEntity.origin.y / size.y) + 0.5f;		// 0 .. 1.0
					if (m_Classes[i].func_Yt == 2)
					{
						y *= y;							// 2 => X*X
					}

					float p = m_Classes[i].func_s * ( x * m_Classes[i].func_x + y * m_Classes[i].func_y + m_Classes[i].func_a);
					// apply custom clamp function
					if (m_Classes[i].func_f == 0)
					{
						if (p < m_Classes[i].func_min || p > m_Classes[i].func_max)
						{
							// outside range, zero-clamp
							//probability = 0.0f;
							// placement will fail, anyway:
							gameLocal.Printf ("LODE %s: Skipping placement, probability == 0 (min %0.2f, p=%0.2f, max %0.2f).\n", 
									GetName(), m_Classes[i].func_min, p, m_Classes[i].func_max );
							continue;
						}
					}
					else
					{
						// clamp to min .. max
						probability = idMath::ClampFloat( m_Classes[i].func_min, m_Classes[i].func_max, p );
					}
					gameLocal.Printf ("LODE %s: falloff func gave p = %0.2f (clamped %0.2f)\n", GetName(), p, probability);
				}

       			// image based falloff probability
				if (m_Classes[i].img)
				{
					// compute the pixel we need to query
					float x = (LodeEntity.origin.x / size.x) + 0.5f;		// 0 .. 1.0
					float y = (LodeEntity.origin.y / size.y) + 0.5f;		// 0 .. 1.0

					// 1 - x to correct for top-left images
					int px = (1.0f - x) * m_Classes[i].img->m_Width;		// 0 .. w (f.i. 0 .. 1024)
					int py = y * m_Classes[i].img->m_Height;				// 0 .. h (f.i. 0 .. 1024)

					// calculate the correct offset
					//int ofs = m_Classes[i].img->m_Bpp * (py * m_Classes[i].img->m_Height + px);
					// Bpp is 1
					int ofs = (py * m_Classes[i].img->m_Height + px);

					unsigned char *imgData = m_Classes[i].img->GetImage();
					int value = imgData[ofs];
					//gameLocal.Printf("LODE %s: Pixel at %i, %i (ofs = %i) has value %i (p=%0.2f).\n", GetName(), px, py, ofs, value, (float)value / 256.0f);
					probability *= (float)value / 256.0f;

					if (probability < 0.000001)
					{
						// p too small, continue instead of doing expensive material checks
						continue;
					}
				}

				// Rotate around our rotation axis (to support rotated LODE brushes)
				LodeEntity.origin *= axis;

				// add origin of the LODE
				LodeEntity.origin += origin;

				// should only appear on certain ground material(s)?
				if (m_Classes[i].materials.Num() > 0)
				{
					// end of the trace (downwards the length from entity class position to bottom of LODE)
					idVec3 traceEnd = LodeEntity.origin; traceEnd.z = origin.z - size.z;
					// TODO: adjust for different "down" directions
					//vTest *= GetGravityNormal();

					trace_t trTest;
					idVec3 traceStart = LodeEntity.origin;

					//gameLocal.Printf ("LODE %s: TracePoint start %0.2f %0.2f %0.2f end %0.2f %0.2f %0.2f\n",
					//		GetName(), traceStart.x, traceStart.y, traceStart.z, traceEnd.x, traceEnd.y, traceEnd.z );
					gameLocal.clip.TracePoint( trTest, traceStart, traceEnd, 
							CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_OPAQUE | CONTENTS_MOVEABLECLIP, this );

					// Didn't hit anything?
					if ( trTest.fraction < 1.0f )
					{
						const idMaterial *mat = trTest.c.material;

						surfTypes_t type = mat->GetSurfaceType();
						idStr descr = "";

						// in case the description is empty
						switch (type)
						{
							case SURFTYPE_METAL:
								descr = "metal";
								break;
							case SURFTYPE_STONE:
								descr = "stone";
								break;
							case SURFTYPE_FLESH:
								descr = "flesh";
								break;
							case SURFTYPE_WOOD:
								descr = "wood";
								break;
							case SURFTYPE_CARDBOARD:
								descr = "cardboard";
								break;
							case SURFTYPE_LIQUID:
								descr = "liquid";
								break;
							case SURFTYPE_GLASS:
								descr = "glass";
								break;
							case SURFTYPE_PLASTIC:
								descr = "plastic";
								break;
							case SURFTYPE_15:
								descr = mat->GetDescription();
								break;
							default:
								break;
						}

						// hit something
						//gameLocal.Printf ("LODE %s: Hit something at %0.2f (%0.2f %0.2f %0.2f material %s (%s))\n",
						//	GetName(), trTest.fraction, trTest.endpos.x, trTest.endpos.y, trTest.endpos.z, descr.c_str(), mat->GetName() );

						float p = m_Classes[i].defaultProb;		// the default if nothing hits

						// see if this entity is inhibited by this material
						for (int e = 0; e < m_Classes[i].materials.Num(); e++)
						{
							// starts with the same as the one we look at?
							if ( m_Classes[i].materials[e].name.Find( descr ) == 0 )
							{
								p = m_Classes[i].materials[e].probability;

								//gameLocal.Printf ("LODE %s: Material (%s) matches class material %i (%s), using probability %0.2f\n",
								//		GetName(), descr.c_str(), e, m_Classes[i].materials[e].name.c_str(), probability); 
								// found a match, break
								break;
							}	
						}

						//gameLocal.Printf ("LODE %s: Using probability %0.2f.\n", GetName(), p );

						// multiply probability with p (so 0.5 * 0.5 results in 0.25)
						probability *= p;

						// TODO: height based probability, angle-of-surface probability

					}	
					else
					{
						// didn't hit anything, floating in air?
					}

				} // end of per-material probability

				// gameLocal.Printf ("LODE %s: Using final p=%0.2f.\n", GetName(), probability );
				// check against the probability (0 => always skip, 1.0 - never skip, 0.5 - skip half)
				float r = RandomFloat();
				if (r > probability)
				{
					//gameLocal.Printf ("LODE %s: Skipping placement, %0.2f > %0.2f.\n", GetName(), r, probability);
					continue;
				}

				if (m_Classes[i].floor)
				{
					gameLocal.Printf( "LODE %s: Flooring entity #%i.\n", GetName(), j );

					// end of the trace (downwards the length from entity class position to bottom of LODE)
					idVec3 traceEnd = LodeEntity.origin; traceEnd.z = origin.z - size.z;
					// TODO: adjust for different "down" directions
					//vTest *= GetGravityNormal();

					// bounds of the class entity
					idVec3 b_1 = - m_Classes[i].size / 2;
					idVec3 b_2 = m_Classes[i].size / 2;
					// assume the entity origin is at the entity bottom
					b_1.z = 0;
					b_2.z = m_Classes[i].size.z;
					idBounds class_bounds = idBounds( b_1, b_2 );
					trace_t trTest;

					idVec3 traceStart = LodeEntity.origin;

					//gameLocal.Printf ("LODE %s: TraceBounds start %0.2f %0.2f %0.2f end %0.2f %0.2f %0.2f bounds %s\n",
					//		GetName(), traceStart.x, traceStart.y, traceStart.z, traceEnd.x, traceEnd.y, traceEnd.z,
					//	   	class_bounds.ToString()	); 
					gameLocal.clip.TraceBounds( trTest, traceStart, traceEnd, class_bounds, 
							CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_OPAQUE | CONTENTS_MOVEABLECLIP, this );

					// Didn't hit anything?
					if ( trTest.fraction != 1.0f )
					{
						// hit something
						//gameLocal.Printf ("LODE %s: Hit something at %0.2f (%0.2f %0.2f %0.2f)\n",
						//	GetName(), trTest.fraction, trTest.endpos.x, trTest.endpos.y, trTest.endpos.z ); 
						LodeEntity.origin = trTest.endpos;
						LodeEntity.angles = trTest.endAxis.ToAngles();

						// TODO: If the model bounds are quite big, but the model itself is "thin"
						// at the bottom (like a tree with a trunk), then the model will "float"
						// in the air. A "min_sink" value can fix this, but only for small inclines.
						// A pine on a 30° slope might still hover 12 units in the air. Let the mapper
						// override the bounds used for collision checks? For instance using a cylinder
						// would already help, using a smaller diameter would help even more.
						// Or could we trace agains the real model?
					}
					else
					{
						// hit nothing
						gameLocal.Printf ("LODE %s: Hit nothing at %0.2f (%0.2f %0.2f %0.2f)\n",
							GetName(), trTest.fraction, LodeEntity.origin.x, LodeEntity.origin.y, LodeEntity.origin.z );
					}
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

					if (tries < MAX_TRIES && m_iDebug > 0)
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

			// compute a random color value
			LodeEntity.color = m_Classes[i].color_max - m_Classes[i].color_min; 
			LodeEntity.color.x = LodeEntity.color.x * RandomFloat() + m_Classes[i].color_min.x;
			LodeEntity.color.y = LodeEntity.color.y * RandomFloat() + m_Classes[i].color_min.y;
			LodeEntity.color.z = LodeEntity.color.z * RandomFloat() + m_Classes[i].color_min.z;

			// choose skin randomly
			LodeEntity.skinIdx = m_Classes[i].skins[ RandomFloat() * m_Classes[i].skins.Num() ];
			//gameLocal.Printf( "LODE %s: Using skin %i.\n", GetName(), LodeEntity.skinIdx );
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
				// TODO: support "random_skin" here by using GetCurrentSkin()
				idStr skin = ent->spawnArgs.GetString("skin","");
				LodeEntity.skinIdx = AddSkin( &skin );
				// already exists
				LodeEntity.hidden = false;
				LodeEntity.exists = true;
				LodeEntity.entity = j;
				LodeEntity.classIdx = i;
				m_Entities.Append( LodeEntity );
			}
		}
	}

	int end = (int) time (NULL);
	gameLocal.Printf("LODE %s: Preparing %i entities took %i seconds.\n", GetName(), m_Entities.Num(), end - start );

	// combine the spawned entities into megamodels if possible
	CombineEntities();
}

// sort a list of offsets by their distance
int SortOffsetsByDistance( const model_ofs_t *a, const model_ofs_t *b ) {
	float d = a->offset.LengthSqr() - b->offset.LengthSqr();

	if ( d < 0 ) {
		return -1;
	}
	if ( d > 0 ) {
		return 1;
	}
	return 0;
}

void Lode::CombineEntities( void )
{
	bool multiPVS = m_iNumPVSAreas > 1 ? true : false;
	idList < int > pvs;								//!< in which PVS is this entity?
	idBounds modelAbsBounds;						//!< for per-entity PVS check
	idBounds entityBounds;							//!< for per-entity PVS check
	int iNumPVSAreas = 0;							//!< for per-entity PVS check
	int iPVSAreas[2];								//!< for per-entity PVS check
	lode_class_t PseudoClass;
	idList< lode_entity_t > newEntities;
	unsigned int mergedCount = 0;
	idList < model_ofs_t > offsets;					//!< To merge the other entities into the first, record their offset and angle
	model_ofs_t ofs;
	idList<int> tobedeleted;

	float max_combine_distance = spawnArgs.GetFloat("combine_distance", "1024");
	if (max_combine_distance < 10)
	{
		gameLocal.Warning("LODE %s: combine distance %0.2f < 10, enforcing minimum 10.\n", GetName(), max_combine_distance);
		max_combine_distance = 10;
	}
	// square for easier comparing
	max_combine_distance *= max_combine_distance;

	if ( ! spawnArgs.GetBool("combine", "0"))
	{
		gameLocal.Printf("LODE %s: combine = 0, skipping combine step.\n", GetName() );
		return;
	}

	int start = (int) time (NULL);

	// for each entity, find out in which PVS it is, unless we only have one PVS on the lode,
	// we then expect all entities to be in the same PVS, too:
	if (multiPVS)
	{
		gameLocal.Printf("LODE %s: MultiPVS.\n", GetName() );
		pvs.Clear();
		// O(N)
		for (int i = 0; i < m_Entities.Num(); i++)
		{
			// find out in which PVS this entity is
			idVec3 size = m_Classes[ m_Entities[i].classIdx ].size / 2; 
		    modelAbsBounds.FromTransformedBounds( idBounds( -size, size ), m_Entities[i].origin, m_Entities[i].angles.ToMat3() );
			int iNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, iPVSAreas, sizeof( iPVSAreas ) / sizeof( iPVSAreas[0] ) );
			if (iNumPVSAreas > 1)
			{
				// more than one PVS area, never combine this entity
				pvs.Append(-1);
			}
			else
			{
				// remember this value
				pvs.Append( iPVSAreas[0] );
			}
		}
	}
	else
	{
		gameLocal.Printf("LODE %s: SinglePVS.\n", GetName() );
	}

	idRenderModel* tempModel = NULL;

	int n = m_Entities.Num();
	// we mark all entities that we combine with another entity with "-1" in the classIdx
	for (int i = 0; i < n - 1; i++)
	{
		unsigned int merged = 0;				//!< merged 0 other entities into this one

		//gameLocal.Printf("LODE %s: At entity %i\n", GetName(), i);
		if (m_Entities[i].classIdx < 0)
		{
			// already combined, skip
			//gameLocal.Printf("LODE %s: Entity %i already combined into another entity, skipping it.\n", GetName(), i);
			continue;
		}

		const lode_class_t * entityClass = & m_Classes[ m_Entities[i].classIdx ];

		// if this class says no combine, skit it
		if (entityClass->nocombine)
		{
			continue;
		}
		offsets.Clear();
		offsets.SetGranularity(64);	// we might have a few hundred entities in there

		ofs.offset = idVec3(0,0,0); // the first copy is the original
		ofs.color  = PackColor( m_Entities[i].color );
		ofs.angles = m_Entities[i].angles;
		ofs.lod = 0;
		offsets.Append(ofs);

		tobedeleted.Clear();
		tobedeleted.SetGranularity(64);	// we might have a few hundred entities in there

		tempModel = entityClass->hModel;
		if (NULL == tempModel)
		{
			// load model, then combine away
			tempModel = renderModelManager->FindModel( entityClass->modelname );
			if (! tempModel)
			{
				gameLocal.Warning("LODE %s: Could not load model %s for entity %i, skipping it.\n", GetName(), entityClass->modelname.c_str(), i);
				continue;
			}
		}

		// how many can we combine at most?
		// TODO: use LOD 0 here for an worse-case estimate
		unsigned int maxModelCount = gameLocal.m_ModelGenerator->GetMaxModelCount( tempModel );
		gameLocal.Printf("LODE %s: Combining at most %u models for entity %i.\n", GetName(), maxModelCount, i );

		// try to combine as much entities into this one
		// O(N*N) performance, but only if we don't combine any entities, otherwise
		// every combine step reduces the number of entities to look at next:
		for (int j = i + 1; j < n; j++)
		{
			//gameLocal.Printf("LODE %s: %i: At entity %i\n", GetName(), i, j);
			if (m_Entities[j].classIdx == -1)
			{
				// already combined, skip
				//gameLocal.Printf("LODE %s: Entity %i already combined into another entity, skipping it.\n", GetName(), j);
				continue;
			}
			if (m_Entities[j].classIdx != m_Entities[i].classIdx)
			{
				// have different classes
				gameLocal.Printf("LODE %s: Entity classes from %i (%i) and %i (%i) differ, skipping it.\n", GetName(), i, m_Entities[i].classIdx, j, m_Entities[j].classIdx);
				continue;
			}
			if (m_Entities[j].skinIdx != m_Entities[i].skinIdx)
			{
				// have different classes
				gameLocal.Printf("LODE %s: Entity skins from %i and %i differ, skipping it.\n", GetName(), i, j);
				continue;
			}
			// in different PVS?
			if ( multiPVS && pvs[j] != pvs[i])
			{
				gameLocal.Printf("LODE %s: Entity %i in different PVS than entity %i, skipping it.\n", GetName(), j, i);
				continue;
			}
			// distance too big?
			idVec3 dist = (m_Entities[i].origin - m_Entities[j].origin);
			float distSq = dist.LengthSqr();
			if (distSq > max_combine_distance)
			{
				// gameLocal.Printf("LODE %s: Distance from entity %i to entity %i to far (%f > %f), skipping it.\n", GetName(), j, i, dist.Length(), max_combine_distance );
				continue;
			}

			ofs.offset = dist;
			ofs.angles = m_Entities[j].angles;
			ofs.color  = PackColor( m_Entities[j].color );
			ofs.lod    = 0;
			offsets.Append( ofs );

			if (merged == 0)
			{
				PseudoClass.pseudo = true;
				PseudoClass.spawnDist = entityClass->spawnDist;
				PseudoClass.cullDist = entityClass->cullDist;
				PseudoClass.size = entityClass->size;
				PseudoClass.img = NULL;
				// a combined entity must be of this class
				PseudoClass.classname = entityClass->classname;
				PseudoClass.hModel = NULL;
				PseudoClass.physicsObj = new idPhysics_StaticMulti;
				PseudoClass.megamodel = NULL;
			}
			// for this entity
			merged ++;
			// overall
			mergedCount ++;
//			gameLocal.Printf("LODE %s: Merging entity %i (origin %s) into entity %i (origin %s, dist %s).\n", 
//					GetName(), j, m_Entities[j].origin.ToString(), i, m_Entities[i].origin.ToString(), dist.ToString() );
			// remember to mark as "to be deleted"
			tobedeleted.Append( j );
			// mark with negative entity so we can skip it
			m_Entities[j].classIdx = -m_Entities[j].classIdx;
		}

		if (merged > 0)
		{
			// if we have more entities to merge than what will fit into the model,
			// sort them based on distance and select the N nearest:
			if (merged > maxModelCount)
			{
				// so we can select the N nearest
				offsets.Sort( SortOffsetsByDistance );
				// truncate to only combine as much as we can:
				offsets.SetNum( maxModelCount );
			}
			// mark all entities that will be merged as "deleted", but skip the rest
			unsigned int n = (unsigned int)tobedeleted.Num();
			for (unsigned int d = 0; d < n; d++)
			{
				int todo = tobedeleted[d];
				// mark
				if (d < maxModelCount)
				{
					m_Entities[ todo ].classIdx = -1;
				}
				else
				{
					// restore classIdx
					m_Entities[todo].classIdx = -m_Entities[todo].classIdx;
				}
			}

			// build the combined model
			idVec3 corr = idVec3(0,0,0);

			// TODO: use all LOD stages here
			idList<const idRenderModel*> LODs;
			LODs.Clear();

			// gameLocal.Printf("LODE %s: Bounds before duplicate %s.\n", GetName(), tempModel->Bounds().ToString() );
			corr -= tempModel->Bounds()[0];
			LODs.Append( tempModel );

			// Get the player pos
			idPlayer *player = gameLocal.GetLocalPlayer();
			// if we have no player (how can this happen?), use our own origin as stand-in
			idVec3 playerPos = renderEntity.origin;
			if ( player ) {
				playerPos = player->GetPhysics()->GetOrigin();
			}
			const idMaterial* material = NULL;
			if (m_bDebugColors)
			{
				// select one at random
				idStr m = "textures/darkmod/debug/";
			   	m += lode_debug_materials[ gameLocal.random.RandomInt( LODE_DEBUG_MATERIAL_COUNT ) ];
				material = declManager->FindMaterial( m, false );
			}
			// use a megamodel to get the combined model, that we later can update, too:
			PseudoClass.megamodel = new CMegaModel( &LODs, &offsets, &playerPos, &m_Entities[i].origin, material );
			PseudoClass.hModel = PseudoClass.megamodel->GetRenderModel();

			// replace the old class with the new pseudo class which contains the merged model
			m_Entities[i].classIdx = m_Classes.Append( PseudoClass );
			// don't try to rotate the combined model after spawn
			m_Entities[i].angles = idAngles(0,0,0);
			//gameLocal.Printf("LODE %s: Correction: %s\n", GetName(), idVec3( PseudoClass.hModel->Bounds()[0]).ToString() );
			// get center of model
			idVec3 center = PseudoClass.hModel->Bounds().GetSize() / 2;
			center += PseudoClass.hModel->Bounds()[0];
			//gameLocal.Printf(" Correction: %s Size: %s Lower: %s Upper: %s\n", 
			//		center.ToString(), PseudoClass.hModel->Bounds().GetSize().ToString(), PseudoClass.hModel->Bounds()[0].ToString(), PseudoClass.hModel->Bounds()[1].ToString() );
			// this seems not right, tho:
			center.z = 0;
			m_Entities[i].origin -= center;
		}
	}

	if (mergedCount > 0)
	{
		gameLocal.Printf("LODE %s: Merged entity positions, now building combined final list.\n", GetName() );

		// delete all entities that got merged

		newEntities.Clear();
		// avoid low performance when we append one-by-one with occasional copy of the entire list
		// TODO: still O(N*N) time, tho
		if (m_Entities.Num() - mergedCount > 100)
		{
			newEntities.SetGranularity(64);
		}
		for (int i = 0; i < m_Entities.Num(); i++)
		{
			// we marked all entities that we combine with another entity with "-1" in the classIdx, so skip these
			if (m_Entities[i].classIdx != -1)
			{
				newEntities.Append( m_Entities[i] );
			}
		}
		m_Entities.Swap( newEntities );
		newEntities.Clear();				// should get freed at return, anyway

	}
	int end = (int) time (NULL);
	gameLocal.Printf("LODE %s: Combined %i entities into %i entities, took %i seconds.\n", GetName(), mergedCount + m_Entities.Num(), m_Entities.Num(), end - start );

	return;
}

/*
================
Lode::SpawnEntity - spawn the entity with the given index, returns true if it was spawned
================
*/

bool Lode::SpawnEntity( const int idx, const bool managed )
{
	struct lode_entity_t* ent = &m_Entities[idx];
	struct lode_class_t*  lclass = &(m_Classes[ ent->classIdx ]);

	// spawn the entity and note its number
	if (m_iDebug)
	{
		gameLocal.Printf( "LODE %s: Spawning entity #%i (%s, skin %s).\n", GetName(), idx, lclass->classname.c_str(), m_Skins[ ent->skinIdx ].c_str() );
	}

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

		// set previously defined (possible random) skin
	    args.Set("skin", m_Skins[ ent->skinIdx ] );
		// disable any other random_skin on the entity class or it would interfere
	    args.Set("random_skin", "");

		// set previously defined (possible random) color
	    args.SetVector("_color", ent->color );

		// TODO: spawn as hidden, then later unhide them via LOD code
		//args.Set("hide", "1");
		// disable LOD checks on entities (we take care of this)
		if (managed)
		{
			args.Set("dist_check_period", "0");
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
			//idStaticEntity *s_ent = static_cast<idStaticEntity*>( ent2 );
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

			// Is this an idStaticEntity? If yes, simply spawning will not recreate the model
			// so we need to do this manually.
			if ( lclass->pseudo || lclass->classname == FUNC_DUMMY )
			{
				// cache this
				renderEntity_t *r = ent2->GetRenderEntity();
				
				// free the old, empty model
				if (r->hModel)
				{
					gameLocal.Printf("LODE %s: Freeing old func_static model.\n", GetName() );
					ent2->FreeModelDef();
					r->hModel = NULL;
				}

				// setup the rendermodel and the clipmodel
				if (lclass->pseudo)
				{
					// each pseudoclass spawns only one entity
					r->hModel = lclass->hModel;
					r->bounds = lclass->hModel->Bounds();
					// TODO: remove old physics object first
					ent2->SetPhysics( lclass->physicsObj );
				}
				else
				{
					r->hModel = gameLocal.m_ModelGenerator->DuplicateModel( lclass->hModel, lclass->classname, false );
					if ( r->hModel )
					{
						// take the model bounds and transform them for the renderentity
						r->bounds.FromTransformedBounds( lclass->hModel->Bounds(), r->origin, r->axis );
						// gameLocal.Printf("LODE %s: Bounds of new model: %s.\n", GetName(), r->bounds.ToString() );
						// gameLocal.Printf("LODE %s: Bounds of old model: %s.\n", GetName(), lclass->hModel->Bounds().ToString() );
					}
					else
					{
						// should not happen
						r->bounds.Zero();
					}
					// clipmodel will be already correct:
					// ent2->GetPhysics()->SetClipModel( ... );
				}
				ent2->Present();

				//ent2->BecomeActive( TH_UPDATEVISUALS );

				// TODO: does not work yet:
				// update the clip model, too, otherwise the "dummy object"'s clipmodel lingers around
				idClipModel *clipmodel = new idClipModel( ent2->GetModelDefHandle() );
				//if (clipmodel && clipmodel->IsTraceModel() && ent2->GetPhysics())
				// is not a trace model, so will this still work?
				if (clipmodel && ent2->GetPhysics())
				{
					// need to set origin on the clipmodel first
					clipmodel->Translate( ent->origin );
					clipmodel->Rotate( ent->angles.ToRotation() );
					gameLocal.Printf("LODE %s: Setting new clipmodel, origin %s.\n", GetName(), clipmodel->GetOrigin().ToString() );
					ent2->GetPhysics()->SetClipModel( clipmodel, 1.0f );		// density 1.0f?
				}

				// short version of "UpdateVisuals()"
				// set to invalid number to force an update the next time the PVS areas are retrieved
				ent2->ClearPVSAreas();
			}
			else
			// might be a moveable?
			if ( ent2->IsType( idMoveable::Type ) ) {
				idMoveable *ment = static_cast<idMoveable*>( ent2 );
				ment->ActivatePhysics( this );
			}
			return true;
		}
	}
	return false;
}

/*
================
Lode::CullEntity - cull the entity with the given index, returns true if it was culled
================
*/
bool Lode::CullEntity( const int idx )
{
	struct lode_entity_t* ent = &m_Entities[idx];
	struct lode_class_t*  lclass = &(m_Classes[ ent->classIdx ]);

	if ( !ent->exists )
	{
		return false;
	}

	// cull (remove) the entity
	idEntity *ent2 = gameLocal.entities[ ent->entity ];
	if (ent2)
		{
		// Before we remove the entity, save it's position and angles
		// That makes it work for moveables or anything else that
		// might have changed position (teleported away etc)
		ent->origin = ent2->GetPhysics()->GetOrigin();
		ent->angles = ent2->GetPhysics()->GetAxis().ToAngles();

		// If the class has a model with shared data, manage this to avoid double frees
		if ( lclass->pseudo )
		{
			// is just a pointer to a rendermodel
			lclass->hModel = NULL;
			// mark as inactive (because the entity is no longer existing)
			// TODO: cl.megaModel.stopUpdates();
			// avoid freeing the composed model
			ent2->GetRenderEntity()->hModel = NULL;
			// TODO: either swap physics object or set to NULL to avoid freeing
			// 		 the object from the pseudo class
		}
		else
		{
			// do nothing, the class model is a duplicate and can be freed
			if ( lclass->hModel )
			{
				// TODO: do not all this as we don't use shared data yet
				// gameLocal.m_ModelGenerator->FreeSharedModelData ( ent2->GetRenderEntity()->hModel );
			}
		}
		// gameLocal.Printf( "LODE %s: Culling entity #%i (%0.2f > %0.2f).\n", GetName(), i, deltaSq, lclass->cullDist );

		m_iNumExisting --;
		m_iNumVisible --;
		ent->exists = false;
		ent->hidden = true;
		ent->entity = 0;

		// TODO: SafeRemve?
		ent2->PostEventMS( &EV_Remove, 0 );

		return true;
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

	// for some reason disabling thinking doesn't work, so return early in case we have no targets
	// also return until activated
	if (m_iNumEntities < 0 || m_bWaitForTrigger)
	{
		return;
	}

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
			m_iSeed_2 = m_iOrgSeed;

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

		// are we outside the player PVS?
		if ( m_iThinkCounter < 20 &&
			 ! gameLocal.pvs.InCurrentPVS( gameLocal.GetPlayerPVS(), m_iPVSAreas, m_iNumPVSAreas ) )
		{
			// if so, do nothing until think counter is high enough again
			//gameLocal.Printf( "LODE %s: Outside player PVS, think counter = %i, exiting.\n", GetName(), m_iThinkCounter );
			m_iThinkCounter ++;
			return;

			// TODO: cull all entities if m_iNumExisting > 0?
			// TODO: investigate if hiding brings us any performance, probably not, because
			//		 things outside the player PVS are not rendered anyway, but hiding/showing
			//		 them takes time.
		}

		m_iThinkCounter = 0;

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
			if (!ent->exists && (lclass->spawnDist == 0 || deltaSq < lclass->spawnDist))
			{
				if (SpawnEntity( i, true ))
				{
					spawned ++;
				}
			}	
			else
			{
				// cull entities that are outside "hide_distance + fade_out_distance + cullRange
				if (ent->exists && lclass->cullDist > 0 && deltaSq > lclass->cullDist)
				{
					// TODO: Limit number of entities to cull per frame
					if (CullEntity( i ))
					{
						culled ++;
					}

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
	m_bWaitForTrigger = false;	// enough waiting around, lets do some action
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

	for (int i = 0; i < m_Entities.Num(); i++)
	{
		CullEntity( i );
	}
}

