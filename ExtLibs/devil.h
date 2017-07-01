#include <IL/il.h>

#include "Export.h"

namespace ExtLibs {
	EXTLIB ILvoid    ILAPIENTRY ilInit( void );
	EXTLIB ILboolean ILAPIENTRY ilLoadL( ILenum Type, const ILvoid *Lump, ILuint Size );
}