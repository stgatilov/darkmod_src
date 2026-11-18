/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop

// mac-port: the real implementation lives in tools/compilers/roqvq/roq.cpp,
// which needs libjpeg's private jpegint.h header (not available in Homebrew's
// jpeg-turbo). The RoQ encoder is an offline tool; stub it out so the "roq"
// console command prints a clear message instead of failing to link.

void RoQFileEncode_f( const idCmdArgs &args )
{
	common->Printf( "The RoQ video encoder tool is not available in this build (requires libjpeg private headers).\n" );
}
