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

#include "../precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

idComplex complex_origin( 0.0f, 0.0f );

/*
=============
idComplex::ToString
=============
*/
const char *idComplex::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}
