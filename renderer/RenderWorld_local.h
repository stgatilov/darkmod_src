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

#ifndef __RENDERWORLDLOCAL_H__
#define __RENDERWORLDLOCAL_H__

#include "containers/HashMap.h"

// assume any lightDef or entityDef index above this is an internal error
#define LUDICROUS_INDEX	65537		// (2 ** 16) + 1;


typedef struct portal_s {
	int						intoArea;		// area this portal leads to
	idWinding 				w;				// winding points have counter clockwise ordering seen this area
	idPlane					plane;			// view must be on the positive side of the plane to cross
	//struct portal_s *		next;			// next portal of the area
	struct doublePortal_s *	doublePortal;
} portal_t;


typedef struct doublePortal_s {
	struct portal_s			portals[2];
	int						blockingBits;	// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc, set by doors that shut them off

	float					lossPlayer;		// grayman #3042 - amount of Player sound loss (in dB)

	// A portal will be considered closed if it is past the
	// fog-out point in a fog volume.  We only support a single
	// fog volume over each portal.
	idRenderLightLocal *	fogLight;
	struct doublePortal_s *	nextFoggedPortal;
	int						portalViewCount;		// For r_showPortals. Keep track whether the player's view flows through 
											// individual portals, not just whole visleafs.  -- SteveL #4162

	doublePortal_s() { // zero fill
		blockingBits = 0;
		lossPlayer = 0;
		fogLight = 0;
		nextFoggedPortal = 0;
		portalViewCount = 0;
	}
} doublePortal_t;


typedef struct portalArea_s {
	int				areaNum;
	int				connectedAreaNum[NUM_PORTAL_ATTRIBUTES];	// if two areas have matching connectedAreaNum, they are
									// not separated by a portal with the apropriate PS_BLOCK_* blockingBits
	int				areaViewCount;		// set by R_FindViewLightsAndEntities. Marks whether anything in this area has been drawn this frame for r_showPortals
	idList<portal_t*> areaPortals;		// never changes after load
	areaReference_t	entityRefs;		// head/tail of doubly linked list, may change
	areaReference_t	lightRefs;		// head/tail of doubly linked list, may change
	idScreenRect	areaScreenRect;

	portalArea_s() { // zero fill
		areaNum = 0;
		memset(connectedAreaNum, 0, sizeof(connectedAreaNum));
		areaViewCount = 0;
		memset(&entityRefs, 0, sizeof(entityRefs));
		memset(&lightRefs, 0, sizeof(lightRefs));
		memset(&areaScreenRect, 0, sizeof(areaScreenRect));
	}
} portalArea_t;


static const int	CHILDREN_HAVE_MULTIPLE_AREAS = -2;
static const int	AREANUM_SOLID = -1;
typedef struct {
	idPlane			plane;
	int				children[2];		// negative numbers are (-1 - areaNumber), 0 = solid
	int				commonChildrenArea;	// if all children are either solid or a single area,
										// this is the area number, else CHILDREN_HAVE_MULTIPLE_AREAS
} areaNode_t;


//used when r_useInteractionTable = 2
struct InterTableHashFunction {
	ID_FORCE_INLINE int operator() (int idx) const {
		//note: f(x) = (A*x % P), where P = 2^31-1 is prime
		static const unsigned MOD = ((1U<<31) - 1);
		uint64_t prod = 0x04738F51ULL * (unsigned)idx;
		unsigned mod = (prod >> 31) + (prod & MOD);
		unsigned modm = mod - MOD;
		mod = mod < MOD ? mod : modm;
		return (int)mod;
	}
};

//this table stores all interactions ever generated and still actual
class idInteractionTable {
public:
	idInteractionTable();
	~idInteractionTable();
	void Init();
	void Shutdown();
	idInteraction *Find(idRenderLightLocal *ldef, idRenderEntityLocal *edef) const;
	bool Add(idInteraction *interaction);
	bool Remove(idInteraction *interaction);
	idStr Stats() const;

private:
	int useInteractionTable = -1;
	//r_useInteractionTable = 1: Single Matrix  (light x entity)
	idInteraction** SM_matrix;
	//r_useInteractionTable = 2: Single Hash Table
	idHashMap<int, idInteraction*> SHT_table;
};

class idRenderWorldLocal : public idRenderWorld {
public:
							idRenderWorldLocal();
	virtual					~idRenderWorldLocal();

	virtual	qhandle_t		AddEntityDef( const renderEntity_t *re );
	virtual	void			UpdateEntityDef( qhandle_t entityHandle, const renderEntity_t *re );
	virtual	void			FreeEntityDef( qhandle_t entityHandle );
	virtual const renderEntity_t *GetRenderEntity( qhandle_t entityHandle ) const;

	virtual	qhandle_t		AddLightDef( const renderLight_t *rlight );
	virtual	void			UpdateLightDef( qhandle_t lightHandle, const renderLight_t *rlight ); 
	virtual	void			FreeLightDef( qhandle_t lightHandle );
	virtual const renderLight_t *GetRenderLight( qhandle_t lightHandle ) const;

	virtual bool			CheckAreaForPortalSky( int areaNum );

	virtual	void			GenerateAllInteractions();
	virtual void			RegenerateWorld();

	virtual void			ProjectDecalOntoWorld( const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime );
	virtual void			ProjectDecal( qhandle_t entityHandle, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime );
	virtual void			ProjectOverlay( qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial *material );
	virtual void			RemoveDecals( qhandle_t entityHandle );

	virtual void			SetRenderView( const renderView_t *renderView );
	virtual	void			RenderScene( const renderView_t &renderView );

	virtual	int				NumAreas( void ) const;
	virtual int				PointInArea( const idVec3 &point ) const;
	virtual int				BoundsInAreas( const idBounds &bounds, int *areas, int maxAreas ) const;
	virtual	int				NumPortalsInArea( int areaNum );
	// grayman #3042 - set portal sound loss (in dB)
	virtual void			SetPortalPlayerLoss( qhandle_t portal, float loss );

	virtual exitPortal_t	GetPortal( int areaNum, int portalNum );

#if 0
	virtual	guiPoint_t		GuiTrace( qhandle_t entityHandle, const idVec3 start, const idVec3 end ) const;
#endif
	virtual bool			ModelTrace( modelTrace_t &trace, qhandle_t entityHandle, const idVec3 &start, const idVec3 &end, const float radius ) const;
	virtual bool			Trace( modelTrace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, bool skipDynamic = true, bool skipPlayer = false ) const;
	virtual bool			FastWorldTrace( modelTrace_t &trace, const idVec3 &start, const idVec3 &end ) const;
	virtual bool			MaterialTrace( const idVec3 &p, const idMaterial *mat, idStr &matName ) const;
	virtual bool			TraceAll( modelTrace_t &trace, const idVec3 &start, const idVec3 &end, bool fastWorld = false, float radius = 0.0f, TraceFilterFunc filterCallback = nullptr, void *context = nullptr ) const;

	virtual void			DebugClearLines( int time );
	virtual void			DebugLine( const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugArrow( const idVec4 &color, const idVec3 &start, const idVec3 &end, int size, const int lifetime = 0 );
	virtual void			DebugWinding( const idVec4 &color, const idWinding &w, const idVec3 &origin, const idMat3 &axis, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugCircle( const idVec4 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugSphere( const idVec4 &color, const idSphere &sphere, const int lifetime = 0, bool depthTest = false );
	virtual void			DebugBounds( const idVec4 &color, const idBounds &bounds, const idVec3 &org = vec3_origin, const int lifetime = 0 );
	virtual void			DebugBox( const idVec4 &color, const idBox &box, const int lifetime = 0 );
	virtual void			DebugFrustum( const idVec4 &color, const idFrustum &frustum, const bool showFromOrigin = false, const int lifetime = 0 );
	virtual void			DebugCone( const idVec4 &color, const idVec3 &apex, const idVec3 &dir, float radius1, float radius2, const int lifetime = 0 );
	virtual void			DebugScreenRect( const idVec4 &color, const idScreenRect &rect, const viewDef_t *viewDef, const int lifetime = 0 );
	virtual void			DebugAxis( const idVec3 &origin, const idMat3 &axis );

	virtual void			DebugClearPolygons( int time );
	virtual void			DebugPolygon( const idVec4 &color, const idWinding &winding, const int lifeTime = 0, const bool depthTest = false );

	virtual void			DebugText( const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align = 1, const int lifetime = 0, bool depthTest = false );

	//-----------------------

	idStr					mapName;				// ie: maps/tim_dm2.proc, written to demoFile
	ID_TIME_T				mapTimeStamp;			// for fast reloads of the same level

	areaNode_t *			areaNodes;
	int						numAreaNodes;

	idList<portalArea_t> portalAreas;
	//int						numPortalAreas;
	int						connectedAreaNum;		// incremented every time a door portal state changes

	idList<doublePortal_t>	doublePortals;
	//int						numInterAreaPortals;

	idList<idRenderModel *>	localModels;

	idList<idRenderEntityLocal*>	entityDefs;
	idList<idRenderLightLocal*>		lightDefs;

	idBlockAlloc<areaReference_t, 1024> areaReferenceAllocator;
	idBlockAlloc<idInteraction, 256>	interactionAllocator;

	// all light / entity interactions are referenced here for fast lookup without
	// having to crawl the doubly linked lists.  EnntityDefs are sequential for better
	// cache access, because the table is accessed by light in idRenderWorldLocal::CreateLightDefInteractions()
	// Growing this table is time consuming, so we add a pad value to the number
	// of entityDefs and lightDefs
	idInteractionTable		interactionTable;


	bool					generateAllInteractionsCalled;

	//-----------------------
	// RenderWorld_load.cpp

	idRenderModel *			ParseModel( idLexer *src );
	idRenderModel *			ParseShadowModel( idLexer *src );
	void					SetupAreaRefs();
	void					ParseInterAreaPortals( idLexer *src );
	void					ParseNodes( idLexer *src );
	int						CommonChildrenArea_r( areaNode_t *node );
	void					FreeWorld();
	void					ClearWorld();
	void					FreeDefs();
	void					TouchWorldModels( void );
	void					AddWorldModelEntities();
	void					ClearPortalStates();
	virtual	bool			InitFromMap( const char *mapName );

	//--------------------------
	// RenderWorld_portals.cpp

	idScreenRect			ScreenRectFromWinding( const idWinding *w, viewEntity_t *space );
	bool					PortalIsFoggedOut( const portal_t *p );
	void					FloodViewThroughArea_r( const idVec3 origin, int areaNum, const struct portalStack_s *ps );
	void					FlowViewThroughPortals( const idVec3 origin, int numPlanes, const idPlane *planes );
	void					FloodLightThroughArea_r( idRenderLightLocal *light, int areaNum, const struct portalStack_s *ps );
	void					FlowLightThroughPortals( idRenderLightLocal *light );
	bool					CullEntityByPortals( const idRenderEntityLocal *entity, const struct portalStack_s *ps );
	void					AddAreaEntityRefs( int areaNum, const struct portalStack_s *ps );
	bool					CullLightByPortals( const idRenderLightLocal *light, const struct portalStack_s *ps );
	void					AddAreaLightRefs( int areaNum, const struct portalStack_s *ps );
	void					AddAreaRefs( int areaNum, const struct portalStack_s *ps );
	void					BuildConnectedAreas_r( int areaNum );
	void					BuildConnectedAreas( void );
	void					FindViewLightsAndEntities( void );

	struct FloodShadowFrustumContext;
	bool					FloodShadowFrustumThroughArea_r( FloodShadowFrustumContext &context, const idBounds &bounds ) const;
	void					FlowShadowFrustumThroughPortals( idScreenRect &scissorRect, const idFrustum &frustum, const int *startAreas, int startAreasNum ) const;

	int						NumPortals( void ) const;
	qhandle_t				FindPortal( const idBounds &b ) const;
	static bool				DoesVisportalContactBox( const idWinding &visportalWinding, const idBounds &box );	//stgatilov #5354
	void					SetPortalState( qhandle_t portal, int blockingBits );
	int						GetPortalState( qhandle_t portal );
	idPlane					GetPortalPlane( qhandle_t portal );	//stgatilov #5462

	bool					AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection );
	void					FloodConnectedAreas( portalArea_t *area, int portalAttributeIndex );
	const idScreenRect &	GetAreaScreenRect( int areaNum ) const { return portalAreas[areaNum].areaScreenRect; }
	void					ShowPortals();

	//--------------------------
	// RenderWorld_demo.cpp

	void					StartWritingDemo( idDemoFile *demo );
	void					StopWritingDemo();
	bool					ProcessDemoCommand( idDemoFile *readDemo, renderView_t *demoRenderView, int *demoTimeOffset );

	void					WriteLoadMap();
	void					WriteRenderView( const renderView_t &renderView );
	void					WriteVisibleDefs( const viewDef_t *viewDef );
	void					WriteFreeLight( qhandle_t handle );
	void					WriteFreeEntity( qhandle_t handle );
	void					WriteRenderLight( qhandle_t handle, const renderLight_t *light );
	void					WriteRenderEntity( qhandle_t handle, const renderEntity_t *ent );
	void					ReadRenderEntity();
	void					ReadRenderLight();
	

	//--------------------------
	// RenderWorld.cpp

	void					AddEntityRefToArea( idRenderEntityLocal *def, portalArea_t *area );
	void					AddLightRefToArea( idRenderLightLocal *light, portalArea_t *area );

	void					RecurseProcBSP_r( modelTrace_t *results, int *areas, int *numAreas, int maxAreas, int parentNodeNum, int nodeNum, float p1f, float p2f, const idVec3 &p1, const idVec3 &p2 ) const;

	void					BoundsInAreas_r( int nodeNum, const idBounds &bounds, int *areas, int *numAreas, int maxAreas ) const;

	float					DrawTextLength( const char *text, float scale, int len = 0 );

	void					PutAllInteractionsIntoTable( bool resetTable );
	void					FreeInteractions();

	void					PushVolumeIntoTree_r( idRenderEntityLocal *def, idRenderLightLocal *light, const idSphere *sphere, int numPoints, const idVec3 (*points), int nodeNum );

	void					PushVolumeIntoTree( idRenderEntityLocal *def, idRenderLightLocal *light, int numPoints, const idVec3 (*points) );

	void					PushFrustumIntoTree_r(idRenderEntityLocal* def, idRenderLightLocal* light, const frustumCorners_t& corners, int nodeNum);
	void					PushFrustumIntoTree(idRenderEntityLocal* def, idRenderLightLocal* light, const idRenderMatrix& frustumTransform, const idBounds& frustumBounds);
	
	//-------------------------------
	// tr_light.c
	void					CreateLightDefInteractions( idRenderLightLocal *ldef );
};


//stgatilov: some informative labels suitable for tracing
//ideally, it should match natvis definitions...

ID_FORCE_INLINE const char *GetTraceLabel(const renderEntity_t &rEnt) {
	assert( g_tracingEnabled );
	if ( rEnt.entityNum != 0 ) {
		return gameLocal.entities[rEnt.entityNum]->name.c_str();
	} else if ( rEnt.hModel ) {
		return rEnt.hModel->Name();
	} else {
		return "[unknown]";
	}
}

ID_FORCE_INLINE const char *GetTraceLabel(const renderLight_t &rLight) {
	assert( g_tracingEnabled );
	if ( rLight.entityNum != 0 ) {
		return gameLocal.entities[rLight.entityNum]->name.c_str();
	} else {
		return "[unknown]";
	}
}

#endif /* !__RENDERWORLDLOCAL_H__ */
