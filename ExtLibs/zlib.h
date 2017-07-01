#ifndef _ZLIB_H
extern "C" {
#include <zlib.h>
}
#endif

#include "Export.h"

namespace ExtLibs {
	EXTLIB ZEXTERN int ZEXPORT inflate OF( (z_streamp strm, int flush) );
	EXTLIB ZEXTERN int ZEXPORT inflateEnd OF( (z_streamp strm) );
	EXTLIB ZEXTERN uLong ZEXPORT crc32   OF( (uLong crc, const Bytef *buf, uInt len) );
	EXTLIB ZEXTERN int ZEXPORT inflateInit2_ OF( (z_streamp strm, int  windowBits,
		const char *version, int stream_size) );
	EXTLIB ZEXTERN int ZEXPORT deflateInit2_ OF( (z_streamp strm, int  level, int  method,
		int windowBits, int memLevel,
		int strategy, const char *version,
		int stream_size) );
	EXTLIB ZEXTERN int ZEXPORT deflate OF( (z_streamp strm, int flush) );
	EXTLIB ZEXTERN int ZEXPORT deflateEnd OF( (z_streamp strm) );
	EXTLIB ZEXTERN int ZEXPORT compress OF( (Bytef *dest, uLongf *destLen,
		const Bytef *source, uLong sourceLen) );
	EXTLIB ZEXTERN uLong ZEXPORT compressBound OF( (uLong sourceLen) );
	EXTLIB ZEXTERN int ZEXPORT uncompress OF( (Bytef *dest, uLongf *destLen,
		const Bytef *source, uLong sourceLen) );

}