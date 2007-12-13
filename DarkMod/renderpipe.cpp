/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1869 $
 * $Date: 2007-12-13 12:45:27 +0100 (Do, 13 Dez 2007) $
 * $Author: crispy $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "renderpipe.h"

// We need a definition for the pure virtual destructor.
// Sounds silly, but it is required, and for good underlying reasons.
CRenderPipe::~CRenderPipe() {}
