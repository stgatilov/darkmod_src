#include "zlib.h"

namespace ExtLibs {
	EXTLIB ZEXTERN int ZEXPORT inflate OF( (z_streamp strm, int flush) ) {
		return ::inflate( strm, flush );
	}
	EXTLIB ZEXTERN int ZEXPORT inflateEnd OF( (z_streamp strm) ) {
		return ::inflateEnd( strm );
	}
	EXTLIB ZEXTERN uLong ZEXPORT crc32   OF( (uLong crc, const Bytef *buf, uInt len) ) {
		return ::crc32( crc, buf, len );
	}
	EXTLIB ZEXTERN int ZEXPORT inflateInit2_ OF( (z_streamp strm, int  windowBits,
		const char *version, int stream_size) ) {
		return ::inflateInit2_( strm, windowBits, version, stream_size );
	}
	EXTLIB ZEXTERN int ZEXPORT deflateInit2_ OF( (z_streamp strm, int  level, int  method,
		int windowBits, int memLevel,
		int strategy, const char *version,
		int stream_size) ) {
		return ::deflateInit2_( strm, level, method, windowBits, memLevel, strategy, version, stream_size );
	}
	EXTLIB ZEXTERN int ZEXPORT deflate OF( (z_streamp strm, int flush) ) {
		return ::deflate( strm, flush );
	}
	EXTLIB ZEXTERN int ZEXPORT deflateEnd OF( (z_streamp strm) ) {
		return ::deflateEnd( strm );
	}
	EXTLIB ZEXTERN int ZEXPORT compress OF( (Bytef *dest, uLongf *destLen,
		const Bytef *source, uLong sourceLen) ) {
		return ::compress( dest, destLen, source, sourceLen );
	}
	EXTLIB ZEXTERN uLong ZEXPORT compressBound OF( (uLong sourceLen) ) {
		return ::compressBound( sourceLen );
	}
	EXTLIB ZEXTERN int ZEXPORT uncompress OF( (Bytef *dest, uLongf *destLen,
		const Bytef *source, uLong sourceLen) ) {
		return ::uncompress( dest, destLen, source, sourceLen );
	}
}