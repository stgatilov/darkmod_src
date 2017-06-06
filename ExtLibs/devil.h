#include <IL/il.h>

#ifdef MAKEDLL
#  define EXTLIB __declspec(dllexport)
#else
#  define EXTLIB __declspec(dllimport)
#endif

namespace ExtLibs {
	EXTLIB ILAPI ILvoid    ILAPIENTRY ilInit( void );
	EXTLIB ILAPI ILboolean ILAPIENTRY ilLoadL( ILenum Type, const ILvoid *Lump, ILuint Size );

}