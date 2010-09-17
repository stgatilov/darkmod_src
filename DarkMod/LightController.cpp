// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4071 $
 * $Date: 2010-07-18 15:57:08 +0200 (Sun, 18 Jul 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod Team)

/*
	LightController

	Lights and local ambient lights are registered with the light controller.
	This object then controls the brightness of the local ambient lights depending
	on the light energy of the other lights.

	TODO: Basically everything.

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: LightController.cpp 4177 2010-09-17 16:10:00Z tels $", init_version);

#include "LightController.h"

/*
===============
CLightController::CLightController
===============
*/
CLightController::CLightController( void ) {

	// the ambient lights we control
	m_Ambients.Clear();
	// the lights we watch and use to change the ambients
	m_Lights.Clear();
	m_bActive = true;
}

/*
===============
CLightController::Save
===============
*/
void CLightController::Save( idSaveGame *savefile ) const {

	savefile->WriteBool( m_bActive );

//	savefile->WriteInt( m_iNextUpdate );
//	savefile->WriteInt( m_iUpdateTime );

	int n = m_Ambients.Num();
	savefile->WriteInt( n );
	for (int i = 0; i < n; i++ )
	{
		savefile->WriteVec3( m_Ambients[i].origin );
		savefile->WriteVec3( m_Ambients[i].color );
		savefile->WriteVec3( m_Ambients[i].target_color );
	}

	n = m_Lights.Num();
	savefile->WriteInt( n );
	for (int i = 0; i < n; i++ )
	{
		savefile->WriteVec3( m_Lights[i].origin );
		savefile->WriteVec3( m_Lights[i].color );
		savefile->WriteFloat( m_Lights[i].radius );
	}
}

/*
===============
CLightController::Restore
===============
*/
void CLightController::Restore( idRestoreGame *savefile ) {

	savefile->ReadBool( m_bActive );
//	savefile->ReadInt( m_iNextUpdate );
//	savefile->ReadInt( m_iUpdateTime );

	m_Ambients.Clear();
	int n;

	savefile->ReadInt( n );
	m_Ambients.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadVec3( m_Ambients[i].origin );
		savefile->ReadVec3( m_Ambients[i].color );
		savefile->ReadVec3( m_Ambients[i].target_color );
	}

	m_Lights.Clear();
	savefile->ReadInt( n );
	m_Lights.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadVec3( m_Ambients[i].origin );
		savefile->ReadVec3( m_Lights[i].color );
		savefile->ReadFloat( m_Lights[i].radius );
	}
}

/*
===============
CLightController::LightChanged
===============
*/
void CLightController::LightChanged( const int entityNum )
{
	if (m_bActive == false ||
		m_Ambients.Num() == 0 ||
		m_Lights.Num() == 0)
	{
		return;
	}

	// TODO: update the ambients
	return;
}

/*
===============
CLightController::RegisterLight
===============
*/
void CLightController::RegisterLight()
{
//	m_Changes.Clear();
}

/*
===============
CLightController::UnregisterLight
===============
*/
void CLightController::UnregisterLight()
{
//	m_Changes.Clear();
}

/*
===============
CLightController::RegisterAmbient
===============
*/
void CLightController::RegisterAmbient()
{
//	m_Changes.Clear();
}

/*
===============
CLightController::UnregisterAmbient
===============
*/
void CLightController::UnregisterAmbient()
{
//	m_Changes.Clear();
}


