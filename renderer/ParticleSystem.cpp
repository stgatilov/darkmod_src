#include "precompiled.h"
#include "ParticleSystem.h"


#define PIN(type) const type &
#define POUT(type) type &
#define PINOUT(type) type &

#define SinCos16 idMath::SinCos16
#define PI idMath::PI
#define clamp(v, l, r) idMath::ClampFloat(l, r, v)
#define length(v) (v).Length()
#define normalize(v) ((v) / (v).Length())
#define cross(a, b) (a).Cross(b)
#define transpose(m) (m).Transpose()

//particle-specific
#define EMITTER idDrawVert* &
struct idParticleDrawVert;
static void idParticle_EmitQuad(
	PIN(idParticleDrawVert) v0, PIN(idParticleDrawVert) v1, PIN(idParticleDrawVert) v2, PIN(idParticleDrawVert) v3,
	EMITTER emitter
);

#include "ParticleSystem_def.h"

static void idParticle_EmitQuad(
	PIN(idParticleDrawVert) v0, PIN(idParticleDrawVert) v1, PIN(idParticleDrawVert) v2, PIN(idParticleDrawVert) v3,
	EMITTER emitter
) {
	#define EMIT_VERTEX(v) \
		emitter->xyz = v.xyz; \
		emitter->st = v.st; \
		emitter->color[0] = idMath::Ftoi(v.color[0] * 255.0f); \
		emitter->color[1] = idMath::Ftoi(v.color[1] * 255.0f); \
		emitter->color[2] = idMath::Ftoi(v.color[2] * 255.0f); \
		emitter->color[3] = idMath::Ftoi(v.color[3] * 255.0f); \
		emitter++;
	EMIT_VERTEX(v0);
	EMIT_VERTEX(v1);
	EMIT_VERTEX(v2);
	EMIT_VERTEX(v3);
}

//===========================================================================

idBounds idParticle_EstimateBoundsStdSys(const idPartStageData &stg) {
	idBounds res;
	res.Clear();

	// this isn't absolutely guaranteed, but it should be close

	idPartSysData psys;
	idParticleData part;
	part.origin.Zero();
	part.axis = mat3_identity;

	idRandom steppingRandom;
	steppingRandom.SetSeed( 0 );

	// just step through a lot of possible particles as a representative sampling
	for ( int i = 0 ; i < 1000 ; i++ ) {
		steppingRandom.RandomInt();	//step to next seed
		part.randomSeed = steppingRandom.GetSeed();

		int	maxMsec = stg.particleLife * 1000;

		// SteveL #4218: Speed up load time for long-lived particles.
		// Limit the sampling to 250 spread across the particle's lifetime.
		const int step_milliseconds = idMath::Imax(maxMsec / 250, 16); // 16 was the original value, meaning test every frame

		for ( int inCycleTime = 0 ; inCycleTime < maxMsec ; inCycleTime += step_milliseconds )
		{
			// make sure we get the very last tic, which may make up an extreme edge
			if ( inCycleTime + step_milliseconds > maxMsec ) {
				inCycleTime = maxMsec - 1;
			}
			part.frac = (float)inCycleTime / ( stg.particleLife * 1000 );

			int random = part.randomSeed;
			idVec3 origin = idParticle_ParticleOriginStdSys(stg, part, random);

			res.AddPoint(origin);
		}
	}

	// find the max size
	float	maxSize = 0;

	for ( float f = 0; f <= 1.0f; f += 1.0f / 64 ) {
		float size = idParticleParm_Eval( stg.size, f );
		float aspect = idParticleParm_Eval( stg.aspect, f );
		if ( aspect > 1 ) {
			size *= aspect;
		}
		if ( size > maxSize ) {
			maxSize = size;
		}
	}

	maxSize += 8;	// just for good measure
	// users can specify a per-stage bounds expansion to handle odd cases
	res.ExpandSelf( maxSize + stg.boundsExpansion );

	return res;
}

idBounds idParticle_GetStageBoundsModel(const idPartStageData &stg, const idBounds &stdBounds, const idMat3 &entityAxis) {
	//we have bounding box for idParticle_ParticleOriginStdSys positions
	idBounds bounds = stdBounds;
	//now we need to apply the remaining modifications from idParticle_ParticleOrigin
	//note: part.origin and part.axis are trivial (they are used only in particle deform)

	//apply transformation
	if ( stg.worldAxis ) { // SteveL #3950 -- allow particles to use world axis for their offset and travel direction
		bounds *= entityAxis.Transpose();
	}

	if (stg.gravity) {
		// add gravity after adjusting for axis
		idVec3 grav = idVec3( 0, 0, -stg.gravity );
		if ( stg.worldGravity ) {
			grav *= entityAxis.Transpose();
		}
		float age = stg.particleLife;
		idVec3 gravityShift = grav * age * age;
		bounds.AddBounds(bounds.Translate(gravityShift));
	}

	return bounds;
}

void idParticle_AnalyzeSurfaceEmitter(srfTriangles_t *tri, idBounds csysBounds[4]) {
	//get bounds for every axis of particle's coordinate system
	//note that each particle gets some interpolation of normal/tangents and xyz as its coordinate system
	for (int l = 0; l < 4; l++)
		csysBounds[l].Clear();
	for (int v = 0; v < tri->numVerts; v++) {
		csysBounds[0].AddPoint(tri->verts[v].tangents[0]);
		csysBounds[1].AddPoint(tri->verts[v].tangents[1]);
		csysBounds[2].AddPoint(tri->verts[v].normal);
		csysBounds[3].AddPoint(tri->verts[v].xyz);
	}
}

idBounds idParticle_GetStageBoundsDeform(const idPartStageData &stg, const idBounds &stdBounds, const idBounds csysBounds[4]) {
	//compute bounds of particle movement
	float radius = stdBounds.GetRadius(vec3_zero);
	//these are pessimistic bounds, considering orientation of movement to be arbitrary
	idBounds bounds = bounds_zero.Expand(radius);

	if (!stg.worldAxis) {
		//movement is in particle space (not in world space)
		//so we can estimate bounds much better
		idBounds sum = bounds_zero;
		for (int l = 0; l < 3; l++) {
			//consider possible shift from l-th local coordinates
			float minCoeff = stdBounds[0][l];
			float maxCoeff = stdBounds[1][l];
			idBounds scaled;
			scaled.Clear();
			//use bounds on particle's axis
			scaled.AddPoint(minCoeff * csysBounds[l][0]);
			scaled.AddPoint(maxCoeff * csysBounds[l][0]);
			scaled.AddPoint(minCoeff * csysBounds[l][1]);
			scaled.AddPoint(maxCoeff * csysBounds[l][1]);
			//add the shift to total sum
			sum[0] += scaled[0];
			sum[1] += scaled[1];
		}
		//we got another bounds on movement, so take their intersection s final estimate
		bounds.IntersectSelf(sum);
	}
	//add bounds of particle origin to movement bounds
	bounds[0] += csysBounds[3][0];
	bounds[1] += csysBounds[3][1];

	float maxGravityMovement = stg.gravity * stg.particleLife * stg.particleLife;
	if (stg.worldGravity) {
		//cannot predict how entity would be oriented, so expand in all directions
		bounds.ExpandSelf(maxGravityMovement);
	}
	else {
		//apply shift from gravity along Z
		bounds.AddBounds(bounds.Translate(idVec3(0.0f, 0.0f, -maxGravityMovement)));
	}

	return bounds;
}
