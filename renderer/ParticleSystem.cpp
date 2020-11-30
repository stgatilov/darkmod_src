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
