/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 * $Log$
 * Revision 1.1  2004/10/30 15:52:36  sparhawk
 * Initial revision
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __MATH_SIMD_ALTIVEC_H__
#define __MATH_SIMD_ALTIVEC_H__

/*
===============================================================================

	AltiVec implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_AltiVec : public idSIMD_Generic {
#ifdef MACOS_X
public:
	virtual const char * VPCALL GetName( void ) const;

#endif
};

#endif /* !__MATH_SIMD_ALTIVEC_H__ */
