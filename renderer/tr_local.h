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

#ifndef __TR_LOCAL_H__
#define __TR_LOCAL_H__

#include <atomic>

#include "Image.h"
#include "MegaTexture.h"

class idRenderWorldLocal;

const int FALLOFF_TEXTURE_SIZE =	64;

const float	DEFAULT_FOG_DISTANCE = 500.0f;

const int FOG_ENTER_SIZE = 64;
const float FOG_ENTER = ( FOG_ENTER_SIZE + 1.0f ) / ( FOG_ENTER_SIZE * 2 );
// picky to get the bilerp correct at terminator


// idScreenRect gets carried around with each drawSurf, so it makes sense
// to keep it compact, instead of just using the idBounds class
class idScreenRect {
public:
	short		x1, y1, x2, y2;							// inclusive pixel bounds inside viewport
	float       zmin, zmax;								// for depth bounds test

	void		Clear();								// clear to backwards values
	void		AddPoint( float x, float y );			// adds a point
	void		Expand();								// expand by one pixel each way to fix roundoffs
	void		Intersect( const idScreenRect &rect );
	void		Union( const idScreenRect &rect );
	bool		Equals( const idScreenRect& rect ) const {
		return ( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
	}
	bool		Overlaps( const idScreenRect& rect ) const {
		// The rectangles don't overlap if
		// one rectangle's minimum in some dimension 
		// is greater than the other's maximum in
		// that dimension.

		bool noOverlap = rect.x1 > x2 ||
			x1 > rect.x2 ||
			rect.y1 > y2 ||
			y1 > rect.y2;

		return !noOverlap;
	}
	bool		IsEmpty() const;
	int			GetArea() const { //anon
		return GetWidth() * GetHeight();
	}
	// duzenko: got tired of all the inline subtractions
	int GetWidth() const {
		return x2 - x1 + 1;
	}
	int GetHeight() const {
		return y2 - y1 + 1;
	}
};

idScreenRect R_ScreenRectFromViewFrustumBounds( const idBounds &bounds );
void R_ShowColoredScreenRect( const idScreenRect &rect, int colorIndex );

typedef enum {
	DC_BAD,
	DC_RENDERVIEW,
	DC_UPDATE_ENTITYDEF,
	DC_DELETE_ENTITYDEF,
	DC_UPDATE_LIGHTDEF,
	DC_DELETE_LIGHTDEF,
	DC_LOADMAP,
	DC_CROP_RENDER,
	DC_UNCROP_RENDER,
	DC_CAPTURE_RENDER,
	DC_END_FRAME,
	DC_DEFINE_MODEL,
	DC_SET_PORTAL_STATE,
	DC_UPDATE_SOUNDOCCLUSION,
	DC_GUI_MODEL,
	DC_SET_PLAYER_PORTAL_LOSS	// grayman #3042
} demoCommand_t;

typedef enum {
	LS_NONE,
	LS_STENCIL,
	LS_MAPS
} lightShadows_t;

typedef enum {
	XR_IGNORE,
	XR_ONLY,
	XR_SUBSTITUTE
} xrayEntityMask_t;

/*
==============================================================================

SURFACES

==============================================================================
*/

#include "ModelDecal.h"
#include "ModelOverlay.h"
#include "Interaction.h"


// drawSurf_t structures command the back end to render surfaces
// a given srfTriangles_t may be used with multiple viewEntity_t,
// as when viewed in a subview or multiple viewport render, or
// with multiple shaders when skinned, or, possibly with multiple
// lights, although currently each lighting interaction creates
// unique srfTriangles_t
// drawSurf_t are always allocated and freed every frame, they are never cached
// drawSurf_t are initialized manually, so check all places they're created if adding a member.

static const int DSF_VIEW_INSIDE_SHADOW	= 1;
static const int DSF_SOFT_PARTICLE = 2; // #3878
static const int DSF_SHADOW_MAP_IGNORE = 4; // #4641
static const int DSF_SHADOW_MAP_ONLY = 8; // #4641

struct viewLight_s;

typedef struct drawSurf_s {
	const srfTriangles_t	*frontendGeo;			// do not use in the backend; may be modified by the frontend
	int						numIndexes;				// these four are frame-safe copies for backend use
	vertCacheHandle_t		indexCache;				// int				
	vertCacheHandle_t		ambientCache;			// idDrawVert
	vertCacheHandle_t		shadowCache;			// shadowCache_t

	const struct viewEntity_s *space;
	const idMaterial		*material;			// may be NULL for shadow volumes
	float					sort;				// material->sort, modified by gui / entity sort offsets
	const float				*shaderRegisters;	// evaluated and adjusted for referenceShaders
	/*const*/ struct drawSurf_s	*nextOnLight;	// viewLight chains

	viewLight_s				**onLights;			// light/entity bounds intersections, terminated by null - multi light shader

	idScreenRect			scissorRect;		// for scissor clipping, local inside renderView viewport
	int						dsFlags;			// DSF_VIEW_INSIDE_SHADOW, etc
	//vertCacheHandle_t		dynamicTexCoords;	// float * in vertex cache memory // duzenko: disabled in 2.08, to be removed in 2.09
	// specular directions for non vertex program cards, skybox texcoords, etc
	float					particle_radius;	// The radius of individual quads for soft particles #3878

	void CopyGeo( const srfTriangles_t *tri ) {
		frontendGeo = tri;
		numIndexes = tri->numIndexes;
		indexCache = tri->indexCache;
		ambientCache = tri->ambientCache;
		shadowCache = tri->shadowCache;
	}
} drawSurf_t;


typedef struct {
	int		numPlanes;		// this is always 6 for now
	idPlane	planes[6];
	// positive sides facing inward
	// plane 5 is always the plane the projection is going to, the
	// other planes are just clip planes
	// all planes are in global coordinates

	bool	makeClippedPlanes;
	// a projected light with a single frustum needs to make sil planes
	// from triangles that clip against side planes, but a point light
	// that has adjacent frustums doesn't need to
} shadowFrustum_t;


// areas have references to hold all the lights and entities in them
typedef struct areaReference_s {
	struct areaReference_s *areaNext;				// chain in the area
	struct areaReference_s *areaPrev;
	struct areaReference_s *ownerNext;				// chain on either the entityDef or lightDef
	idRenderEntityLocal 	*entity;					// only one of entity / light will be non-NULL
	idRenderLightLocal 	*light;					// only one of entity / light will be non-NULL
	struct portalArea_s		*area;					// so owners can find all the areas they are in
} areaReference_t;


// idRenderLight should become the new public interface replacing the qhandle_t to light defs in the idRenderWorld interface
class idRenderLight {
public:
	virtual					~idRenderLight() {}

	virtual void			FreeRenderLight() = 0;
	virtual void			UpdateRenderLight( const renderLight_t *re, bool forceUpdate = false ) = 0;
	virtual void			GetRenderLight( renderLight_t *re ) = 0;
	virtual void			ForceUpdate() = 0;
	virtual int				GetIndex() = 0;
};


// idRenderEntity should become the new public interface replacing the qhandle_t to entity defs in the idRenderWorld interface
class idRenderEntity {
public:
	virtual					~idRenderEntity() {}

	virtual void			FreeRenderEntity() = 0;
	virtual void			UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate = false ) = 0;
	virtual void			GetRenderEntity( renderEntity_t *re ) = 0;
	virtual void			ForceUpdate() = 0;
	virtual int				GetIndex() = 0;

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay( const idPlane localTextureAxis[2], const idMaterial *material ) = 0;
	virtual void			RemoveDecals() = 0;
};


class idRenderLightLocal : public idRenderLight {
public:
	idRenderLightLocal();

	virtual void			FreeRenderLight();
	virtual void			UpdateRenderLight( const renderLight_t *re, bool forceUpdate = false );
	virtual void			GetRenderLight( renderLight_t *re );
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	renderLight_t			parms;					// specification

	bool					lightHasMoved;			// the light has changed its position since it was
	// first added, so the prelight model is not valid

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

	idRenderWorldLocal 		*world;
	int						index;					// in world lightdefs

	int						areaNum;				// if not -1, we may be able to cull all the light's
	// interactions if !viewDef->connectedAreas[areaNum]

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
	// and should go in the dynamic frame memory, or kept
	// in the cached memory
	bool					archived;				// for demo writing


	// derived information
	idPlane					lightProject[4];
	idRenderMatrix			baseLightProject;		// global xyz1 to projected light strq
	idRenderMatrix			inverseBaseLightProject;// transforms the zero-to-one cube to exactly cover the light in world space

	const idMaterial 		*lightShader;			// guaranteed to be valid, even if parms.shader isn't
	idImage 				*falloffImage;

	idVec3					globalLightOrigin;		// accounting for lightCenter and parallel
	idBounds				globalLightBounds;


	idPlane					frustum[6];				// in global space, positive side facing out, last two are front/back
	idWinding				frustumWindings[6];		// used for culling
	srfTriangles_t 			*frustumTris;			// triangulated frustumWindings[]

	int						numShadowFrustums;		// one for projected lights, usually six for point lights
	shadowFrustum_t			shadowFrustums[6];

	int						viewCount;				// if == tr.viewCount, the light is on the viewDef->viewLights list
	struct viewLight_s 		*viewLight;

	areaReference_t 		*references;				// each area the light is present in will have a lightRef
	idInteraction 			*firstInteraction;		// doubly linked list
	idInteraction 			*lastInteraction;

	struct doublePortal_s 	*foggedPortals;
};


class idRenderEntityLocal : public idRenderEntity {
public:
	idRenderEntityLocal();

	virtual void			FreeRenderEntity();
	virtual void			UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate = false );
	virtual void			GetRenderEntity( renderEntity_t *re );
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay( const idPlane localTextureAxis[2], const idMaterial *material );
	virtual void			RemoveDecals();

	renderEntity_t			parms;

	float					modelMatrix[16];			// this is just a rearrangement of parms.axis and parms.origin
	idRenderMatrix			modelRenderMatrix;
	idRenderMatrix			inverseBaseModelProject;	// transforms the unit cube to exactly cover the model in world space

	idRenderWorldLocal 	*world;
	int						index;						// in world entityDefs

	int						lastModifiedFrameNum;		// to determine if it is constantly changing,
	// and should go in the dynamic frame memory, or kept
	// in the cached memory
	bool					archived;					// for demo writing

	idRenderModel 			*dynamicModel;				// if parms.model->IsDynamicModel(), this is the generated data
	int						dynamicModelFrameCount;		// continuously animating dynamic models will recreate
	// dynamicModel if this doesn't == tr.viewCount
	idRenderModel 			*cachedDynamicModel;

	idBounds				referenceBounds;			// the local bounds used to place entityRefs, either from parms or a model
	// axis aligned bounding box in world space, derived from refernceBounds and
	// modelMatrix in R_CreateEntityRefs()
	idBounds				globalReferenceBounds;

	// a viewEntity_t is created whenever a idRenderEntityLocal is considered for inclusion
	// in a given view, even if it turns out to not be visible
	int						viewCount;					// if tr.viewCount == viewCount, viewEntity is valid,
	// but the entity may still be off screen
	struct viewEntity_s 	*viewEntity;				// in frame temporary memory

	int						visibleCount;
	// if tr.viewCount == visibleCount, at least one ambient
	// surface has actually been added by R_AddAmbientDrawsurfs
	// note that an entity could still be in the view frustum and not be visible due
	// to portal passing

	idRenderModelDecal		*decals;					// chain of decals that have been projected on this model
	idRenderModelOverlay 	*overlay;					// blood overlays on animated models

	areaReference_t 		*entityRefs;				// chain of all references
	idInteraction 			*firstInteraction;			// doubly linked list
	idInteraction 			*lastInteraction;

	bool					needsPortalSky;
	int						centerArea;

	idSysMutex				mutex;						// needed to synchronize R_EntityDefDynamicModel over multiple threads
};

// viewLights are allocated on the frame temporary stack memory
// a viewLight contains everything that the back end needs out of an idRenderLightLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// a viewLight may exist even without any surfaces, and may be relevent for fogging,
// but should never exist if its volume does not intersect the view frustum
typedef struct viewLight_s {
	struct viewLight_s 		*next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	idRenderLightLocal 		*lightDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals
	idScreenRect			scissorRect;

	// if the view isn't inside the light, we can use the non-reversed
	// shadow drawing, avoiding the draws of the front and rear caps
	bool					viewInsideLight;

	// true if globalLightOrigin is inside the view frustum, even if it may
	// be obscured by geometry.  This allows us to skip shadows from non-visible objects
	bool					viewSeesGlobalLightOrigin;

	// if !viewInsideLight, the corresponding bit for each of the shadowFrustum
	// projection planes that the view is on the negative side of will be set,
	// allowing us to skip drawing the projected caps of shadows if we can't see the face
	int						viewSeesShadowPlaneBits;
	int						shadowMapIndex;				// zero - shadow maps not used, positive - shadow page index +1

	bool					noFogBoundary;				// Stops fogs drawing and fogging their bounding boxes -- SteveL #3664
	bool					singleLightOnly;			// multi-light shader can't handle it
	bool					pointLight;
	bool					noShadows;
	bool					noSpecular;
	bool					volumetricNoshadows;		// stgatilov #5816: ignore shadows in volumetric light

	idVec3					globalLightOrigin;			// global light origin used by backend
	idPlane					lightProject[4];			// light project used by backend
	idPlane					fogPlane;					// fog plane for backend fog volume rendering
	srfTriangles_t 			*frustumTris;				// light frustum for backend fog volume rendering
	lightShadows_t 			shadows;					// per-light shadowing mode, not always == r_shadows
	const idMaterial 		*lightShader;				// light shader used by backend
	const float				*shaderRegisters;			// shader registers used by backend
	idImage 				*falloffImage;				// falloff image used by backend
	float					volumetricDust;				// stgatilov #5816: strength of volumetric light

	/*const */struct drawSurf_s	*globalShadows;				// shadow everything
	/*const */struct drawSurf_s	*localInteractions;			// don't get local shadows
	/*const */struct drawSurf_s	*localShadows;				// don't shadow local Surfaces
	/*const */struct drawSurf_s	*globalInteractions;		// get shadows from everything
	/*const */struct drawSurf_s	*translucentInteractions;	// get shadows from everything

	uint32_t				lightMask;
} viewLight_t;

struct preparedSurf_t {
	drawSurf_t		*surf;
	idUserInterface *gui;
	preparedSurf_t	*next;
};
// a viewEntity is created whenever a idRenderEntityLocal is considered for inclusion
// in the current view, but it may still turn out to be culled.
// viewEntity are allocated on the frame temporary stack memory
// a viewEntity contains everything that the back end needs out of a idRenderEntityLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// A single entityDef can generate multiple viewEntity_t in a single frame, as when seen in a mirror
typedef struct viewEntity_s {
	struct viewEntity_s	*next;

	// back end should NOT reference the entityDef, because it can change when running SMP
	idRenderEntityLocal	*entityDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the viewEntity_t was never actually
	// seen through any portals, but was created for shadow casting.
	// a viewEntity can have a non-empty scissorRect, meaning that an area
	// that it is in is visible, and still not be visible.
	idScreenRect		scissorRect;

	bool				weaponDepthHack;
	float				modelDepthHack;		  // Used by particles only. Causes the particle to be drawn in front of intersecting
	// geometry by up to modelDepthhack units, to remove some ugly intersections.

	float				modelMatrix[16];		// local coords to global coords
	float				modelViewMatrix[16];	// local coords to eye coords
	idRenderMatrix		mvp;

	preparedSurf_t		*preparedSurfs;

	int					drawCalls;				// perf tool
} viewEntity_t;

// viewDefs are allocated on the frame temporary stack memory
typedef struct viewDef_s {
	// specified in the call to DrawScene()
	renderView_t		renderView;

	float				projectionMatrix[16];
	idRenderMatrix		projectionRenderMatrix;	// tech5 version of projectionMatrix

	viewEntity_t		worldSpace;

	idRenderWorldLocal *renderWorld;

	float				floatTime;

	idVec3				initialViewAreaOrigin;
	// Used to find the portalArea that view flooding will take place from.
	// for a normal view, the initialViewOrigin will be renderView.viewOrg,
	// but a mirror may put the projection origin outside
	// of any valid area, or in an unconnected area of the map, so the view
	// area must be based on a point just off the surface of the mirror / subview.
	// It may be possible to get a failed portal pass if the plane of the
	// mirror intersects a portal, and the initialViewAreaOrigin is on
	// a different side than the renderView.viewOrg is.

	bool				isSubview;				// true if this view is not the main view
	bool				isMirror;				// the portal is a mirror, invert the face culling
	xrayEntityMask_t	xrayEntityMask;
	bool				hasXraySubview;

	bool				isEditor;

	idPlane				*clipPlane;			// in world space, the positive side, mirrors will often use a single clip plane
	// of the plane is the visible side
	idScreenRect		viewport;				// in real pixels and proper Y flip

	idScreenRect		scissor;
	// for scissor clipping, local inside renderView viewport
	// subviews may only be rendering part of the main view
	// these are real physical pixel values, possibly scaled and offset from the
	// renderView x/y/width/height

	struct viewDef_s 	*superView;				// never go into an infinite subview loop
	struct drawSurf_s 	*subviewSurface;

	// drawSurfs are the visible surfaces of the viewEntities, sorted
	// by the material sort parameter
	drawSurf_t 			**drawSurfs;			// we don't use an idList for this, because
	int					numDrawSurfs;			// it is allocated in frame temporary memory
	int					maxDrawSurfs;			// may be resized
	int					numOffscreenSurfs;		// light occluders only, used by multi light shader

	struct viewLight_s	*viewLights;			// chain of all viewLights effecting view
	struct viewEntity_s	*viewEntitys;			// chain of all viewEntities effecting view, including off screen ones casting shadows
	// we use viewEntities as a check to see if a given view consists solely
	// of 2D rendering, which we can optimize in certain ways.  A 2D view will
	// not have any viewEntities

	idPlane				frustum[5];				// positive sides face outward, [4] is the front clip plane
	idFrustum			viewFrustum;

	idVec3				lightSample;

	int					areaNum;				// -1 = not in a valid area

	bool 				*connectedAreas;
	// An array in frame temporary memory that lists if an area can be reached without
	// crossing a closed door.  This is used to avoid drawing interactions
	// when the light is behind a closed door.

	// tools
	char				*portalStates;

	bool				IsLightGem() const {
		return renderView.viewID < 0;
	}
} viewDef_t;


// complex light / surface interactions are broken up into multiple passes of a
// simple interaction shader
typedef struct {
	const drawSurf_t 	*surf;

	idImage 			*lightImage;
	idImage 			*lightFalloffImage;
	idImage 			*bumpImage;
	idImage 			*diffuseImage;
	idImage 			*specularImage;

	idVec4				diffuseColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	idVec4				specularColor;	// may have a light color baked into it, will be < tr.backEndRendererMaxLight
	idVec4				ambientRimColor;
	stageVertexColor_t	vertexColor;	// applies to both diffuse and specular

	int					ambientLight;	// use tr.ambientNormalMap instead of normalization cube map
										// (not a bool just to avoid an uninitialized memory check of the pad region by valgrind)
	int					cubicLight;		// nbohr1more #3881: dedicated cubemap light // probably not needed

	idVec4				localLightOrigin;
	idVec4				localViewOrigin;
	idVec4				lightProjection[4];		// transforms object coords into light-volume coords
	idVec4				lightTextureMatrix[2];	// transforms light-volume coords into lightImage texcoords
	idVec4				bumpMatrix[2];
	idVec4				diffuseMatrix[2];
	idVec4				specularMatrix[2];
	idVec4				worldUpLocal; // rebb: world up vector in local space, required for effects like hemisphere lighting. alternatively always pass model matrix ?
} drawInteraction_t;


/*
=============================================================

RENDERER BACK END COMMAND QUEUE

TR_CMDS

=============================================================
*/

typedef enum {
	RC_NOP,
	RC_DRAW_VIEW,
	RC_DRAW_LIGHTGEM,
	RC_SET_BUFFER,
	RC_COPY_RENDER,
	RC_BLOOM,
	RC_SWAP_BUFFERS		// can't just assume swap at end of list because
	// of forced list submission before syncs
} renderCommand_t;

struct baseCommand_t {
	renderCommand_t commandId;
};

struct emptyCommand_t : baseCommand_t {
	baseCommand_t* next;
};

struct bloomCommand_t : emptyCommand_t {
};

struct setBufferCommand_t : emptyCommand_t {
	GLenum	buffer;
	int		frameCount;
};

struct drawSurfsCommand_t : emptyCommand_t {
	viewDef_t	*viewDef;
};

struct drawLightgemCommand_t : drawSurfsCommand_t {
	byte *dataBuffer;
};

struct copyRenderCommand_t : emptyCommand_t {
	int		x, y, imageWidth, imageHeight;
	idImage	*image;
	int		cubeFace;			// when copying to a cubeMap
	unsigned char	*buffer;	// to memory instead of to texture
	bool	usePBO;				// lightgem optimization
};

//=======================================================================

// this is the inital allocation for max number of drawsurfs
// in a given view, but it will automatically grow if needed
const int	INITIAL_DRAWSURFS =			0x4000;

// a request for frame memory will never fail
// (until malloc fails), but it may force the
// allocation of a new memory block that will
// be discontinuous with the existing memory
typedef struct frameMemoryBlock_s {
	struct frameMemoryBlock_s *next;
	int		size;
	int		used;
	int		poop;		// so that base is 16 byte aligned
	byte	base[4];	// dynamically allocated as [size]
} frameMemoryBlock_t;

// all of the information needed by the back end must be
// contained in a frameData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
typedef struct {
	std::atomic<int>	frameMemoryAllocated;
	std::atomic<int>	frameMemoryUsed;
	byte				*frameMemory;

	srfTriangles_t 		*firstDeferredFreeTriSurf;
	srfTriangles_t 		*lastDeferredFreeTriSurf;
	idSysMutex			deferredFreeMutex;

	int					memoryHighwater;	// max used on any frame

	// the currently building command list
	// commands can be inserted at the front if needed, as for required
	// dynamically generated textures
	emptyCommand_t		*cmdHead, *cmdTail;		// may be of other command type based on commandId
} frameData_t;

extern	frameData_t		*frameData;
extern	frameData_t		*backendFrameData;

//=======================================================================

void R_LockSurfaceScene( viewDef_t &parms );
void R_ClearCommandChain( frameData_t *frameData );
void R_AddDrawViewCmd( viewDef_t &parms );

void R_ReloadGuis_f( const idCmdArgs &args );
void R_ListGuis_f( const idCmdArgs &args );

void *R_GetCommandBuffer( int bytes );

// this allows a global override of all materials
bool R_GlobalShaderOverride( const idMaterial **shader );

// this does various checks before calling the idDeclSkin
const idMaterial *R_RemapShaderBySkin( const idMaterial *shader, const idDeclSkin *customSkin, const idMaterial *customShader );


//====================================================


/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_in, c_sphere_cull_clip, c_sphere_cull_out;
	int		c_box_cull_in, c_box_cull_out;
	int		c_createInteractions;	// number of calls to idInteraction::CreateInteraction
	int		c_createLightTris;
	int		c_createShadowVolumes;
	int		c_generateMd5;
	int		c_entityDefCallbacks;
	int		c_alloc, c_free;		// counts for R_StaticAllc/R_StaticFree
	int		c_visibleViewEntities;
	int		c_shadowViewEntities;
	int		c_viewLights;
	int		c_numViews;				// number of total views rendered
	int		c_deformedSurfaces;		// idMD5Mesh::GenerateSurface
	int		c_deformedVerts;		// idMD5Mesh::GenerateSurface
	int		c_deformedIndexes;		// idMD5Mesh::GenerateSurface
	int		c_tangentIndexes;		// R_DeriveTangents()
	int		c_entityUpdates, c_lightUpdates, c_entityReferences, c_lightReferences;
	int		c_guiSurfs, c_noshadowSurfs;
	int		frontEndMsec;			// sum of time in all RE_RenderScene's in a frame
	int		frontEndMsecLast;		// time in last RE_RenderScene
} performanceCounters_t;


typedef struct {
	int		current2DMap;
	int		currentCubeMap;
} tmu_t;

const int MAX_MULTITEXTURE_UNITS = 32;
typedef struct {
	tmu_t		tmu[MAX_MULTITEXTURE_UNITS];
	int			currenttmu;

	int			faceCulling;
	int			glStateBits;
	bool		forceGlState;		// the next GL_State will ignore glStateBits and set everything
} glstate_t;


typedef struct {
	int		c_surfaces;
	int		c_shaders;
	int		c_vertexes;
	int		c_indexes;		// one set per pass
	int		c_totalIndexes;	// counting all passes

	int		c_drawElements;
	int		c_drawIndexes;
	int		c_drawVertexes;
	int		c_drawRefIndexes;
	int		c_drawRefVertexes;

	int		c_shadowElements;
	int		c_shadowIndexes;
	int		c_shadowVertexes;

	int		c_vboIndexes;
	int		c_matrixLoads;
	float	c_overDraw;

	uint	c_interactions, c_interactionSingleLights, c_interactionLights, c_interactionMaxLights, c_interactionMaxShadowMaps;
	uint	textureLoads, textureBackgroundLoads, textureLoadTime, textureUploadTime;

	int		msec;			// total msec for backend run
	int		msecLast;		// last msec for backend run
	char	waitedFor;		// . - backend, F = frontend, S - GPU Sync
} backEndCounters_t;

// all state modified by the back end is separated
// from the front end state
typedef struct {
	int					frameCount;		// used to track all images used in a frame
	const viewDef_t		*viewDef;
	backEndCounters_t	pc;

	const viewEntity_t *currentSpace;		// for detecting when a matrix must change
	idScreenRect		currentScissor;
	// for scissor clipping, local inside renderView viewport

	viewLight_t 		*vLight;
	int					depthFunc;			// GLS_DEPTHFUNC_EQUAL, or GLS_DEPTHFUNC_LESS for translucent
	float				lightColor[4];		// evaluation of current light's color stage

	float				lightScale;			// Every light color calaculation will be multiplied by this,
	// which will guarantee that the result is < tr.backEndRendererMaxLight
	// A card with high dynamic range will have this set to 1.0
	float				overBright;			// The amount that all light interactions must be multiplied by
	// with post processing to get the desired total light level.
	// A high dynamic range card will have this set to 1.0.

	bool				currentRenderCopied;	// true if any material has already referenced _currentRender
	bool				afterFogRendered;		// added post process pass.

	// Test if lightDepthBounds should be enabled or not
	/*bool				useLightDepthBounds;
	bool				lightDepthBoundsDisabled;*/

	// our OpenGL state deltas
	glstate_t			glState;

	int					c_copyFrameBuffer;
	int					c_copyDepthBuffer;
} backEndState_t;

const int MAX_GUI_SURFACES	= 1024;		// default size of the drawSurfs list for guis, will
// be automatically expanded as needed

typedef struct {
	int		x, y, width, height;	// these are in physical, OpenGL Y-at-bottom pixels
} renderCrop_t;
static const int	MAX_RENDER_CROPS = 8;

/*
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
// #4395 Duzenko lightem pixel pack buffer optimization
class idRenderSystemLocal : public idRenderSystem {
public:
	// external functions
	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			InitOpenGL( void );
	virtual void			ShutdownOpenGL( void );
	virtual bool			IsOpenGLRunning( void ) const;
	virtual bool			IsFullScreen( void ) const;
	virtual int				GetScreenWidth( void ) const;
	virtual int				GetScreenHeight( void ) const;
	virtual idRenderWorld 	*AllocRenderWorld( void );
	virtual void			FreeRenderWorld( idRenderWorld *rw );
	virtual void			BeginLevelLoad( void );
	virtual void			EndLevelLoad( void );
	virtual bool			RegisterFont( const char *fontName, fontInfoEx_t &font );
	virtual void			SetColor( const idVec4 &rgba );
	virtual void			SetColor4( float r, float g, float b, float a );
	virtual void			DrawStretchPic( const idDrawVert *verts, const glIndex_t *indexes, int vertCount, int indexCount, const idMaterial *material,
											bool clip = true, float x = 0.0f, float y = 0.0f, float w = 640.0f, float h = 0.0f );
	virtual void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material );

	virtual void			DrawStretchTri( idVec2 p1, idVec2 p2, idVec2 p3, idVec2 t1, idVec2 t2, idVec2 t3, const idMaterial *material );
	virtual void			GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc );
	virtual void			GetGLSettings( int &width, int &height );
	virtual void			PrintMemInfo( MemInfo_t *mi );

	virtual void			DrawSmallChar( int x, int y, int ch, const idMaterial *material );
	virtual void			DrawSmallStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material );
	virtual void			DrawBigChar( int x, int y, int ch, const idMaterial *material );
	virtual void			DrawBigStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor, const idMaterial *material );
	virtual void			WriteDemoPics();
	virtual void			DrawDemoPics();
	virtual void			BeginFrame( int windowWidth, int windowHeight );
	virtual void			EndFrame( int *frontEndMsec, int *backEndMsec );
	virtual void			TakeScreenshot( int width, int height, const char *fileName, int downSample, renderView_t *ref, bool envshot = false );
	virtual void			CropRenderSize( int width, int height, bool makePowerOfTwo = false, bool forceDimensions = false );
	virtual void			GetCurrentRenderCropSize( int &width, int &height );
	virtual void			CaptureRenderToImage( idImage &image );
	virtual void			CaptureRenderToFile( const char *fileName, bool fixAlpha );
	virtual void			CaptureRenderToBuffer( unsigned char *buffer, bool usePbo = false );
	virtual void			PostProcess();
	virtual void			UnCrop();
	virtual bool			UploadImage( const char *imageName, const byte *data, int width, int height );

public:
	// internal functions
	idRenderSystemLocal() { Clear(); }
	~idRenderSystemLocal() {}

	void					Clear( void );
	void					RenderViewToViewport( const renderView_t &renderView, idScreenRect &viewport );

public:
	// renderer globals
	bool					takingScreenshot;

	int						frameCount;			// incremented every frame
	int						viewCount;			// incremented every view (twice a scene if subviewed)
												// and every R_MarkFragments call

	int						staticAllocCount;	// running total of bytes allocated

	float					frameShaderTime;	// shader time for all non-world 2D rendering

	int						viewportOffset[2];	// for doing larger-than-window tiled renderings
	int						tiledViewport[2];

	idVec4					ambientLightVector;	// used for "ambient bump mapping"

	float					sortOffset;				// for determinist sorting of equal sort materials

	idList<idRenderWorldLocal *>worlds;

	idRenderWorldLocal 		*primaryWorld;
	renderView_t			primaryRenderView;
	viewDef_t 				*primaryView;

	// many console commands need to know which world they should operate on
	const idMaterial 		*defaultMaterial;
	const idMaterial 		*defaultShaderPoint;
	const idMaterial 		*defaultShaderProj;

	idImage 				*testImage;
	idCinematic 			*testVideo;
	float					testVideoStartTime;

	idImage 				*ambientCubeImage;	// hack for testing dependent ambient lighting

	viewDef_t 				*viewDef;

	performanceCounters_t	pc;					// performance counters

	viewEntity_t			identitySpace;		// can use if we don't know viewDef->worldSpace is valid
	FILE 					*logFile;			// for logging GL calls and frame breaks

	renderCrop_t			renderCrops[MAX_RENDER_CROPS];
	int						currentRenderCrop;

	// GUI drawing variables for surface creation
	int						guiRecursionLevel;		// to prevent infinite overruns
	class idGuiModel 		*guiModel;
	class idGuiModel 		*demoGuiModel;

	unsigned short			gammaTable[256];	// brightness / gamma modify this
	idParallelJobList*		frontEndJobList;
};

extern backEndState_t		backEnd;
extern idRenderSystemLocal	tr;
extern glconfig_t			glConfig;		// outside of TR since it shouldn't be cleared during ref re-init


//
// cvars
//
extern idCVar r_ext_vertex_array_range;

extern idCVar r_glDriver;				// "opengl32", etc
extern idCVar r_glDebugOutput;
extern idCVar r_glDebugContext;
extern idCVar r_displayRefresh;			// optional display refresh rate option for vid mode
extern idCVar r_fullscreen;				// 0 = windowed, 1 = full screen
extern idCVar r_multiSamples;			// number of antialiasing samples
extern idCVarBool r_fboSRGB;

extern idCVar r_ignore;					// used for random debugging without defining new vars
extern idCVar r_ignore2;				// used for random debugging without defining new vars
extern idCVar r_znear;					// near Z clip plane

extern idCVar r_finish;					// force a call to glFinish() every frame
extern idCVar r_frontBuffer;			// draw to front buffer for debugging
extern idCVarInt r_swapInterval;			// changes wglSwapIntarval
extern idCVar r_offsetFactor;			// polygon offset parameter
extern idCVar r_offsetUnits;			// polygon offset parameter
extern idCVar r_singleTriangle;			// only draw a single triangle per primitive
extern idCVar r_logFile;				// number of frames to emit GL logs
extern idCVar r_clear;					// force screen clear every frame
extern idCVar r_shadows;				// enable shadows
extern idCVar r_subviewOnly;			// 1 = don't render main view, allowing subviews to be debugged
extern idCVar r_lightScale;				// all light intensities are multiplied by this, which is normally 2
extern idCVar r_flareSize;				// scale the flare deforms from the material def

extern idCVar r_ambientMinLevel;		// tweaking overall ambient brightness
extern idCVar r_ambientGamma;			// tweaking overall ambient brightness

extern idCVar r_checkBounds;			// compare all surface bounds with precalculated ones

extern idCVar r_useLightPortalFlow;		// 1 = do a more precise area reference determination
extern idCVar r_useShadowSurfaceScissor;// 1 = scissor shadows by the scissor rect of the interaction surfaces
extern idCVar r_useConstantMaterials;	// 1 = use pre-calculated material registers if possible
extern idCVar r_useNodeCommonChildren;	// stop pushing reference bounds early when possible
extern idCVar r_useSilRemap;			// 1 = consider verts with the same XYZ, but different ST the same for shadows
extern idCVar r_useCulling;				// 0 = none, 1 = sphere, 2 = sphere + box
extern idCVar r_useLightPortalCulling;	// 0 = none, 1 = box, 2 = exact clip of polyhedron faces
extern idCVar r_useEntityPortalCulling;	// 0 = none, 1 = box
extern idCVar r_useLightCulling;		// 0 = none, 1 = box, 2 = exact clip of polyhedron faces
extern idCVar r_useLightScissors;		// 1 = use custom scissor rectangle for each light
extern idCVar r_useClippedLightScissors;// 0 = full screen when near clipped, 1 = exact when near clipped, 2 = exact always
extern idCVar r_useEntityCulling;		// 0 = none, 1 = box
extern idCVar r_useEntityScissors;		// 1 = use custom scissor rectangle for each entity
extern idCVar r_useInteractionCulling;	// 1 = cull interactions
extern idCVar r_useInteractionScissors;	// 1 = use a custom scissor rectangle for each interaction
extern idCVar r_useFrustumFarDistance;	// if != 0 force the view frustum far distance to this distance
extern idCVar r_useShadowCulling;		// try to cull shadows from partially visible lights
extern idCVar r_usePreciseTriangleInteractions;	// 1 = do winding clipping to determine if each ambiguous tri should be lit
extern idCVar r_useTurboShadow;			// 1 = use the infinite projection with W technique for dynamic shadows
extern idCVar r_useInteractionTable;
extern idCVar r_useExternalShadows;		// 1 = skip drawing caps when outside the light volume
extern idCVar r_useOptimizedShadows;	// 1 = use the dmap generated static shadow volumes
extern idCVar r_useShadowProjectedCull;	// 1 = discard triangles outside light volume before shadowing
extern idCVar r_useDeferredTangents;	// 1 = don't always calc tangents after deform
extern idCVar r_useCachedDynamicModels;	// 1 = cache snapshots of dynamic models
extern idCVar r_useScissor;				// 1 = scissor clip as portals and lights are processed
extern idCVar r_usePortals;				// 1 = use portals to perform area culling, otherwise draw everything
extern idCVar r_useStateCaching;		// avoid redundant state changes in GL_*() calls
extern idCVar r_useVertexBuffers;		// if 0, don't use ARB_vertex_buffer_object for vertexes
extern idCVar r_useIndexBuffers;		// if 0, don't use ARB_vertex_buffer_object for indexes
extern idCVar r_useEntityCallbacks;		// if 0, issue the callback immediately at update time, rather than defering
extern idCVar r_lightAllBackFaces;		// light all the back faces, even when they would be shadowed
extern idCVar r_useDepthBoundsTest;     // use depth bounds test to reduce shadow fill

extern idCVar r_skipPostProcess;		// skip all post-process renderings
extern idCVar r_skipSuppress;			// ignore the per-view suppressions
extern idCVar r_skipInteractions;		// skip all light/surface interaction drawing
extern idCVar r_skipFrontEnd;			// bypasses all front end work, but 2D gui rendering still draws
extern idCVar r_skipBackEnd;			// don't draw anything
extern idCVar r_skipCopyTexture;		// do all rendering, but don't actually copyTexSubImage2D
extern idCVar r_skipRender;				// skip 3D rendering, but pass 2D
extern idCVar r_skipRenderContext;		// NULL the rendering context during backend 3D rendering
extern idCVar r_skipTranslucent;		// skip the translucent interaction rendering
extern idCVar r_skipAmbient;			// bypasses all non-interaction drawing
extern idCVarInt r_skipNewAmbient;			// bypasses all vertex/fragment program ambients
extern idCVar r_skipBlendLights;		// skip all blend lights
extern idCVarInt r_skipFogLights;			// skip all fog lights
extern idCVarBool r_skipSubviews;			// 1 = don't render any mirrors / cameras / etc
extern idCVar r_skipGuiShaders;			// 1 = don't render any gui elements on surfaces
extern idCVar r_skipParticles;			// 1 = don't render any particles
extern idCVar r_skipUpdates;			// 1 = don't accept any entity or light updates, making everything static
extern idCVar r_skipDeforms;			// leave all deform materials in their original state
extern idCVar r_skipDynamicTextures;	// don't dynamically create textures
extern idCVar r_skipBump;				// uses a flat surface instead of the bump map
extern idCVar r_skipSpecular;			// use black for specular
extern idCVar r_skipDiffuse;			// use black for diffuse
extern idCVar r_skipOverlays;			// skip overlay surfaces
extern idCVar r_skipROQ;
extern idCVar r_skipDepthCapture;		// skip capture of early depth pass. revelator + SteveL #3877
extern idCVar r_useSoftParticles;		// SteveL #3878

extern idCVar r_ignoreGLErrors;

extern idCVar r_forceLoadImages;		// draw all images to screen after registration
extern idCVar r_demonstrateBug;			// used during development to show IHV's their problems
extern idCVar r_screenFraction;			// for testing fill rate, the resolution of the entire screen can be changed

extern idCVar r_showUnsmoothedTangents;	// highlight geometry rendered with unsmoothed tangents
extern idCVar r_showSilhouette;			// highlight edges that are casting shadow planes
extern idCVar r_showVertexColor;		// draws all triangles with the solid vertex color
extern idCVar r_showUpdates;			// report entity and light updates and ref counts
extern idCVar r_showDemo;				// report reads and writes to the demo file
extern idCVar r_showDynamic;			// report stats on dynamic surface generation
extern idCVar r_showIntensity;			// draw the screen colors based on intensity, red = 0, green = 128, blue = 255
extern idCVar r_showDefs;				// report the number of modeDefs and lightDefs in view
extern idCVar r_showTrace;				// show the intersection of an eye trace with the world
extern idCVar r_showSmp;				// show which end (front or back) is blocking
extern idCVar com_smp;					// enable SMP
extern idCVar r_showDepth;				// display the contents of the depth buffer and the depth range
extern idCVar r_showImages;				// draw all images to screen instead of rendering
extern idCVar r_showTris;				// enables wireframe rendering of the world
extern idCVar r_showSurfaceInfo;		// show surface material name under crosshair
extern idCVar r_showNormals;			// draws wireframe normals
extern idCVar r_showEdges;				// draw the sil edges
extern idCVar r_showViewEntitys;		// displays the bounding boxes of all view models and optionally the index
extern idCVarInt r_showEntityDraws;
extern idCVar r_showTexturePolarity;	// shade triangles by texture area polarity
extern idCVar r_showTangentSpace;		// shade triangles by tangent space
extern idCVar r_showDominantTri;		// draw lines from vertexes to center of dominant triangles
extern idCVar r_showTextureVectors;		// draw each triangles texture (tangent) vectors
extern idCVarInt r_showLights;			// 1 = print light info, 2 = also draw volumes
extern idCVar r_showLightCount;			// colors surfaces based on light count
extern idCVar r_showShadows;			// visualize the stencil shadow volumes
extern idCVar r_showShadowCount;		// colors screen based on shadow volume depth complexity
extern idCVar r_showLightScissors;		// show light scissor rectangles
extern idCVar r_showEntityScissors;		// show entity scissor rectangles
extern idCVar r_showInteractionFrustums;// show a frustum for each interaction
extern idCVar r_showInteractionScissors;// show screen rectangle which contains the interaction frustum
extern idCVar r_showMemory;				// print frame memory utilization
extern idCVar r_showCull;				// report sphere and box culling stats
extern idCVar r_showInteractions;		// report interaction generation activity
extern idCVar r_showSurfaces;			// report surface/light/shadow counts
extern idCVar r_showPrimitives;			// report vertex/index/draw counts
extern idCVar r_showMultiLight;			// 
extern idCVarInt r_showPortals;			// draw portal outlines in color based on passed / not passed
extern idCVar r_showAlloc;				// report alloc/free counts
extern idCVar r_showSkel;				// draw the skeleton when model animates
extern idCVar r_showOverDraw;			// show overdraw
extern idCVar r_jointNameScale;			// size of joint names when r_showskel is set to 1
extern idCVar r_jointNameOffset;		// offset of joint names when r_showskel is set to 1

extern idCVar r_testGamma;				// draw a grid pattern to test gamma levels
extern idCVar r_testStepGamma;			// draw a grid pattern to test gamma levels
extern idCVar r_testGammaBias;			// draw a grid pattern to test gamma levels

extern idCVar r_interactionProgram;			// experiment with vertex/fragment programs

extern idCVar r_singleLight;			// suppress all but one light
extern idCVar r_singleEntity;			// suppress all but one entity
extern idCVar r_singleArea;				// only draw the portal area the view is actually in
extern idCVar r_singleSurface;			// suppress all but one surface on each entity
extern idCVar r_shadowPolygonOffset;	// bias value added to depth test for stencil shadow drawing
extern idCVar r_shadowPolygonFactor;	// scale value for stencil shadow drawing
extern idCVar r_skipModels;

extern idCVar r_jitter;					// randomly subpixel jitter the projection matrix
extern idCVar r_lightSourceRadius;		// for soft-shadow sampling
extern idCVar r_lockSurfaces;
extern idCVar r_orderIndexes;			// perform index reorganization to optimize vertex use

extern idCVar r_debugLineDepthTest;		// perform depth test on debug lines
extern idCVar r_debugLineWidth;			// width of debug lines
extern idCVar r_debugArrowStep;			// step size of arrow cone line rotation in degrees
extern idCVar r_debugPolygonFilled;

extern idCVar r_materialOverride;		// override all materials

extern idCVar r_debugRenderToTexture;

// rebb: dedicated ambient
extern idCVar r_dedicatedAmbient;

extern idCVar r_softShadowsQuality;
extern idCVar r_softShadowsRadius;
extern idCVar r_useBumpmapLightTogglingFix;

extern idCVar r_useAnonreclaimer;
extern idCVarInt r_shadowMapSinglePass;

// stgatilov ROQ
extern idCVar r_cinematic_legacyRoq;

//stgatilov: temporary cvars, to be removed when ARB->GLSL migration is complete and settled
extern idCVar r_glCoreProfile;

//stgatilov #5816: volumetric light config
extern idCVar r_volumetricSamples;
extern idCVar r_volumetricDither;

/*
====================================================================

GL wrapper/helper functions

====================================================================
*/

void	GL_SelectTexture( int unit );
void	GL_CheckErrors( void );
void	GL_ClearStateDelta( void );
void	GL_State( int stateVector );
void	GL_Cull( int cullType );

/* Set scissor region in absolute screen coordinates */
void	GL_ScissorAbsolute( int x /* left*/, int y /* bottom */, int w, int h );
/* Set scissor region in glConfig vidSize screen coordinates, scaled by current FBO resolution */
void	GL_ScissorVidSize( int x /* left*/, int y /* bottom */, int w, int h );
/* Set scissor region in relative coordinates [0,1] */
void	GL_ScissorRelative( float x /* left*/, float y /* bottom */, float w, float h );

/* Set viewport in absolute screen coordinates */
void	GL_ViewportAbsolute( int x /* left */, int y /* bottom */, int w, int h );
/* Set viewport in glConfig vidSize screen coordinates, scaled by current FBO resolution */
void	GL_ViewportVidSize( int x /* left */, int y /* bottom */, int w, int h );
/* Set viewport in relative coordinates [0,1] */
void	GL_ViewportRelative( float x /* left */, float y /* bottom */, float w, float h );

void	GL_SetProjection( float* matrix );

// RAII-style wrapper for qglDepthBoundsEXT
class DepthBoundsTest {
public:
	DepthBoundsTest( const idScreenRect &scissorRect );
	~DepthBoundsTest();
};

// overloaded color functions vector first
struct GLColorOverride {
	bool enabled = false;

	GLColorOverride() {} // conditional override
	//GL_FloatColor( const idVec3& color );
	GLColorOverride( const idVec4& color );

	// float type
	GLColorOverride( const float* color );
	GLColorOverride( float r, float g, float b );
	GLColorOverride( float r, float g, float b, float a );

	void Enable( const float* color );
	
	~GLColorOverride();
};

#define MERGE_(a,b)  a##b
#define LABEL_(a) MERGE_(colorOverride_, a)
#define GL_FloatColor GLColorOverride LABEL_(__LINE__)

// byte type
void	GL_ByteColor( const byte* color );
void	GL_ByteColor( byte r, byte g, byte b );
void	GL_ByteColor( byte r, byte g, byte b, byte a );

const int GLS_SRCBLEND_ONE						= 0x0;
const int GLS_SRCBLEND_ZERO						= 0x00000001;
const int GLS_SRCBLEND_DST_COLOR				= 0x00000003;
const int GLS_SRCBLEND_ONE_MINUS_DST_COLOR		= 0x00000004;
const int GLS_SRCBLEND_SRC_ALPHA				= 0x00000005;
const int GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		= 0x00000006;
const int GLS_SRCBLEND_DST_ALPHA				= 0x00000007;
const int GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		= 0x00000008;
const int GLS_SRCBLEND_ALPHA_SATURATE			= 0x00000009;
const int GLS_SRCBLEND_BITS						= 0x0000000f;

const int GLS_DSTBLEND_ZERO						= 0x0;
const int GLS_DSTBLEND_ONE						= 0x00000020;
const int GLS_DSTBLEND_SRC_COLOR				= 0x00000030;
const int GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		= 0x00000040;
const int GLS_DSTBLEND_SRC_ALPHA				= 0x00000050;
const int GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		= 0x00000060;
const int GLS_DSTBLEND_DST_ALPHA				= 0x00000070;
const int GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		= 0x00000080;
const int GLS_DSTBLEND_BITS						= 0x000000f0;


// these masks are the inverse, meaning when set the glColorMask value will be 0,
// preventing that channel from being written
const int GLS_DEPTHMASK							= 0x00000100;
const int GLS_REDMASK							= 0x00000200;
const int GLS_GREENMASK							= 0x00000400;
const int GLS_BLUEMASK							= 0x00000800;
const int GLS_ALPHAMASK							= 0x00001000;
const int GLS_COLORMASK							= ( GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK );

const int GLS_POLYMODE_LINE						= 0x00002000;

const int GLS_DEPTHFUNC_ALWAYS					= 0x00010000;
const int GLS_DEPTHFUNC_EQUAL					= 0x00020000;
const int GLS_DEPTHFUNC_LESS					= 0x0;

/*const int GLS_ATEST_EQ_255						= 0x10000000;
const int GLS_ATEST_LT_128						= 0x20000000;
const int GLS_ATEST_GE_128						= 0x40000000;
const int GLS_ATEST_BITS						= 0x70000000;*/

const int GLS_DEFAULT							= GLS_DEPTHFUNC_ALWAYS;

void R_InitOpenGL( void );
void R_DoneFreeType( void );
void R_SetColorMappings( void );

void R_ScreenShot_f( const idCmdArgs &args );
void R_StencilShot( void );


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

typedef struct {
	int			width;
	int			height;
	bool		fullScreen;
	bool		stereo;
	int			displayHz;
	int			multiSamples;
} glimpParms_t;

bool		GLimp_Init( glimpParms_t parms );
// If the desired mode can't be set satisfactorily, false will be returned.
// The renderer will then reset the glimpParms to "safe mode" of 640x480
// fullscreen and try again.  If that also fails, the error will be fatal.

bool		GLimp_SetScreenParms( glimpParms_t parms );
// will set up gl up with the new parms

void		GLimp_Shutdown( void );
// Destroys the rendering context, closes the window, resets the resolution,
// and resets the gamma ramps.

void		GLimp_SwapBuffers( void );
// Calls the system specific swapbuffers routine, and may also perform
// other system specific cvar checks that happen every frame.
// This will not be called if 'r_drawBuffer GL_FRONT'

void		GLimp_SetGamma( unsigned short red[256],
							unsigned short green[256],
							unsigned short blue[256] );
// Sets the hardware gamma ramps for gamma and brightness adjustment.
// These are now taken as 16 bit values, so we can take full advantage
// of dacs with >8 bits of precision


bool		GLimp_SpawnRenderThread( void ( *function )( void ) );
// Returns false if the system only has a single processor

void 		*GLimp_BackEndSleep( void );
void		GLimp_FrontEndSleep( void );
void		GLimp_WakeBackEnd( void *data );
// these functions implement the dual processor syncronization

void		GLimp_ActivateContext( void );
void		GLimp_DeactivateContext( void );
// These are used for managing SMP handoffs of the OpenGL context
// between threads, and as a performance tunining aid.  Setting
// 'r_skipRenderContext 1' will call GLimp_DeactivateContext() before
// the 3D rendering code, and GLimp_ActivateContext() afterwards.  On
// most OpenGL implementations, this will result in all OpenGL calls
// being immediate returns, which lets us guage how much time is
// being spent inside OpenGL.

//void		GLimp_EnableLogging( bool enable );

/*
====================================================================

MAIN

====================================================================
*/

void R_RenderView( viewDef_t &parms );

bool R_RadiusCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes );
bool R_CornerCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes );
/*
=================
R_CullLocalBox

// performs radius cull first, then corner cull
Performs quick test before expensive test
Returns true if the box is outside the given global frustum, (positive sides are out)
Moved from tr_main.cpp to save on the function call cost
=================
*/
ID_INLINE bool R_CullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes ) {
	if ( R_RadiusCullLocalBox( bounds, modelMatrix, numPlanes, planes ) ) {
		return true;
	}
	return R_CornerCullLocalBox( bounds, modelMatrix, numPlanes, planes );
}

void R_AxisToModelMatrix( const idMat3 &axis, const idVec3 &origin, float modelMatrix[16] );

// note that many of these assume a normalized matrix, and will not work with scaled axis
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane &in, idPlane &out );
void R_PointTimesMatrix( const float modelMatrix[16], const idVec4 &in, idVec4 &out );
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane &in, idPlane &out );
void R_TransformEyeZToWin( float src_z, const float *projectionMatrix, float &dst_z );

void R_GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc );

void R_TransformModelToClip( const idVec3 &src, const float *modelMatrix, const float *projectionMatrix, idPlane &eye, idPlane &dst );

void R_TransformClipToDevice( const idPlane &clip, const viewDef_t *view, idVec3 &normalized );

void R_TransposeGLMatrix( const float in[16], float out[16] );

void R_SetViewMatrix( viewDef_t &viewDef );

// corrected this one.
void myGlMultMatrix( const float a[16], const float b[16], float out[16] );

/*
============================================================

LIGHT

============================================================
*/

void R_ListRenderLightDefs_f( const idCmdArgs &args );
void R_ListRenderEntityDefs_f( const idCmdArgs &args );

bool R_IssueEntityDefCallback( idRenderEntityLocal *def );
idRenderModel *R_EntityDefDynamicModel( idRenderEntityLocal *def );

viewEntity_t *R_SetEntityDefViewEntity( idRenderEntityLocal *def );
viewLight_t *R_SetLightDefViewLight( idRenderLightLocal *def );

void R_AddDrawSurf( const srfTriangles_t *tri, const viewEntity_t *space, const renderEntity_t *renderEntity,
					const idMaterial *shader, const idScreenRect &scissor, const float soft_particle_radius = -1.0f, bool deferred = false ); // soft particles in #3878

drawSurf_t *R_PrepareLightSurf( const srfTriangles_t *tri, const viewEntity_t *space,
					const idMaterial *shader, const idScreenRect &scissor, bool viewInsideShadow );

bool R_CreateAmbientCache( srfTriangles_t *tri, bool needsLighting );
void R_CreatePrivateShadowCache( srfTriangles_t *tri );
void R_CreateVertexProgramShadowCache( srfTriangles_t *tri );

/*
============================================================

LIGHTRUN

============================================================
*/
void R_RegenerateWorld_f( const idCmdArgs &args );

void R_ModulateLights_f( const idCmdArgs &args );

void R_SetLightProject( idPlane lightProject[4], const idVec3 origin, const idVec3 targetPoint,
						const idVec3 rightVector, const idVec3 upVector, const idVec3 start, const idVec3 stop );

void R_AddLightSurfaces( void );
void R_AddModelSurfaces( void );
void R_RemoveUnecessaryViewLights( void );

void R_FreeDerivedData( void );
void R_ReCreateWorldReferences( void );
//anon begin
void R_DeriveEntityData( idRenderEntityLocal *def );
//anon end

void R_CreateEntityRefs( idRenderEntityLocal *def );
void R_CreateLightRefs( idRenderLightLocal *light );

void R_DeriveLightData( idRenderLightLocal *light );
void R_FreeLightDefDerivedData( idRenderLightLocal *light );
void R_CheckForEntityDefsUsingModel( idRenderModel *model );

void R_ClearEntityDefDynamicModel( idRenderEntityLocal *def );
void R_FreeEntityDefDerivedData( idRenderEntityLocal *def, bool keepDecals, bool keepCachedDynamicModel );
void R_FreeEntityDefDecals( idRenderEntityLocal *def );
void R_FreeEntityDefOverlay( idRenderEntityLocal *def );
void R_FreeEntityDefFadedDecals( idRenderEntityLocal *def, int time );

void R_CreateLightDefFogPortals( idRenderLightLocal *ldef );

/*
============================================================

POLYTOPE

============================================================
*/

srfTriangles_t *R_PolytopeSurface( int numPlanes, const idPlane *planes, idWinding *windings );
bool R_PolytopeSurfaceFrustumLike( const idPlane planes[6], idVec3 vertices[8], idWinding windings[6], srfTriangles_t* *surface = NULL );

/*
============================================================

RENDER

============================================================
*/

void RB_EnterWeaponDepthHack();
void RB_EnterModelDepthHack( float depth );
void RB_LeaveDepthHack();
void RB_DrawElementsImmediate( const srfTriangles_t *tri );
void RB_RenderTriangleSurface( const drawSurf_t *surf );
void RB_T_RenderTriangleSurface( const drawSurf_t *surf );
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs,
										void ( *triFunc_ )( const drawSurf_t * ) );
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs,
		void ( *triFunc_ )( const drawSurf_t * ) );
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const shaderStage_t* pStage );
void RB_GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] );
void RB_CreateSingleDrawInteractions( const drawSurf_t *surf );

void RB_DrawView();

void RB_BeginDrawingView( void );

/*
============================================================

DRAW_STANDARD

============================================================
*/

void RB_DrawElementsWithCounters( const drawSurf_t* surf );
void RB_DrawElementsInstanced( const drawSurf_t *surf, int instances );
void RB_DrawTriangles( const srfTriangles_t& tri );
void RB_DrawShadowElementsWithCounters( const drawSurf_t *surf );
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters );
void RB_StencilShadowPass( const drawSurf_t *drawSurfs );
void RB_STD_DrawView( void );

// multi draw
void RB_Multi_AddSurf( const drawSurf_t* surf );
void RB_Multi_DrawElements( int instances = 0 );

// postprocessing related
void RB_DrawFullScreenQuad( float e = 1 );
void RB_DrawFullScreenTri();

/*
============================================================
DRAW_*
============================================================
*/

void    RB_GLSL_DrawInteraction( const drawInteraction_t *din );
void    RB_GLSL_DrawInteractions( void );
void	R_ReloadGLSLPrograms_f( const idCmdArgs &args );

/*
============================================================

TR_STENCILSHADOWS

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1

============================================================
*/

void R_MakeShadowFrustums( idRenderLightLocal *def );

typedef enum {
	SG_DYNAMIC,		// use infinite projections
	SG_STATIC,		// clip to bounds
	SG_OFFLINE		// perform very time consuming optimizations
} shadowGen_t;

srfTriangles_t *R_CreateShadowVolume( const idRenderEntityLocal *ent,
									  const srfTriangles_t *tri, const idRenderLightLocal *light,
									  shadowGen_t optimize, srfCullInfo_t &cullInfo );

/*
============================================================

TR_TURBOSHADOW

Fast, non-clipped overshoot shadow volumes

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1
calling this function may modify "facing" based on culling

============================================================
*/

srfTriangles_t *R_CreateVertexProgramTurboShadowVolume( const idRenderEntityLocal *ent,
		const srfTriangles_t *tri, const idRenderLightLocal *light,
		srfCullInfo_t &cullInfo );

/*
============================================================

util/shadowopt3

dmap time optimization of shadow volumes, called from R_CreateShadowVolume

============================================================
*/


typedef struct {
	idVec3	*verts;			// includes both front and back projections, caller should free
	int		numVerts;
	glIndex_t	*indexes;	// caller should free

	// indexes must be sorted frontCap, rearCap, silPlanes so the caps can be removed
	// when the viewer is in a position that they don't need to see them
	int		numFrontCapIndexes;
	int		numRearCapIndexes;
	int		numSilPlaneIndexes;
	int		totalIndexes;
} optimizedShadow_t;

optimizedShadow_t SuperOptimizeOccluders( idVec4 *verts, glIndex_t *indexes, int numIndexes,
		idPlane projectionPlane, idVec3 projectionOrigin );

void CleanupOptimizedShadowTris( srfTriangles_t *tri );

/*
============================================================

TRISURF

============================================================
*/

//#define USE_TRI_DATA_ALLOCATOR

void				R_InitTriSurfData( void );
void				R_ShutdownTriSurfData( void );
void				R_PurgeTriSurfData( frameData_t *frame );
void				R_ShowTriSurfMemory_f( const idCmdArgs &args );

srfTriangles_t 	*R_AllocStaticTriSurf( void );
srfTriangles_t 	*R_CopyStaticTriSurf( const srfTriangles_t *tri );
void				R_AllocStaticTriSurfVerts( srfTriangles_t *tri, int numVerts );
void				R_AllocStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes );
void				R_AllocStaticTriSurfShadowVerts( srfTriangles_t *tri, int numVerts );
void				R_AllocStaticTriSurfPlanes( srfTriangles_t *tri, int numIndexes );
void				R_ResizeStaticTriSurfVerts( srfTriangles_t *tri, int numVerts );
void				R_ResizeStaticTriSurfIndexes( srfTriangles_t *tri, int numIndexes );
void				R_ResizeStaticTriSurfShadowVerts( srfTriangles_t *tri, int numVerts );
void				R_ReferenceStaticTriSurfVerts( srfTriangles_t *tri, const srfTriangles_t *reference );
void				R_ReferenceStaticTriSurfIndexes( srfTriangles_t *tri, const srfTriangles_t *reference );
void				R_FreeStaticTriSurfSilIndexes( srfTriangles_t *tri );
void				R_FreeStaticTriSurf( srfTriangles_t *tri );
void				R_FreeStaticTriSurfVertexCaches( srfTriangles_t *tri );
void				R_FreeStaticTriSurfIndexes( srfTriangles_t *tri );
void				R_ReallyFreeStaticTriSurf( srfTriangles_t *tri );
void				R_FreeDeferredTriSurfs( frameData_t *frame );
int					R_TriSurfMemory( const srfTriangles_t *tri );

void				R_BoundTriSurf( srfTriangles_t *tri );
void				R_RemoveDuplicatedTriangles( srfTriangles_t *tri );
void				R_CreateSilIndexes( srfTriangles_t *tri );
void				R_RemoveDegenerateTriangles( srfTriangles_t *tri );
void				R_RemoveUnusedVerts( srfTriangles_t *tri );
void				R_RangeCheckIndexes( const srfTriangles_t *tri );
void				R_CreateVertexNormals( srfTriangles_t *tri );	// also called by dmap
void				R_DeriveFacePlanes( srfTriangles_t *tri );		// also called by renderbump
void				R_CleanupTriangles( srfTriangles_t *tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents );
void				R_ReverseTriangles( srfTriangles_t *tri );

// Only deals with vertexes and indexes, not silhouettes, planes, etc.
// Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
srfTriangles_t 		*R_MergeSurfaceList( const srfTriangles_t **surfaces, int numSurfaces );
srfTriangles_t 		*R_MergeTriangles( const srfTriangles_t *tri1, const srfTriangles_t *tri2 );

// if the deformed verts have significant enough texture coordinate changes to reverse the texture
// polarity of a triangle, the tangents will be incorrect
void				R_DeriveTangents( srfTriangles_t *tri, bool allocFacePlanes = true );

// For static surfaces, the indexes, ambient, and shadow buffers can be pre-created at load
// time, rather than being re-created each frame in the frame temporary buffers.
void				R_CreateStaticBuffersForTri( srfTriangles_t &tri );

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete srfTriangles_t from just a new set of vertexes
typedef struct deformInfo_s {
	int				numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int				numOutputVerts;

	int				numMirroredVerts;
	int 			*mirroredVerts;

	int				numIndexes;
	glIndex_t 		*indexes;

	glIndex_t 		*silIndexes;

	int				numDupVerts;
	int 			*dupVerts;

	int				numSilEdges;
	silEdge_t 		*silEdges;

	dominantTri_t 	*dominantTris;
} deformInfo_t;


deformInfo_t 		*R_BuildDeformInfo( int numVerts, const idDrawVert *verts, int numIndexes, const int *indexes, bool useUnsmoothedTangents );
void				R_FreeDeformInfo( deformInfo_t *deformInfo );
int					R_DeformInfoMemoryUsed( deformInfo_t *deformInfo );

/*
============================================================

SUBVIEW

============================================================
*/

bool	R_PreciseCullSurface( const drawSurf_t *drawSurf, idBounds &ndcBounds );
bool	R_GenerateSubViews( void );

/*
============================================================

SCENE GENERATION

============================================================
*/

void R_InitFrameData( void );
void R_ShutdownFrameData( void );
void R_ToggleSmpFrame( void );
void *R_FrameAlloc( int bytes );
void *R_ClearedFrameAlloc( int bytes );
void R_FrameFree( void *data );

void *R_StaticAlloc( int bytes );		// just malloc with error checking
void *R_ClearedStaticAlloc( int bytes );	// with memset
void R_StaticFree( void *data );


/*
=============================================================

RENDERER DEBUG TOOLS

=============================================================
*/

float R_DrawTextLength( const char *text, float scale, int len );
void R_AddDebugText( const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align, const int lifetime, const bool depthTest );
void R_ClearDebugText( int time );
void R_AddDebugLine( const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifeTime, const bool depthTest );
void R_ClearDebugLines( int time );
void R_AddDebugPolygon( const idVec4 &color, const idWinding &winding, const int lifeTime, const bool depthTest );
void R_ClearDebugPolygons( int time );
void RB_ShowLights( void );
void RB_ShowLightCount( void );
void RB_ScanStencilBuffer( void );
void RB_ShowDestinationAlpha( void );
void RB_ShowOverdraw( void );
void R_Tools();
void RB_RenderDebugTools( drawSurf_t **drawSurfs, int numDrawSurfs );
void RB_ShutdownDebugTools( void );
void RB_CopyDebugPrimitivesToBackend( void );

/*
=============================================================

TR_BACKEND

=============================================================
*/

void RB_SetDefaultGLState( void );
void RB_SetGL2D( void );

// write a comment to the r_logFile if it is enabled
//void RB_LogComment( const char *comment, ... ) id_attribute( (format( printf, 1, 2 )) );
// duzenko: switch to define so that we have a chance to skip the function calls getting the parameters
#define RB_LogComment(comment, ...)						\
	if ( tr.logFile ) {									\
		fprintf( tr.logFile, "// " );					\
		fprintf( tr.logFile, comment, ##__VA_ARGS__ );	\
	}													

void RB_ShowImages( void );

void RB_ExecuteBackEndCommands( const emptyCommand_t *cmds );


/*
=============================================================

TR_GUISURF

=============================================================
*/

void R_SurfaceToTextureAxis( const srfTriangles_t *tri, idVec3 &origin, idVec3 axis[3] );
void R_RenderGuiSurf( idUserInterface *gui, drawSurf_t *drawSurf );

/*
=============================================================

TR_ORDERINDEXES

=============================================================
*/

void R_OrderIndexes( int numIndexes, glIndex_t *indexes );

/*
=============================================================

TR_DEFORM

=============================================================
*/

void R_DeformDrawSurf( drawSurf_t *drawSurf );

/*
=============================================================

TR_TRACE

=============================================================
*/

typedef struct {
	float		fraction;
	// only valid if fraction < 1.0
	idVec3		point;
	idVec3		normal;
	int			indexes[3];
} localTrace_t;

localTrace_t R_LocalTrace( const idVec3 &start, const idVec3 &end, const float radius, const srfTriangles_t *tri );

/*
=============================================================

TR_SHADOWBOUNDS

=============================================================
*/
idScreenRect R_CalcIntersectionScissor( const idRenderLightLocal *lightDef,
										const idRenderEntityLocal *entityDef,
										const viewDef_t *viewDef );

//=============================================

#include "RenderWorld_local.h"
#include "GuiModel.h"
#include "VertexCache.h"

#endif /* !__TR_LOCAL_H__ */
