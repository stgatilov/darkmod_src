/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

// #include "math.h"
#include "../DarkMod/DarkModGlobals.h"
#include "Intersection.h"

EIntersection IntersectLinesegmentEllipsoid(const idVec3 Segment[LSG_COUNT], 
								 const idVec3 Ellipsoid[ELL_COUNT],
								 idVec3 Contained[2], bool bInside[LSG_COUNT])
{
	EIntersection rc = INTERSECT_COUNT;
	float fRoot;
	float fInvA;
    float afT[2] = { 0.0, 0.0 }; // OrbWeaver: "may be used uninitialised" warning
	float riQuantity;

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
	idMat3 A(1/(Ellipsoid[ELA_AXIS].x*Ellipsoid[ELA_AXIS].x), 0, 0,
		0, 1/(Ellipsoid[ELA_AXIS].y*Ellipsoid[ELA_AXIS].y), 0,
		0, 0, 1/(Ellipsoid[ELA_AXIS].z*Ellipsoid[ELA_AXIS].z));

    idVec3 kDiff = Segment[LSG_ORIGIN] - Ellipsoid[ELL_ORIGIN];
    idVec3 kMatDir = A * Segment[LSG_DIRECTION];
    idVec3 kMatDiff = A * kDiff;
    float fA = Segment[LSG_DIRECTION]* kMatDir;
    float fB = Segment[LSG_DIRECTION] * kMatDiff;
    float fC = kDiff * kMatDiff - 1.0;
	bInside[0] = false;
	bInside[1] = false;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegStart", Segment[LSG_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegDirection", Segment[LSG_DIRECTION]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllOrigin", Ellipsoid[ELL_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllCenter", Ellipsoid[ELA_CENTER]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllAxis", Ellipsoid[ELA_AXIS]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Diff", kDiff);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDir", kMatDir);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDiff", kMatDiff);

    // no intersection if Q(t) has no real roots
    float fDiscr = fB*fB - fA*fC;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("fA: %f   fB: %f   fC: %f   fDiscr: %f\r", fA, fB, fC, fDiscr);
    if(fDiscr < 0.0)
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
		rc = INTERSECT_OUTSIDE;
        riQuantity = 0;
    }
    else if(fDiscr > 0.0)
    {
		fRoot = idMath::Sqrt64(fDiscr);
        fInvA = 1.0/fA;
        afT[0] = (-fB - fRoot)*fInvA;
        afT[1] = (-fB + fRoot)*fInvA;

		if(afT[0] > 1.0 || afT[1] < 0.0)
		{
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
			rc = INTERSECT_NONE;
            riQuantity = 0;
		}
        else if(afT[0] >= 0.0)
        {
            if(afT[1] > 1.0)
            {
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
				rc = INTERSECT_PARTIAL;
				riQuantity = 1;

                Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
            }
            else
            {
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found two intersections\r");
				rc = INTERSECT_FULL;
                riQuantity = 2;
                Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
                Contained[1] = Segment[LSG_ORIGIN]+afT[1]*Segment[LSG_DIRECTION];
            }
        }
        else  // afT[1] >= 0
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
			rc = INTERSECT_PARTIAL;
			riQuantity = 1;
			Contained[0] = Segment[LSG_ORIGIN]+afT[1]*Segment[LSG_DIRECTION];
        }
    }
    else
    {
        afT[0] = -fB/fA;
        if(0.0 <= afT[0] && afT[0] <= 1.0)
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
            riQuantity = 1;
            Contained[0] = Segment[LSG_ORIGIN]+afT[0]*Segment[LSG_DIRECTION];
        }
        else
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
			rc = INTERSECT_OUTSIDE;
			riQuantity = 0;
        }
    }

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("afT[0]: %f   afT[1]: %f\r", afT[0], afT[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[0]", Contained[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[1]", Contained[1]);

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rc: %u   riQuantity %f\r", rc, riQuantity);

	return rc;
}



EIntersection IntersectRayEllipsoid(const idVec3 Ray[LSG_COUNT], 
								 const idVec3 Ellipsoid[ELL_COUNT],
								 idVec3 Contained[2], bool bInside[LSG_COUNT])
{
	EIntersection rc = INTERSECT_COUNT;
	float fRoot;
	float fInvA;
    float afT[2] = { 0.0, 0.0 };
	float riQuantity;

	idMat3 A(1/(Ellipsoid[ELA_AXIS].x*Ellipsoid[ELA_AXIS].x), 0, 0,
		0, 1/(Ellipsoid[ELA_AXIS].y*Ellipsoid[ELA_AXIS].y), 0,
		0, 0, 1/(Ellipsoid[ELA_AXIS].z*Ellipsoid[ELA_AXIS].z));

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
    idVec3 kDiff = Ray[LSG_ORIGIN] - Ellipsoid[ELL_ORIGIN];
    idVec3 kMatDir = A * Ray[LSG_DIRECTION];
    idVec3 kMatDiff = A * kDiff;
    float fA = Ray[LSG_DIRECTION]* kMatDir;
    float fB = Ray[LSG_DIRECTION] * kMatDiff;
    float fC = kDiff * kMatDiff - 1.0;
	bInside[0] = false;
	bInside[1] = false;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegStart", Ray[LSG_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineSegDirection", Ray[LSG_DIRECTION]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllOrigin", Ellipsoid[ELL_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllCenter", Ellipsoid[ELA_CENTER]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllAxis", Ellipsoid[ELA_AXIS]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Diff", kDiff);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDir", kMatDir);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDiff", kMatDiff);

    float fDiscr = fB*fB - fA*fC;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("fA: %f   fB: %f   fC: %f   fDiscr: %f\r", fA, fB, fC, fDiscr);
    if(fDiscr < 0.0)
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
		rc = INTERSECT_OUTSIDE;
        riQuantity = 0;
    }
    else if(fDiscr > 0.0)
    {
		fRoot = idMath::Sqrt64(fDiscr);
        fInvA = 1.0/fA;
        afT[0] = (-fB - fRoot)*fInvA;
        afT[1] = (-fB + fRoot)*fInvA;

        if(afT[0] >= 0.0)
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found two intersections\r");
			rc = INTERSECT_FULL;
            riQuantity = 2;
            Contained[0] = Ray[LSG_ORIGIN] + afT[0]*Ray[LSG_DIRECTION];
            Contained[1] = Ray[LSG_ORIGIN] + afT[1]*Ray[LSG_DIRECTION];
        }
        else if(afT[1] >= 0.0)
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
			rc = INTERSECT_PARTIAL;
            riQuantity = 1;
            Contained[0] = Ray[LSG_ORIGIN] + afT[1]*Ray[LSG_DIRECTION];
        }
        else
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
			rc = INTERSECT_OUTSIDE;
            riQuantity = 0;
        }
    }
    else
    {
        afT[0] = -fB/fA;
        if(afT[0] >= 0.0)
        {
 			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
			riQuantity = 1;
            Contained[0] = Ray[LSG_ORIGIN] + afT[0]*Ray[LSG_DIRECTION];
        }
        else
        {
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
            riQuantity = 0;
        }
    }

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("afT[0]: %f   afT[1]: %f\r", afT[0], afT[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[0]", Contained[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[1]", Contained[1]);

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rc: %u   riQuantity %f\r", rc, riQuantity);

	return rc;
}


EIntersection IntersectLineEllipsoid(const idVec3 Line[LSG_COUNT], 
								 const idVec3 Ellipsoid[ELL_COUNT],
								 idVec3 Contained[2])
{
	EIntersection rc = INTERSECT_COUNT;
	float fRoot;
	float fInvA;
    float afT[2] = { 0.0, 0.0 };
	float riQuantity;

	idMat3 A(1/(Ellipsoid[ELA_AXIS].x*Ellipsoid[ELA_AXIS].x), 0, 0,
		0, 1/(Ellipsoid[ELA_AXIS].y*Ellipsoid[ELA_AXIS].y), 0,
		0, 0, 1/(Ellipsoid[ELA_AXIS].z*Ellipsoid[ELA_AXIS].z));

    // set up quadratic Q(t) = a*t^2 + 2*b*t + c
    idVec3 kDiff = Line[LSG_ORIGIN] - Ellipsoid[ELL_ORIGIN];
    idVec3 kMatDir = A * Line[LSG_DIRECTION];
    idVec3 kMatDiff = A * kDiff;
    float fA = Line[LSG_DIRECTION]* kMatDir;
    float fB = Line[LSG_DIRECTION] * kMatDiff;
    float fC = kDiff * kMatDiff - 1.0;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineStart", Line[LSG_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "LineDirection", Line[LSG_DIRECTION]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllOrigin", Ellipsoid[ELL_ORIGIN]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllCenter", Ellipsoid[ELA_CENTER]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "EllAxis", Ellipsoid[ELA_AXIS]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Diff", kDiff);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDir", kMatDir);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "MatDiff", kMatDiff);

    float fDiscr = fB*fB - fA*fC;
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("fA: %f   fB: %f   fC: %f   fDiscr: %f\r", fA, fB, fC, fDiscr);
    if(fDiscr < 0.0)
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found no intersections\r");
		rc = INTERSECT_OUTSIDE;
        riQuantity = 0;
    }
    else if(fDiscr > 0.0)
    {
		fRoot = idMath::Sqrt64(fDiscr);
        fInvA = 1.0/fA;
        riQuantity = 2;
        afT[0] = (-fB - fRoot)*fInvA;
        afT[1] = (-fB + fRoot)*fInvA;
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found two intersections\r");
		rc = INTERSECT_FULL;
        riQuantity = 2;
        Contained[0] = Line[LSG_ORIGIN] + afT[0]*Line[LSG_DIRECTION];
        Contained[1] = Line[LSG_ORIGIN] + afT[1]*Line[LSG_DIRECTION];
    }
    else
    {
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Found one intersection\r");
		rc = INTERSECT_PARTIAL;
        riQuantity = 1;
        afT[0] = -fB/fA;
        Contained[0] = Line[LSG_ORIGIN] + afT[0]*Line[LSG_DIRECTION];
    }

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("afT[0]: %f   afT[1]: %f\r", afT[0], afT[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[0]", Contained[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Contained[1]", Contained[1]);

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("rc: %u   riQuantity %f\r", rc, riQuantity);

	return rc;
}

bool LineSegTriangleIntersect(const idVec3 Seg[LSG_COUNT], idVec3 Tri[3], idVec3 &Intersect, float &t)
{
	bool rc = false;

	idVec3 e1, e2, p, s, q;
	float u, v, tmp;

	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Tri[0]", Tri[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Tri[1]", Tri[1]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Tri[2]", Tri[2]);

	e1 = Tri[1] - Tri[0];
	e2 = Tri[2] - Tri[0];
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "e1", e1);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "e2", e2);

	p = Seg[LSG_DIRECTION].Cross(e2);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "p", p);

	tmp = p * e1;			// Dotproduct

	if(tmp > -idMath::FLT_EPSILON && tmp < idMath::FLT_EPSILON)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection: tmp = %f\r", tmp);
		goto Quit;
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("tmp = %f\r", tmp);
	tmp = 1.0/tmp;
	s = Seg[LSG_ORIGIN] - Tri[0];
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "s", s);

	u = tmp * (s * p);
	if(u < 0.0 || u > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection: u = %f\r", u);
		goto Quit;
	}

	q = s.Cross(e1);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "q", q);
	v = tmp * (Seg[LSG_DIRECTION] * q);
	if(v < 0.0 || v > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection: v = %f\r", v);
		goto Quit;
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("u = %f   v = %f\r", u, v);
	if((u + v) > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection  (u + v) = %f\r", u + v);
		goto Quit;
	}

	// positive value for t > 1 means that the linesegment is below the triangle
	// while negative values mean the lineseg is above the triangle.
	t = tmp * (e2 * q);
	Intersect = Seg[LSG_ORIGIN] + (t * Seg[LSG_DIRECTION]);

/*
	if(t < 0.0 || t > 1.0)
	{
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("No triangle intersection  t = %f\r", t);
		goto Quit;
	}
*/
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Triangle intersection t = %f\r", t);

	rc = true;

Quit:
	return rc;
}


void R_SetLightProject(idPlane lightProject[4], 
					   const idVec3 &origin, 
					   const idVec3 &target,
					   idVec3 &right, 
					   idVec3 &up,
					   const idVec3 &start, 
					   const idVec3 &stop)
{
	float		dist;
	float		scale;
	float		rLen, uLen;
	idVec3		normal;
	float		ofs;
	idVec3		startGlobal;
	idVec4		targetGlobal;

	rLen = right.Normalize();
	uLen = up.Normalize();
	normal = up.Cross(right);
	normal.Normalize();

	dist = target * normal; //  - (origin * normal);
	if(dist < 0)
	{
		dist = -dist;
		normal = -normal;
	}

	scale = (0.5f * dist) / rLen;
	right *= scale;
	scale = -(0.5f * dist) / uLen;
	up *= scale;

	lightProject[2] = normal;
	lightProject[2][3] = -(origin * lightProject[2].Normal());

	lightProject[0] = right;
	lightProject[0][3] = -(origin * lightProject[0].Normal());

	lightProject[1] = up;
	lightProject[1][3] = -(origin * lightProject[1].Normal());

	// now offset to center
	targetGlobal.ToVec3() = target + origin;
	targetGlobal[3] = 1;
	ofs = 0.5f - (targetGlobal * lightProject[0].ToVec4()) / (targetGlobal * lightProject[2].ToVec4());
	lightProject[0].ToVec4() += ofs * lightProject[2].ToVec4();
	ofs = 0.5f - (targetGlobal * lightProject[1].ToVec4()) / (targetGlobal * lightProject[2].ToVec4());
	lightProject[1].ToVec4() += ofs * lightProject[2].ToVec4();

	// set the falloff vector
	normal = stop - start;
	dist = normal.Normalize();
	if (dist <= 0)
	{
		dist = 1;
	}
	lightProject[3] = normal * (1.0f / dist);
	startGlobal = start + origin;
	lightProject[3][3] = -(startGlobal * lightProject[3].Normal());
}

void R_SetLightFrustum(const idPlane lightProject[4], idPlane frustum[6])
{
	int		i;

	// we want the planes of s=0, s=q, t=0, and t=q
	frustum[0] = lightProject[0];
	frustum[1] = lightProject[1];
	frustum[2] = lightProject[2] - lightProject[0];
	frustum[3] = lightProject[2] - lightProject[1];

	// we want the planes of s=0 and s=1 for front and rear clipping planes
	frustum[4] = lightProject[3];

	frustum[5] = lightProject[3];
	frustum[5][3] -= 1.0f;
	frustum[5] = -frustum[5];

	for (i = 0 ; i < 6 ; i++)
	{
		float	l;

		frustum[i] = -frustum[i];
		l = frustum[i].Normalize();
		frustum[i][3] /= l;
	}
}


EIntersection IntersectLineCone(const idVec3 rkLine[LSG_COUNT],
								 idVec3 rkCone[ELC_COUNT],
								 idVec3 Intersect[2], bool Stump)
{
	EIntersection rc = INTERSECT_COUNT;
	int i, n, l, x;
	float t, angle;
	idPlane lightProject[4];
	idPlane frustum[6];
	int Start[6];
	int End[6];
	bool bStart, bEnd;
	bool bCalcIntersection;
	idStr txt;
	idStr format("Frustum[%u]");
	idVec3 EndPoint(rkLine[LSG_ORIGIN]+rkLine[LSG_DIRECTION]);

	R_SetLightProject(lightProject,
					   rkCone[ELC_ORIGIN],
					   rkCone[ELA_TARGET],
					   rkCone[ELA_RIGHT],
					   rkCone[ELA_UP],
					   rkCone[ELA_START],
					   rkCone[ELA_END]);
	R_SetLightFrustum(lightProject, frustum);

/*
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[0]", lightProject[0]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[1]", lightProject[1]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[2]", lightProject[2]);
	DM_LOGPLANE(LC_MATH, LT_DEBUG, "Light[3]", lightProject[3]);
*/
	n = format.Length();

	// Calculate the angle between the player and the lightvector.
	angle = rkCone[ELA_TARGET].Length() * rkLine[LSG_DIRECTION].Length();
	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Denominator: %f\r", angle);
	if(angle >= idMath::FLT_EPSILON)
	{
		angle = idMath::ACos((rkCone[ELA_TARGET] * rkLine[LSG_DIRECTION])/angle);
//		if(t > (idMath::PI/2))
//			angle = idMath::PI  - angle;

		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Angle: %f\r", angle);
	}
	else
		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Impossible line!\r");

	bCalcIntersection = false;
	l = 0;
	for(i = 0; i < 6; i++)
	{
		sprintf(txt, format, i);

		DM_LOGPLANE(LC_MATH, LT_DEBUG, txt, frustum[i]);

		Start[i] = frustum[i].Side(rkLine[LSG_ORIGIN], idMath::FLT_EPSILON);
		End[i] = frustum[i].Side(EndPoint, idMath::FLT_EPSILON);

		DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u]: Start %u   End: %u\r", i, Start[i], End[i]);

		// If the points are all on the outside there will be no intersections
		if(Start[i] == PLANESIDE_BACK || End[i] == PLANESIDE_BACK)
			bCalcIntersection = true;
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("CalcIntersection: %u\r", bCalcIntersection);
	if(bCalcIntersection == true)
	{
		DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "PlayerOrigin", rkLine[LSG_ORIGIN]);
		DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "PlayerDirection", rkLine[LSG_DIRECTION]);
		DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "Endpoint", EndPoint);

		for(i = 0; i < 6; i++)
		{
			if(frustum[i].LineIntersection(rkLine[LSG_ORIGIN], rkLine[LSG_DIRECTION], &t) == true)
			{
				DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u] intersects\r", i);
				Intersect[l] = rkLine[LSG_ORIGIN] + t*rkLine[LSG_DIRECTION];
				l++;

				if(l > 1)
					break;
			}
		}
	}

	if(l < 2)
		rc = INTERSECT_OUTSIDE;
	else
	{
		rc = INTERSECT_FULL;
		bStart = bEnd = true;
		for(i = 0; i < 6; i++)
		{
			x = frustum[i].Side(Intersect[0], idMath::FLT_EPSILON);
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u/0] intersection test returns %u\r", i, x);
			if(x != PLANESIDE_BACK)
				bStart = false;

			x = frustum[i].Side(Intersect[1], idMath::FLT_EPSILON);
			DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Frustum[%u/1] intersection test returns %u\r", i, x);
			if(x != PLANESIDE_BACK)
				bEnd = false;
		}

		if(bStart == false && bEnd == false)
			rc = INTERSECT_OUTSIDE;
	}

	DM_LOG(LC_MATH, LT_DEBUG)LOGSTRING("Intersection count = %u\r", l);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "akPoint[0]", Intersect[0]);
	DM_LOGVECTOR3(LC_MATH, LT_DEBUG, "akPoint[1]", Intersect[1]);

	return rc;
}

