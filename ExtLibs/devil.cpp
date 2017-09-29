#include "devil.h"

namespace ExtLibs {
	ILAPI ILvoid    ILAPIENTRY ilInit( void ) {
		return ::ilInit();
	}
	EXTLIB ILAPI ILboolean ILAPIENTRY ilLoadL( ILenum Type, const ILvoid *Lump, ILuint Size ) {
		return ::ilLoadL( Type, Lump, Size );
	}
}