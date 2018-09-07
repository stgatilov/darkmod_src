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

#include "Game_local.h"

CLASS_DECLARATION(idEntity, idListener)
	EVENT(EV_Activate, idListener::Event_Activate)
	EVENT(EV_PostSpawn, idListener::PostSpawn)
END_CLASS

/*
=====================
idListener::Spawn
=====================
*/
void idListener::Spawn(void)
{
	// keep track during cinematics
	cinematic = true;

	// Schedule a post-spawn event to setup other spawnargs
	PostEventMS(&EV_PostSpawn, 1);
}

void idListener::PostSpawn()
{
}

/*
===============
idListener::Event_Activate
================
*/
void idListener::Event_Activate(idEntity *_activator)
{
	//gameLocal.Printf("Activating %s\n", GetName()); // grayman debug
	idPlayer* player = gameLocal.GetLocalPlayer();

	// If the current listener is this listener, turn off this listener.
	// If the current listener is NOT this listener, then switch to this listener.
	// If there's no current listener, then turn on this listener.

	idListener* currentListener = player->m_Listener.GetEntity();

	if ( currentListener )
	{
		if ( currentListener == this )
		{
			//gameLocal.Printf("turn off current listener %s\n", GetName()); // grayman debug
			player->m_Listener = NULL; // turn off listener
		}
		else
		{
			//gameLocal.Printf("1 turn on new listener %s\n", GetName()); // grayman debug
			player->m_Listener = this; // turn on listener
		}
	}
	else // no current listener
	{
		player->m_Listener = this; // turn on listener
		//gameLocal.Printf("2 turn on new listener\n", GetName()); // grayman debug
	}
}

idListener::~idListener(void)
{
}
