/******************************************************************************/
/*                                                                            */
/*         DarkModGlobals (C) by Gerhard W. Gruber in Germany 2005            */
/*                          All rights reserved                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************
 *
 * PROJECT: DarkMod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 * $Name$
 *
 * $Log$
 * Revision 1.1  2005/03/21 23:10:19  sparhawk
 * Intersection code for ellipsoids, triangles and planes.
 *
 ******************************************************************************/

/**
 * This file contains functions to calculate intersections between various 
 * objects.
 */

#ifndef INTERSECTION_H
#define INTERSECTION_H

typedef enum {
	INTERSECT_NONE = 0,			// Object is contained fully
	INTERSECT_PARTIAL = 1,		// One intersection
	INTERSECT_FULL = 2,			// Both endpoints are outside but the line is passing through.
	INTERSECT_OUTSIDE = 3,		// Segment is outside the object
	INTERSECT_COUNT
} EIntersection;

typedef enum {
	ELL_ORIGIN,			// Origin has to stay on the same index as for ECONE_AREA
	ELA_CENTER,
	ELA_AXIS,
	ELL_COUNT
} ELLIPSOID_AREA;

typedef enum {
	ELC_ORIGIN,			// Origin has to stay on the same index as for ELLIPSOID_AREA
	ELA_TARGET,			// Direction where the light is pointing to (middle axis)
	ELA_RIGHT,			// radius of the cone in the left/right direction
	ELA_UP,				// radius of the cone forward/backward direction
	ELA_START,			// Offset from the origin
	ELA_END,			// Where the cone should end.
	ELC_COUNT
} ECONE_AREA;

typedef enum {
	LSG_ORIGIN,
	LSG_DIRECTION,
	LSG_COUNT
} ELINESEGMENT;

/**
 * IntersectEllipsoid calculates the intersection points of a
 * linesegment with an ellipsoid. 
 * There are four possible scenarios considered:
 *
 * a) The line is fully outside => INTERSECT_OUTSIDE
 *
 * b) One of the points is outside => INTERSECT_PARTIAL, the intersecting point is returned
 *
 * c) Both points are inside the ellipsoid => INTERSECT_NONE
 *
 * d) Both endpoints are outside, but the line is intersecting the ellipsoid => INTERSECT_FULL both intersections are returned.
 *
 * e) The line is only touching the ellipsiod. This is treated like a.
 *
 * Linesegment contains the origin of the line at [0] and the direction vector at [1].
 * Ellipsoid contains the information about the elliposid using EELIPSOID as indices.
 * Intersect will return either 0, 1 or 2 intersection points depending on the return value.
 * Inside [LSG_ORIGIN] is set to true if the origin point is inside the ellipsoid and
 * Inside[LSG_DIRECTION] is set to tru if the endpoint is inside. Depending on the results
 * either none, one or both of them can be set. Depending on the actual positioning it is also
 * possible that Inside[0] = false while Inside[1] is true. So you can not rely on [0] being false
 * means both are false as they are independent points.
 */
EIntersection IntersectLinesegmentEllipsoid(const idVec3 LineSegment[LSG_COUNT], const idVec3 Ellipsoid[ELL_COUNT], idVec3 Intersect[2], bool Inside[LSG_COUNT]);
EIntersection IntersectLineEllipsoid(const idVec3 Line[LSG_COUNT], const idVec3 Ellipsoid[ELL_COUNT], idVec3 Intersect[2]);
EIntersection IntersectRayEllipsoid(const idVec3 Ray[LSG_COUNT], const idVec3 Ellipsoid[ELL_COUNT], idVec3 Intersect[2], bool Inside[LSG_COUNT]);

EIntersection IntersectLineCone(const idVec3 rkLine[LSG_COUNT], idVec3 rkCone[ELC_COUNT], idVec3 akPoint[2], bool Stump);
bool LineSegTriangleIntersect(const idVec3 Seg[LSG_COUNT], idVec3 Triangle[3], idVec3 &Intersect, float &t);

void R_SetLightFrustum(const idPlane lightProject[4], idPlane frustum[6]);
void R_SetLightProject(idPlane lightProject[4], const idVec3 &origin, const idVec3 &target, idVec3 &right, idVec3 &up, const idVec3 &start, const idVec3 &stop);

#endif
