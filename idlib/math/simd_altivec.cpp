/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2004/10/30 15:52:35  sparhawk
 * Initial revision
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "Simd_Generic.h"
#include "Simd_AltiVec.h"

//===============================================================
//
//	AltiVec implementation of idSIMDProcessor
//
//===============================================================

#ifdef MACOS_X

/*
============
idSIMD_AltiVec::GetName
============
*/
const char *idSIMD_AltiVec::GetName( void ) const {
	return "AltiVec";
}

#endif /* MACOS_X */
