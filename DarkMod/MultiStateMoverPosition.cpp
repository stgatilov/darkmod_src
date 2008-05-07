/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 2167 $
 * $Date: 2008-04-06 20:41:22 +0200 (So, 06 Apr 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: MultiStateMoverPosition.cpp 2167 2008-04-06 18:41:22Z greebo $", init_version);

#include "MultiStateMoverPosition.h"

CLASS_DECLARATION( idEntity, CMultiStateMoverPosition )
END_CLASS

void CMultiStateMoverPosition::Spawn() 
{
	// nothing to do yet
}

void CMultiStateMoverPosition::OnMultistateMoverArrive(CMultiStateMover* mover)
{
	if (mover == NULL) return;
}

void CMultiStateMoverPosition::OnMultistateMoverLeave(CMultiStateMover* mover)
{
	if (mover == NULL) return;
}
