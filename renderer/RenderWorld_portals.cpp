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

#include "precompiled.h"
#pragma hdrstop



#include "tr_local.h"

//anon begin
idCVar r_useLightAreaCulling( "r_useLightAreaCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = off, 1 = on" );
//anon end

/*


All that is done in these functions is the creation of viewLights
and viewEntitys for the lightDefs and entityDefs that are visible
in the portal areas that can be seen from the current viewpoint.

*/


// if we hit this many planes, we will just stop cropping the
// view down, which is still correct, just conservative
const int MAX_PORTAL_PLANES	= 20;

// stgatilov: ensure that normal vector of a portal plane has error less than this coefficient
// if after winding is clipped, we see a portal plane with larger error, we simply drop such a plane
const float MAX_PLANE_NORMAL_ERROR = 0.001f;

typedef struct portalStack_s {
	portal_t	*p;
	const struct portalStack_s *next;

	idScreenRect	rect;

	int			numPortalPlanes;
	idPlane		portalPlanes[MAX_PORTAL_PLANES + 1];
	// positive side is outside the visible frustum
} portalStack_t;


//====================================================================


/*
===================
idRenderWorldLocal::ScreenRectForWinding
===================
*/
idScreenRect idRenderWorldLocal::ScreenRectFromWinding( const idWinding *w, viewEntity_t *space ) {
	idScreenRect	r;
	int				i;
	idVec3			v;
	idVec3			ndc;
	float			windowX, windowY;

	r.Clear();
	for ( i = 0 ; i < w->GetNumPoints() ; i++ ) {
		R_LocalPointToGlobal( space->modelMatrix, ( *w )[i].ToVec3(), v );
		R_GlobalToNormalizedDeviceCoordinates( v, ndc );

		windowX = 0.5f * ( 1.0f + ndc[0] ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 );
		windowY = 0.5f * ( 1.0f + ndc[1] ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 );

		r.AddPoint( windowX, windowY );
	}
	r.Expand();

	return r;
}

/*
===================
PortalIsFoggedOut
===================
*/
bool idRenderWorldLocal::PortalIsFoggedOut( const portal_t *p ) {
	idRenderLightLocal	*ldef;
	int			i;
	idPlane		forward;

	ldef = p->doublePortal->fogLight;

	if ( !ldef ) {
		return false;
	}

	// find the current density of the fog
	const idMaterial	*lightShader = ldef->lightShader;
	int		size = sizeof( float ) * lightShader->GetNumRegisters();
	float	*regs = ( float * )_alloca( size );

	lightShader->EvaluateRegisters( regs, ldef->parms.shaderParms, tr.viewDef, ldef->parms.referenceSound );

	const shaderStage_t	*stage = lightShader->GetStage( 0 );

	float a, alpha = regs[ stage->color.registers[3] ];

	// if they left the default value on, set a fog distance of 500
	if ( alpha <= 1.0f ) {
		a = -0.5f / DEFAULT_FOG_DISTANCE;
	} else {
		// otherwise, distance = alpha color
		a = -0.5f / alpha;
	}
	forward[0] = a * tr.viewDef->worldSpace.modelViewMatrix[2];
	forward[1] = a * tr.viewDef->worldSpace.modelViewMatrix[6];
	forward[2] = a * tr.viewDef->worldSpace.modelViewMatrix[10];
	forward[3] = a * tr.viewDef->worldSpace.modelViewMatrix[14];

	auto w = &p->w;

	for ( i = 0 ; i < w->GetNumPoints() ; i++ ) {
		float d = forward.Distance( ( *w )[i].ToVec3() );
		if ( d < 0.5f ) {
			return false;		// a point not clipped off
		}
	}
	return true;
}

/*
===================
FloodViewThroughArea_r
===================
*/
void idRenderWorldLocal::FloodViewThroughArea_r( const idVec3 origin, int areaNum,
        const struct portalStack_s *ps ) {
	//portal_t*		p;
	float			d;
	const portalStack_t	*check;
	portalStack_t	newStack;
	int				j;
	idVec3			v1, v2;
	int				addPlanes;
	idFixedWinding	w;		// we won't overflow because MAX_PORTAL_PLANES = 20

	auto &area = portalAreas[ areaNum ];

	// cull models and lights to the current collection of planes
	AddAreaRefs( areaNum, ps );

	if ( portalAreas[areaNum].areaScreenRect.IsEmpty() ) {
		portalAreas[areaNum].areaScreenRect = ps->rect;
	} else {
		portalAreas[areaNum].areaScreenRect.Union( ps->rect );
	}

	// go through all the portals
	for ( auto p : area.areaPortals ) {
		// an enclosing door may have sealed the portal off
		if ( p->doublePortal->blockingBits & PS_BLOCK_VIEW ) {
			continue;
		}

		// make sure this portal is facing away from the view
		d = p->plane.Distance( origin );
		if ( d < -0.1f ) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for ( check = ps; check; check = check->next ) {
			if ( check->p == p ) {
				break;		// don't recursively enter a stack
			}
		}
		if ( check ) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if ( d < 1.0f ) {
			// SteveL #3815: check the view origin is really in front of the portal
			idBounds pBounds; // uninitialized
			p->w.GetBounds( pBounds );
			pBounds.ExpandSelf( 1.0f );
			if ( pBounds.ContainsPoint( origin ) ) {
				// go through this portal
				newStack = *ps;
				newStack.p = p;
				newStack.next = ps;
				p->doublePortal->portalViewCount = tr.viewCount;	// For r_showPortals. Keep track whether the player's view flows through
				// individual portals, not just whole visleafs.  -- SteveL #4162
				FloodViewThroughArea_r( origin, p->intoArea, &newStack );
				continue;
			}
		}

		// clip the portal winding to all of the planes
		w = p->w;
		for ( j = 0; j < ps->numPortalPlanes; j++ ) {
			if ( !w.ClipInPlace( -ps->portalPlanes[j], 0 ) ) {
				break;
			}
		}
		if ( !w.GetNumPoints() ) {
			continue;	// portal not visible
		}

		// see if it is fogged out
		if ( PortalIsFoggedOut( p ) ) {
			continue;
		}

		// go through this portal
		newStack.p = p;
		newStack.next = ps;
		p->doublePortal->portalViewCount = tr.viewCount;	// For r_showPortals. Keep track whether the player's view flows through
		// individual portals, not just whole visleafs.  -- SteveL #4162

		// find the screen pixel bounding box of the remaining portal
		// so we can scissor things outside it
		newStack.rect = ScreenRectFromWinding( &w, &tr.identitySpace );

		// slop might have spread it a pixel outside, so trim it back
		newStack.rect.Intersect( ps->rect );

		// generate a set of clipping planes that will further restrict
		// the visible view beyond just the scissor rect
		addPlanes = w.GetNumPoints();
		if ( addPlanes > MAX_PORTAL_PLANES ) {
			addPlanes = MAX_PORTAL_PLANES;
		}
		newStack.numPortalPlanes = 0;

		for ( int i = 0; i < addPlanes; i++ ) {
			j = i + 1;
			if ( j == w.GetNumPoints() ) {
				j = 0;
			}
			v1 = origin - w[i].ToVec3();
			v2 = origin - w[j].ToVec3();

			//stgatilov: drop plane if its direction is not precise enough
			idVec3 normal;
			normal.Cross( v2, v1 );
			float sinAng = normal.LengthFast() * idMath::RSqrt( v2.LengthSqr() * v1.LengthSqr() );
			static const float SIN_THRESHOLD = idMath::FLT_EPS / MAX_PLANE_NORMAL_ERROR;
			if ( sinAng <= SIN_THRESHOLD ) {
				continue;
			}
			newStack.portalPlanes[newStack.numPortalPlanes].Normal() = normal;
			newStack.portalPlanes[newStack.numPortalPlanes].Normalize();
			newStack.portalPlanes[newStack.numPortalPlanes].FitThroughPoint( origin );

			newStack.numPortalPlanes++;
		}

		// the last stack plane is the portal plane
		newStack.portalPlanes[newStack.numPortalPlanes] = p->plane;
		newStack.numPortalPlanes++;

		FloodViewThroughArea_r( origin, p->intoArea, &newStack );
	}
}

/*
=======================
FlowViewThroughPortals

Finds viewLights and viewEntities by flowing from an origin through the visible portals.
origin point can see into.  The planes array defines a volume (positive
sides facing in) that should contain the origin, such as a view frustum or a point light box.
Zero planes assumes an unbounded volume.
=======================
*/
void idRenderWorldLocal::FlowViewThroughPortals( const idVec3 origin, int numPlanes, const idPlane *planes ) {
	portalStack_t	ps;
	int				i;

	ps.next = nullptr;
	ps.p = nullptr;

	for ( i = 0 ; i < numPlanes ; i++ ) {
		ps.portalPlanes[i] = planes[i];
	}
	ps.numPortalPlanes = numPlanes;
	ps.rect = tr.viewDef->scissor;

	if ( tr.viewDef->areaNum < 0 ) {

		for ( auto &a : portalAreas ) {
			a.areaScreenRect = tr.viewDef->scissor;
		}

		// if outside the world, mark everything
		for ( i = 0; i < ( int )portalAreas.size(); i++ ) {
			AddAreaRefs( i, &ps );
		}
	} else {

		for ( auto &a : portalAreas ) {
			a.areaScreenRect.Clear();
		}

		// flood out through portals, setting area viewCount
		FloodViewThroughArea_r( origin, tr.viewDef->areaNum, &ps );
	}
}

//==================================================================================================


/*
===================
FloodLightThroughArea_r
===================
*/
void idRenderWorldLocal::FloodLightThroughArea_r( idRenderLightLocal *light, int areaNum,
        const struct portalStack_s *ps ) {
	float			d;
	portalArea_t 	*area;
	const portalStack_t	*check, *firstPortalStack;
	portalStack_t	newStack;
	int				j;
	idVec3			v1, v2;
	int				addPlanes;
	idFixedWinding	w;		// we won't overflow because MAX_PORTAL_PLANES = 20

	area = &portalAreas[ areaNum ];

	// add an areaRef
	AddLightRefToArea( light, area );

	// go through all the portals
	for ( auto p : area->areaPortals ) {
		// make sure this portal is facing away from the view
		d = p->plane.Distance( light->globalLightOrigin );
		if ( d < -0.1f ) {
			continue;
		}

		// make sure the portal isn't in our stack trace,
		// which would cause an infinite loop
		for ( check = ps; check; check = check->next ) {
			firstPortalStack = check;
			if ( check->p == p ) {
				break;		// don't recursively enter a stack
			}
		}

		if ( check ) {
			continue;	// already in stack
		}

		// if we are very close to the portal surface, don't bother clipping
		// it, which tends to give epsilon problems that make the area vanish
		if ( d < 1.0f ) {
			// go through this portal
			newStack = *ps;
			newStack.p = p;
			newStack.next = ps;
			FloodLightThroughArea_r( light, p->intoArea, &newStack );
			continue;
		}

		// clip the portal winding to all of the planes
		w = p->w;
		for ( j = 0; j < ps->numPortalPlanes; j++ ) {
			if ( !w.ClipInPlace( -ps->portalPlanes[j], 0 ) ) {
				break;
			}
		}

		if ( !w.GetNumPoints() ) {
			continue;	// portal not visible
		}

		// also always clip to the original light planes, because they aren't
		// necessarily extending to infinitiy like a view frustum
		for ( j = 0; j < firstPortalStack->numPortalPlanes; j++ ) {
			if ( !w.ClipInPlace( -firstPortalStack->portalPlanes[j], 0 ) ) {
				break;
			}
		}
		if ( !w.GetNumPoints() ) {
			continue;	// portal not visible
		}

		// go through this portal
		newStack.p = p;
		newStack.next = ps;

		// generate a set of clipping planes that will further restrict
		// the visible view beyond just the scissor rect
		addPlanes = w.GetNumPoints();
		if ( addPlanes > MAX_PORTAL_PLANES ) {
			addPlanes = MAX_PORTAL_PLANES;
		}
		newStack.numPortalPlanes = 0;

		for ( int i = 0; i < addPlanes; i++ ) {
			j = i + 1;
			if ( j == w.GetNumPoints() ) {
				j = 0;
			}
			v1 = light->globalLightOrigin - w[i].ToVec3();
			v2 = light->globalLightOrigin - w[j].ToVec3();

			//stgatilov: drop plane if its direction is not precise enough
			idVec3 normal;
			normal.Cross( v2, v1 );
			float sinAng = normal.LengthFast() * idMath::RSqrt( v2.LengthSqr() * v1.LengthSqr() );
			static const float SIN_THRESHOLD = idMath::FLT_EPS / MAX_PLANE_NORMAL_ERROR;
			if ( sinAng <= SIN_THRESHOLD ) { 
				continue; 
			}
			newStack.portalPlanes[newStack.numPortalPlanes].Normal() = normal;
			newStack.portalPlanes[newStack.numPortalPlanes].Normalize();
			newStack.portalPlanes[newStack.numPortalPlanes].FitThroughPoint( light->globalLightOrigin );

			newStack.numPortalPlanes++;
		}
		FloodLightThroughArea_r( light, p->intoArea, &newStack );
	}
}


/*
=======================
FlowLightThroughPortals

Adds an arearef in each area that the light center flows into.
This can only be used for shadow casting lights that have a generated
prelight, because shadows are cast from back side which may not be in visible areas.
=======================
*/
void idRenderWorldLocal::FlowLightThroughPortals( idRenderLightLocal *light ) {
	portalStack_t	ps;
	if ( r_useAnonreclaimer.GetBool() ) {
		// if the light origin areaNum is not in a valid area,
		// the light won't have any area refs
		if ( light->areaNum == -1 ) {
			return;
		}
		idPlane frustumPlanes[6];
		idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );

		memset( &ps, 0, sizeof( ps ) );
		ps.numPortalPlanes = 6;
		for ( int i = 0; i < 6; i++ ) {
			ps.portalPlanes[i] = -frustumPlanes[i];
		}
	} else {
		const idVec3	origin = light->globalLightOrigin;

		// if the light origin areaNum is not in a valid area,
		// the light won't have any area refs
		if ( light->areaNum == -1 ) {
			return;
		}
		memset( &ps, 0, sizeof( ps ) );

		ps.numPortalPlanes = 6;
		for ( int i = 0; i < 6; i++ ) {
			ps.portalPlanes[i] = light->frustum[i];
		}
	}
	FloodLightThroughArea_r( light, light->areaNum, &ps );
}

//======================================================================================================

/*
===================
idRenderWorldLocal::FloodFrustumAreas_r
===================
*/
areaNumRef_t *idRenderWorldLocal::FloodFrustumAreas_r( const idFrustum &frustum, const int areaNum, const idBounds &bounds, areaNumRef_t *areas ) {
	idBounds		newBounds;
	areaNumRef_t	*a;
	auto			&portalArea = portalAreas[ areaNum ];

	// go through all the portals
	for ( auto const &p : portalArea.areaPortals ) {
		// check if we already visited the area the portal leads to
		for ( a = areas; a; a = a->next ) {
			if ( a->areaNum == p->intoArea ) {
				break;
			}
		}

		if ( a ) {
			continue;
		}

		// the frustum origin must be at the front of the portal plane
		if ( p->plane.Side( frustum.GetOrigin(), 0.1f ) == SIDE_BACK ) {
			continue;
		}

		// the frustum must cross the portal plane
		if ( frustum.PlaneSide( p->plane, 0.0f ) != PLANESIDE_CROSS ) {
			continue;
		}

		// get the bounds for the portal winding projected in the frustum
		frustum.ProjectionBounds( p->w, newBounds );

		newBounds.IntersectSelf( bounds );

		if ( newBounds[0][0] > newBounds[1][0] || newBounds[0][1] > newBounds[1][1] || newBounds[0][2] > newBounds[1][2] ) {
			continue;
		}
		newBounds[1][0] = frustum.GetFarDistance();

		a = areaNumRefAllocator.Alloc();
		a->areaNum = p->intoArea;
		a->next = areas;
		areas = a;

		areas = FloodFrustumAreas_r( frustum, p->intoArea, newBounds, areas );
	}
	return areas;
}

/*
===================
idRenderWorldLocal::FloodFrustumAreas

  Retrieves all the portal areas the frustum floods into where the frustum starts in the given areas.
  All portals are assumed to be open.
===================
*/
areaNumRef_t *idRenderWorldLocal::FloodFrustumAreas( const idFrustum &frustum, areaNumRef_t *areas ) {
	idBounds bounds;
	areaNumRef_t *a;

	// bounds that cover the whole frustum
	bounds[0].Set( frustum.GetNearDistance(), -1.0f, -1.0f );
	bounds[1].Set( frustum.GetFarDistance(), 1.0f, 1.0f );

	for ( a = areas; a; a = a->next ) {
		areas = FloodFrustumAreas_r( frustum, a->areaNum, bounds, areas );
	}
	return areas;
}


/*
=======================================================================

R_FindViewLightsAndEntities

=======================================================================
*/

/*
================
CullEntityByPortals

Return true if the entity reference bounds do not intersect the current portal chain.
================
*/
bool idRenderWorldLocal::CullEntityByPortals( const idRenderEntityLocal *entity, const portalStack_t *ps ) {
	if ( r_useAnonreclaimer.GetBool() ) {
		if ( r_useEntityPortalCulling.GetInteger() == 1 ) {
			ALIGNTYPE16 frustumCorners_t corners;
			idRenderMatrix::GetFrustumCorners( corners, entity->inverseBaseModelProject, bounds_unitCube );
			for ( int i = 0; i < ps->numPortalPlanes; i++ ) {
				if ( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT ) {
					return true;
				}
			}
		} else if ( r_useEntityPortalCulling.GetInteger() >= 2 ) {

			idRenderMatrix baseModelProject;
			// Was idRenderMatrix::Inverse but changed to InverseByDoubles for precision
			idRenderMatrix::Inverse( entity->inverseBaseModelProject, baseModelProject );
			idPlane frustumPlanes[6];
			idRenderMatrix::GetFrustumPlanes( frustumPlanes, baseModelProject, false, true );

			// exact clip of light faces against all planes
			for ( int i = 0; i < 6; i++ ) {
				// the entity frustum planes face inward, so the planes that have the
				// view origin on the positive side will be the "back" faces of the entity,
				// which must have some fragment inside the portal stack planes to be visible
				if ( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f ) {
					continue;
				}

				// calculate a winding for this frustum side
				idFixedWinding w;
				w.BaseForPlane( frustumPlanes[i] );
				for ( int j = 0; j < 6; j++ ) {
					if ( j == i ) {
						continue;
					}
					if ( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) ) {
						break;
					}
				}

				if ( w.GetNumPoints() <= 2 ) {
					continue;
				}
				assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
				assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

				// now clip the winding against each of the portalStack planes
				// skip the last plane which is the last portal itself
				for ( int j = 0; j < ps->numPortalPlanes - 1; j++ ) {
					if ( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) ) {
						break;
					}
				}

				if ( w.GetNumPoints() > 2 ) {
					// part of the winding is visible through the portalStack,
					// so the entity is not culled
					return false;
				}
			}

			// nothing was visible
			return true;

		}
	} else {
		if ( !r_useEntityCulling.GetBool() ) {
			return false;
		}

		// try to cull the entire thing using the reference bounds.
		// we do not yet do callbacks or dynamic model creation,
		// because we want to do all touching of the model after
		// we have determined all the lights that may effect it,
		// which optimizes cache usage
		if ( R_CullLocalBox( entity->referenceBounds, entity->modelMatrix, ps->numPortalPlanes, ps->portalPlanes ) ) {
			return true;
		}
	}
	return false;
}

/*
===================
AddAreaEntityRefs

Any models that are visible through the current portalStack will
have their scissor
===================
*/
void idRenderWorldLocal::AddAreaEntityRefs( int areaNum, const portalStack_t *ps ) {
	areaReference_t		*ref;
	idRenderEntityLocal	*entity;
	portalArea_t		*area;
	viewEntity_t		*vEnt;
	idBounds			b;

	area = &portalAreas[ areaNum ];

	for ( ref = area->entityRefs.areaNext ; ref != &area->entityRefs ; ref = ref->areaNext ) {
		entity = ref->entity;

		// debug tool to allow viewing of only one entity at a time
		if ( r_singleEntity.GetInteger() >= 0 && r_singleEntity.GetInteger() != entity->index ) {
			continue;
		}

		// remove decals that are completely faded away
		R_FreeEntityDefFadedDecals( entity, tr.viewDef->renderView.time );

		// check for completely suppressing the model
		if ( !r_skipSuppress.GetBool() ) {
			// nbohr1more: #4379 lightgem culling
			if ( !entity->parms.isLightgem && entity->parms.noShadow && tr.viewDef->IsLightGem() ) { 
				continue; 
			}
			if ( entity->parms.suppressSurfaceInViewID && entity->parms.suppressSurfaceInViewID == tr.viewDef->renderView.viewID ) {
				continue;
			}
			if ( entity->parms.allowSurfaceInViewID && entity->parms.allowSurfaceInViewID != tr.viewDef->renderView.viewID ) {
				continue;
			}
		}

		// cull reference bounds
		if ( CullEntityByPortals( entity, ps ) ) {
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}
		vEnt = R_SetEntityDefViewEntity( entity );

		// possibly expand the scissor rect
		vEnt->scissorRect.Union( ps->rect );
	}
}

/*
================
CullLightByPortals

Return true if the light frustum does not intersect the current portal chain.
The last stack plane is not used because lights are not near clipped.
================
*/
bool idRenderWorldLocal::CullLightByPortals( const idRenderLightLocal *light, const portalStack_t *ps ) {
	if ( r_useAnonreclaimer.GetBool() ) {
		if ( r_useLightPortalCulling.GetInteger() == 1 ) {

			ALIGNTYPE16 frustumCorners_t corners;
			idRenderMatrix::GetFrustumCorners( corners, light->inverseBaseLightProject, bounds_zeroOneCube );
			for ( int i = 0; i < ps->numPortalPlanes; i++ ) {
				if ( idRenderMatrix::CullFrustumCornersToPlane( corners, ps->portalPlanes[i] ) == FRUSTUM_CULL_FRONT ) {
					return true;
				}
			}
		} else if ( r_useLightPortalCulling.GetInteger() >= 2 ) {

			idPlane frustumPlanes[6];
			idRenderMatrix::GetFrustumPlanes( frustumPlanes, light->baseLightProject, true, true );

			// exact clip of light faces against all planes
			for ( int i = 0; i < 6; i++ ) {
				// the light frustum planes face inward, so the planes that have the
				// view origin on the positive side will be the "back" faces of the light,
				// which must have some fragment inside the the portal stack planes to be visible
				if ( frustumPlanes[i].Distance( tr.viewDef->renderView.vieworg ) <= 0.0f ) {
					continue;
				}

				// calculate a winding for this frustum side
				idFixedWinding w;
				w.BaseForPlane( frustumPlanes[i] );
				for ( int j = 0; j < 6; j++ ) {
					if ( j == i ) {
						continue;
					}
					if ( !w.ClipInPlace( frustumPlanes[j], ON_EPSILON ) ) {
						break;
					}
				}

				if ( w.GetNumPoints() <= 2 ) {
					continue;
				}
				assert( ps->numPortalPlanes <= MAX_PORTAL_PLANES );
				assert( w.GetNumPoints() + ps->numPortalPlanes < MAX_POINTS_ON_WINDING );

				// now clip the winding against each of the portalStack planes
				// skip the last plane which is the last portal itself
				for ( int j = 0; j < ps->numPortalPlanes - 1; j++ ) {
					if ( !w.ClipInPlace( -ps->portalPlanes[j], ON_EPSILON ) ) {
						break;
					}
				}

				if ( w.GetNumPoints() > 2 ) {
					// part of the winding is visible through the portalStack,
					// so the light is not culled
					return false;
				}
			}

			// nothing was visible
			return true;
		}
	} else {
		int						i, j;
		const srfTriangles_t	*tri;
		float					d;
		idFixedWinding			w;		// we won't overflow because MAX_PORTAL_PLANES = 20

		if ( r_useLightCulling.GetInteger() == 0 ) {
			return false;
		}

		if ( r_useLightCulling.GetInteger() >= 2 ) {
			// exact clip of light faces against all planes
			for ( i = 0; i < 6; i++ ) {
				// the light frustum planes face out from the light,
				// so the planes that have the view origin on the negative
				// side will be the "back" faces of the light, which must have
				// some fragment inside the portalStack to be visible
				if ( light->frustum[i].Distance( tr.viewDef->renderView.vieworg ) >= 0 ) {
					continue;
				}

				// get the exact winding for this side
				const idWinding &ow = light->frustumWindings[i];

				// projected lights may have one of the frustums degenerated
				if ( !ow.GetNumPoints() ) {
					continue;
				}
				w = ow;

				// now check the winding against each of the portalStack planes
				for ( j = 0; j < ps->numPortalPlanes - 1; j++ ) {
					if ( !w.ClipInPlace( -ps->portalPlanes[j] ) ) {
						break;
					}
				}

				if ( w.GetNumPoints() ) {
					// part of the winding is visible through the portalStack,
					// so the light is not culled
					return false;
				}
			}
			// none of the light surfaces were visible
			return true;
		} else {

			// simple point check against each plane
			tri = light->frustumTris;

			// check against frustum planes
			for ( i = 0; i < ps->numPortalPlanes - 1; i++ ) {
				for ( j = 0; j < tri->numVerts; j++ ) {
					d = ps->portalPlanes[i].Distance( tri->verts[j].xyz );
					if ( d < 0.0f ) {
						break;	// point is inside this plane
					}
				}

				if ( j == tri->numVerts ) {
					// all points were outside one of the planes
					tr.pc.c_box_cull_out++;
					return true;
				}
			}
		}
	}
	return false;
}

/*
===================
AddAreaLightRefs

This is the only point where lights get added to the viewLights list
===================
*/
void idRenderWorldLocal::AddAreaLightRefs( int areaNum, const portalStack_t *ps ) {
	areaReference_t		*lref;
	portalArea_t		*area;
	idRenderLightLocal	*light;
	viewLight_t			*vLight;

	area = &portalAreas[ areaNum ];

	for ( lref = area->lightRefs.areaNext ; lref != &area->lightRefs ; lref = lref->areaNext ) {
		light = lref->light;


		// debug tool to allow viewing of only one light at a time
		if ( r_singleLight.GetInteger() >= 0 && r_singleLight.GetInteger() != light->index ) {
			continue;
		}

		// check for being closed off behind a door
		// a light that doesn't cast shadows will still light even if it is behind a door
		if ( r_useAnonreclaimer.GetBool() ) {
			if ( r_useLightAreaCulling.GetInteger() &&
			    !light->parms.noShadows && light->lightShader->LightCastsShadows() && 
				light->areaNum != -1 && !tr.viewDef->connectedAreas[light->areaNum] ) {
				continue;
			}
		} else {
			if ( r_useLightCulling.GetInteger() >= 3 &&
			    !light->parms.noShadows && light->lightShader->LightCastsShadows() && 
				light->areaNum != -1 && !tr.viewDef->connectedAreas[light->areaNum] ) {
				continue;
			}
		}

		// cull frustum
		if ( CullLightByPortals( light, ps ) ) {
			// we are culled out through this portal chain, but it might
			// still be visible through others
			continue;
		}

		vLight = R_SetLightDefViewLight( light );

		// expand the scissor rect
		vLight->scissorRect.Union( ps->rect );
	}
}

/*
===================
AddAreaRefs

This may be entered multiple times with different planes
if more than one portal sees into the area
===================
*/
void idRenderWorldLocal::AddAreaRefs( int areaNum, const portalStack_t *ps ) {
	// mark the viewCount, so r_showPortals can display the
	// considered portals
	portalAreas[ areaNum ].areaViewCount = tr.viewCount;

	// add the models and lights, using more precise culling to the planes
	AddAreaEntityRefs( areaNum, ps );
	AddAreaLightRefs( areaNum, ps );
}

/*
===================
BuildConnectedAreas_r
===================
*/
void idRenderWorldLocal::BuildConnectedAreas_r( int areaNum ) {
	portalArea_t	*area;
	//portal_t		*portal;

	if ( tr.viewDef->connectedAreas[areaNum] ) {
		return;
	}
	tr.viewDef->connectedAreas[areaNum] = true;

	// flood through all non-blocked portals
	area = &portalAreas[ areaNum ];
	for ( auto const &portal : area->areaPortals ) {
		if ( !( portal->doublePortal->blockingBits & PS_BLOCK_VIEW ) ) {
			BuildConnectedAreas_r( portal->intoArea );
		}
	}
}

/*
===================
BuildConnectedAreas

This is only valid for a given view, not all views in a frame
===================
*/
void idRenderWorldLocal::BuildConnectedAreas( void ) {
	int		i;

	tr.viewDef->connectedAreas = ( bool * )R_FrameAlloc( ( int )portalAreas.size()
	                             * sizeof( tr.viewDef->connectedAreas[0] ) );

	// if we are outside the world, we can see all areas
	if ( tr.viewDef->areaNum == -1 ) {
		for ( i = 0; i < ( int )portalAreas.size(); i++ ) {
			tr.viewDef->connectedAreas[i] = true;
		}
		return;
	}

	// start with none visible, and flood fill from the current area
	memset( tr.viewDef->connectedAreas, 0, portalAreas.size() * sizeof( tr.viewDef->connectedAreas[0] ) );
	BuildConnectedAreas_r( tr.viewDef->areaNum );
}

/*
=============
FindViewLightsAndEntites

All the modelrefs and lightrefs that are in visible areas
will have viewEntitys and viewLights created for them.

The scissorRects on the viewEntitys and viewLights may be empty if
they were considered, but not actually visible.
=============
*/
void idRenderWorldLocal::FindViewLightsAndEntities( void ) {
	// clear the visible lightDef and entityDef lists
	tr.viewDef->viewLights = nullptr;
	tr.viewDef->viewEntitys = nullptr;

	// find the area to start the portal flooding in
	if ( !r_usePortals.GetBool() ) {
		// debug tool to force no portal culling
		tr.viewDef->areaNum = -1;
	} else {
		tr.viewDef->areaNum = PointInArea( tr.viewDef->initialViewAreaOrigin );
	}

	// determine all possible connected areas for
	// light-behind-door culling
	BuildConnectedAreas();

	// bump the view count, invalidating all visible areas
	//tr.viewCount++; duzenko: double increment here and in R_RenderView

	// flow through all the portals and add models / lights
	if ( r_singleArea.GetBool() ) {
		// if debugging, only mark this area
		// if we are outside the world, don't draw anything
		if ( tr.viewDef->areaNum >= 0 ) {
			portalStack_t	ps;
			int				i;
			static int		lastPrintedAreaNum;

			if ( tr.viewDef->areaNum != lastPrintedAreaNum ) {
				lastPrintedAreaNum = tr.viewDef->areaNum;
				common->Printf( "entering portal area %i\n", tr.viewDef->areaNum );
			}

			for ( i = 0 ; i < 5 ; i++ ) {
				ps.portalPlanes[i] = tr.viewDef->frustum[i];
			}
			ps.numPortalPlanes = 5;
			ps.rect = tr.viewDef->scissor;

			AddAreaRefs( tr.viewDef->areaNum, &ps );
		}
	} else {
		// note that the center of projection for flowing through portals may
		// be a different point than initialViewAreaOrigin for subviews that
		// may have the viewOrigin in a solid/invalid area
		FlowViewThroughPortals( tr.viewDef->renderView.vieworg, 5, tr.viewDef->frustum );
	}
}

/*
==============
NumPortals
==============
*/
int idRenderWorldLocal::NumPortals( void ) const {
	return ( int )doublePortals.size();
}

/*
==============
FindPortal

Game code uses this to identify which portals are inside doors.
Returns 0 if no portal contacts the bounds
==============
*/
qhandle_t idRenderWorldLocal::FindPortal( const idBounds &b ) const {
	int				i, j;
	idBounds		wb;
	//doublePortal_t	*portal;
	//idWinding		*w;

	for ( i = 0; i < ( int )doublePortals.size(); i++ ) {
		auto &portal = doublePortals[i];
		auto &w = portal.portals[0].w;
		wb.Clear();
		for ( j = 0 ; j < w.GetNumPoints() ; j++ ) {
			wb.AddPoint( w[j].ToVec3() );
		}

		if ( wb.IntersectsBounds( b ) ) {
			return i + 1;
		}
	}
	return 0;
}

/*
=============
FloodConnectedAreas
=============
*/
void idRenderWorldLocal::FloodConnectedAreas( portalArea_t *area, int portalAttributeIndex ) {
	if ( area->connectedAreaNum[portalAttributeIndex] == connectedAreaNum ) {
		return;
	}
	area->connectedAreaNum[portalAttributeIndex] = connectedAreaNum;

	for ( auto p : area->areaPortals ) {
		if ( !( p->doublePortal->blockingBits & ( 1 << portalAttributeIndex ) ) ) {
			FloodConnectedAreas( &portalAreas[p->intoArea], portalAttributeIndex );
		}
	}
}

/*
==============
AreasAreConnected

==============
*/
bool idRenderWorldLocal::AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) {
	if ( areaNum1 == -1 || areaNum2 == -1 ) {
		return false;
	}

	if ( areaNum1 > ( int )portalAreas.size() || areaNum2 > ( int )portalAreas.size() || areaNum1 < 0 || areaNum2 < 0 ) {
		common->Error( "idRenderWorldLocal::AreAreasConnected: bad parms: %i, %i", areaNum1, areaNum2 );
	}
	int	attribute = 0;
	int	intConnection = ( int )connection;

	while ( intConnection > 1 ) {
		attribute++;
		intConnection >>= 1;
	}

	if ( attribute >= NUM_PORTAL_ATTRIBUTES || ( 1 << attribute ) != ( int )connection ) {
		common->Error( "idRenderWorldLocal::AreasAreConnected: bad connection number: %i\n", ( int )connection );
	}
	return portalAreas[areaNum1].connectedAreaNum[attribute] == portalAreas[areaNum2].connectedAreaNum[attribute];
}


/*
==============
SetPortalState

doors explicitly close off portals when shut
==============
*/
void idRenderWorldLocal::SetPortalState( qhandle_t portal, int blockTypes ) {
	if ( portal == 0 ) {
		return;
	}

	if ( portal < 1 || portal > ( int )doublePortals.size() ) {
		common->Error( "SetPortalState: bad portal number %i", portal );
	}
	int	old = doublePortals[portal - 1].blockingBits;

	if ( old == blockTypes ) {
		return;
	}
	doublePortals[portal - 1].blockingBits = blockTypes;

	// leave the connectedAreaGroup the same on one side,
	// then flood fill from the other side with a new number for each changed attribute
	for ( int i = 0 ; i < NUM_PORTAL_ATTRIBUTES ; i++ ) {
		if ( ( old ^ blockTypes ) & ( 1 << i ) ) {
			connectedAreaNum++;
			FloodConnectedAreas( &portalAreas[doublePortals[portal - 1].portals[1].intoArea], i );
		}
	}

	if ( session->writeDemo ) {
		session->writeDemo->WriteInt( DS_RENDER );
		session->writeDemo->WriteInt( DC_SET_PORTAL_STATE );
		session->writeDemo->WriteInt( portal );
		session->writeDemo->WriteInt( blockTypes );
	}
}

/*
==============
GetPortalState
==============
*/
int		idRenderWorldLocal::GetPortalState( qhandle_t portal ) {
	if ( portal == 0 ) {
		return 0;
	}

	if ( portal < 1 || portal > ( int )doublePortals.size() ) {
		common->Error( "GetPortalState: bad portal number %i", portal );
	}
	return doublePortals[portal - 1].blockingBits;
}

/*
=====================
idRenderWorldLocal::ShowPortals

Debugging tool, won't work correctly with SMP or when mirrors are present
=====================
*/
void idRenderWorldLocal::ShowPortals() {
	int	j;
	int viewCount = tr.viewCount; // 4659 - subviews may have screwed this, try harder
	if ( backEnd.vLight && backEnd.vLight->lightDef ) { 
		viewCount = backEnd.vLight->lightDef->viewCount; 
	}
	// Check viewcount on areas and portals to see which got used this frame.
	for ( auto &area : portalAreas ) {
		if ( area.areaViewCount != viewCount ) { 
			continue; 
		}
		for ( auto p : area.areaPortals ) {
			// Changed to show 3 colours. -- SteveL #4162
			if ( p->doublePortal->portalViewCount == viewCount ) { 
				qglColor3f( 0, 1, 0 ); 	// green = we see through this portal
			}
			else if ( portalAreas[p->intoArea].areaViewCount == viewCount )	{ 
				qglColor3f( 1, 1, 0 );	// yellow = we see into this visleaf but not through this portal
			} else { 
				qglColor3f( 1, 0, 0 ); 	// red = can't see
			}
			qglBegin( GL_LINE_LOOP );
			for ( j = 0; j < p->w.GetNumPoints(); j++ )	{ 
				qglVertex3fv( p->w[j].ToFloatPtr() ); 
			}
			qglEnd();
		}
	}
}
