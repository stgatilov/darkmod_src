/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __LIB_H__
#define __LIB_H__
/*
===============================================================================

	idLib contains stateless support classes and concrete types. Some classes
	do have static variables, but such variables are initialized once and
	read-only after initialization (they do not maintain a modifiable state).

	The interface pointers idSys, idCommon, idCVarSystem and idFileSystem
	should be set before using idLib. The pointers stored here should not
	be used by any part of the engine except for idLib.

	The frameNumber should be continuously set to the number of the current
	frame if frame base memory logging is required.

===============================================================================
*/

#include <cstring>

class idLib {
public:
	static class idSys *		sys;
	static class idCommon *		common;
	static class idCVarSystem *	cvarSystem;
	static class idFileSystem *	fileSystem;
	static int					frameNumber;

	static void					Init( void );
	static void					ShutDown( void );

	// wrapper to idCommon functions 
	static void					Error( const char *fmt, ... );
	static void					Warning( const char *fmt, ... );
};


/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

typedef unsigned char			byte;		// 8 bits
typedef unsigned short			word;		// 16 bits
typedef unsigned int			dword;		// 32 bits
typedef unsigned int			uint;
typedef unsigned long			ulong;

typedef int						qhandle_t;

class idFile;
class idVec3;
class idVec4;

#ifndef NULL
#define NULL					((void *)0)
#endif

#ifndef BIT
#define BIT( num )				( 1 << ( num ) )
#endif

/* Bit operations */
#define BITCHK(flag,bit) (((flag)&(bit))==(bit)) /* if all the bits are set */
#define BITANY(flag,bit) ((flag)&(bit)) /* if any bit is set */
#define BITSET(flag,bit) flag|=(bit)
#define BITCLR(flag,bit) flag&=(~(bit))
#define BITFLIP(flag,bit) if (BIT( (flag),(bit) )) BITCLR( (flag),(bit) ); else BITSET( (flag),(bit) )


#define	MAX_STRING_CHARS		1024		// max length of a string

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

// basic colors
extern	idVec4 colorBlack;
extern	idVec4 colorWhite;
extern	idVec4 colorRed;
extern	idVec4 colorGreen;
extern	idVec4 colorBlue;
extern	idVec4 colorYellow;
extern	idVec4 colorMagenta;
extern	idVec4 colorCyan;
extern	idVec4 colorOrange;
extern	idVec4 colorPurple;
extern	idVec4 colorPink;
extern	idVec4 colorBrown;
extern	idVec4 colorLtGrey;
extern	idVec4 colorMdGrey;
extern	idVec4 colorDkGrey;

// packs color floats in the range [0,1] into an integer
dword	PackColor( const idVec3 &color );
void	UnpackColor( const dword color, idVec3 &unpackedColor );
dword	PackColor( const idVec4 &color );
void	UnpackColor( const dword color, idVec4 &unpackedColor );

// little/big endian conversion
short	BigShort( short l );
short	LittleShort( short l );
int		BigLong( int l );
int		LittleLong( int l );
float	BigFloat( float l );
float	LittleFloat( float l );
void	BigRevBytes( void *bp, int elsize, int elcount );
void	LittleRevBytes( void *bp, int elsize, int elcount );
void	LittleBitField( void *bp, int elsize );
void	Swap_Init( void );

bool	Swap_IsBigEndian( void );

// for base64
void	SixtetsForInt( byte *out, int src);
int		IntForSixtets( byte *in );


#ifdef _DEBUG
void AssertFailed( const char *file, int line, const char *expression );
#undef assert
#define assert( X )		if ( X ) { } else AssertFailed( __FILE__, __LINE__, #X )
#endif

class idException {
public:
	char error[MAX_STRING_CHARS];

	idException( const char *text = "" ) { strcpy( error, text ); }
};


/*
===============================================================================

	idLib headers.

===============================================================================
*/

// memory management and arrays
#include "heap.h"
#include "containers/list.h"

// math
#include "math/simd.h"
#include "math/math.h"
#include "math/random.h"
#include "math/complex.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/angles.h"
#include "math/quat.h"
#include "math/rotation.h"
#include "math/plane.h"
#include "math/pluecker.h"
#include "math/polynomial.h"
#include "math/extrapolate.h"
#include "math/interpolate.h"
#include "math/curve.h"
#include "math/ode.h"
#include "math/lcp.h"

// bounding volumes
#include "bv/sphere.h"
#include "bv/bounds.h"
#include "bv/box.h"
#include "bv/frustum.h"

// geometry
#include "geometry/drawvert.h"
#include "geometry/jointtransform.h"
#include "geometry/winding.h"
#include "geometry/winding2d.h"
#include "geometry/surface.h"
#include "geometry/surface_patch.h"
#include "geometry/surface_polytope.h"
#include "geometry/surface_sweptspline.h"
#include "geometry/tracemodel.h"

// text manipulation
#include "str.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "base64.h"
#include "cmdargs.h"

// containers
#include "containers/btree.h"
#include "containers/binsearch.h"
#include "containers/hashindex.h"
#include "containers/hashtable.h"
#include "containers/staticlist.h"
#include "containers/linklist.h"
#include "containers/hierarchy.h"
#include "containers/queue.h"
#include "containers/stack.h"
#include "containers/strlist.h"
#include "containers/strpool.h"
#include "containers/vectorset.h"
#include "containers/planeset.h"

// hashing
#include "hashing/crc8.h"
#include "hashing/crc16.h"
#include "hashing/crc32.h"
#include "hashing/honeyman.h"
#include "hashing/md4.h"
#include "hashing/md5.h"

// misc
#include "dict.h"
#include "langdict.h"
#include "bitmsg.h"
#include "mapfile.h"
#include "timer.h"

#endif	/* !__LIB_H__ */
