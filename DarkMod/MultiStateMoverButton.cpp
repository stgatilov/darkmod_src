/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "DarkModGlobals.h"
#include "MultiStateMoverButton.h"
#include "MultiStateMover.h"

//===============================================================================
// CMultiStateMoverButton
//===============================================================================

CLASS_DECLARATION( CFrobButton, CMultiStateMoverButton )
	EVENT( EV_PostSpawn,				CMultiStateMoverButton::Event_PostSpawn)
END_CLASS

void CMultiStateMoverButton::Spawn()
{
	PostEventMS(&EV_PostSpawn, 10);
}

void CMultiStateMoverButton::Event_PostSpawn()
{
	// TODO: Send the button information to the targetted multistatemover
}
