#include <IL/il.h>

#ifdef MAKEDLL
#  define EXTLIB __declspec(dllexport)
#else
#  define EXTLIB __declspec(dllimport)
#endif

namespace ExtLibs {
	EXTLIB ILvoid    ILAPIENTRY ilInit( void );
	EXTLIB ILboolean ILAPIENTRY ilLoadL( ILenum Type, const ILvoid *Lump, ILuint Size );
}