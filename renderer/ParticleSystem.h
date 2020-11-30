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
