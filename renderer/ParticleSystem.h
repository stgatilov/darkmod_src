#pragma once

#define PIN(type) const type &
#define POUT(type) type &
#define PINOUT(type) type &

//particle-specific
#define EMITTER idDrawVert* &

#include "ParticleSystem_decl.h"

#undef PIN
#undef POUT
#undef PINOUT

#undef EMITTER

//===========================================================================

//compute bounding box of particles in "standard" coordinate system --- transformation and world gravity not applied yet
//it is computed by sampling many particles with idParticle_ParticleOriginStdSys function
idBounds idParticle_EstimateBoundsStdSys(const idPartStageData &stg);

//compute bounding box of particles for smoke or particle model, with all transformations applied
//"entityAxis" must be set to renderEntity_t::axis: world-space effects like gravity depend on this
//"stdBounds" must be set to returned value of idParticle_EstimateBoundsStdSys (usually precomputed on load)
idBounds idParticle_GetStageBoundsModel(const idPartStageData &stg, const idBounds &stdBounds, const idMat3 &entityAxis);

//analyze surface (triangular mesh) which can be used as emitter with particle deform
//produces bounds for X axis, Y axis, Z axis, and origin of emitted particles (from idParticleData::axis and idParticleData::origin)
//TODO: put csysBounds into some sort of temporary structure?
void idParticle_AnalyzeSurfaceEmitter(struct srfTriangles_s *tri, idBounds csysBounds[4]);

//compute bounding box of particles emitted from surface via "particle deform"
//"csysBounds" must contain information about surface, computed by idParticle_AnalyzeSurfaceEmitter
//"stdBounds" must be set to returned value of idParticle_EstimateBoundsStdSys (usually precomputed on load)
idBounds idParticle_GetStageBoundsDeform(const idPartStageData &stg, const idBounds &stdBounds, const idBounds csysBounds[4]);
