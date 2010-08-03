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
   MegaModel

   Contains a rendermodel combined from different LOD stages of the same model

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: ModelGenerator.cpp 4071 2010-07-18 13:57:08Z tels $", init_version);

#include "MegaModel.h"

/*
===============
CMegaModel::CMegaModel
===============
*/
CMegaModel::CMegaModel( idList<const idRenderModel*>* LODs, idList<model_ofs_t>* offsets, const idVec3 *playerPos, const idVec3 *origin, const int iUpdateTime ) {

	m_hModel = NULL;
	m_iUpdateTime = iUpdateTime;
	m_iNextUpdate = gameLocal.time + iUpdateTime;
	m_iMaxChanges = 25;

	m_bActive = true;

	m_Offsets.Clear();
	m_Offsets.Append( *offsets );

	m_Changes.Clear();

	m_hModel = gameLocal.m_ModelGenerator->DuplicateLODModels( LODs, "megamodel", true, offsets, origin, playerPos );
}

/*
===============
CMegaModel::Save
===============
*/
void CMegaModel::Save( idSaveGame *savefile ) const {

	savefile->WriteBool( m_bActive );
	savefile->WriteInt( m_iNextUpdate );
	savefile->WriteInt( m_iUpdateTime );

	// TODO: Wouldn't the existing entity also save the model?
	// TODO: Could we simply regenerate the model on Restore()?
	savefile->WriteModel( m_hModel );

	savefile->WriteInt( m_Offsets.Num() );
	for (int i = 0; i < m_Offsets.Num(); i++ )
	{
		savefile->WriteVec3( m_Offsets[i].offset );
		savefile->WriteAngles( m_Offsets[i].angles );
		savefile->WriteInt( m_Offsets[i].lod );
		savefile->WriteUnsignedInt( m_Offsets[i].color );
	}

	savefile->WriteInt( m_Changes.Num() );
	for (int i = 0; i < m_Changes.Num(); i++ )
	{
		savefile->WriteInt( m_Changes[i].entity );
		savefile->WriteInt( m_Changes[i].oldLOD );
		savefile->WriteInt( m_Changes[i].newLOD );
	}
}

/*
===============
CMegaModel::Restore
===============
*/
void CMegaModel::Restore( idRestoreGame *savefile ) {

	savefile->ReadBool( m_bActive );
	savefile->ReadInt( m_iNextUpdate );
	savefile->ReadInt( m_iUpdateTime );

	savefile->ReadModel( m_hModel );

	m_Offsets.Clear();
	int n;

	savefile->ReadInt( n );
	m_Offsets.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadVec3( m_Offsets[i].offset );
		savefile->ReadAngles( m_Offsets[i].angles );
		savefile->ReadInt( m_Offsets[i].lod );
		savefile->ReadUnsignedInt( m_Offsets[i].color );
	}

	m_Changes.Clear();
	savefile->ReadInt( n );
	m_Changes.SetNum(n);
	for (int i = 0; i < n; i ++)
	{
		savefile->ReadInt( m_Changes[i].entity );
		savefile->ReadInt( m_Changes[i].oldLOD );
		savefile->ReadInt( m_Changes[i].newLOD );
	}
}

/*
===============
CMegaModel::GetRenderModel
===============
*/
idRenderModel* CMegaModel::GetRenderModel() const
{
	return m_hModel;
}

/*
===============
CMegaModel::Update

Updates the rendermodel if nec. and returns true if it was updated.
===============
*/
bool CMegaModel::Update()
{
	if (m_bActive == false ||
		m_Changes.Num() == 0 ||
		(m_Changes.Num() < m_iMaxChanges && gameLocal.time < m_iNextUpdate) )
	{
		return false;
	}

	m_iNextUpdate = gameLocal.time + m_iUpdateTime;

	// TODO:
	return false;
}

/*
===============
CMegaModel::AddChange
===============
*/
void CMegaModel::AddChange( const int entity, const int newLOD ) {

	// go through our changes and see if we already have one for this entity
	int n = m_Changes.Num();

	for (int i = 0; i < n; i++)
	{
		if (m_Changes[i].entity == entity)
		{

			// TODO: if the new change changes the model back to what it already is, remove
			// the change set:
			m_Changes[i].newLOD = newLOD;
			// done
			return;
		}
	}

	// found no change set, add one
	model_changeinfo_t change;

	change.entity = entity;
	change.oldLOD = m_Offsets[ entity ].lod;
	change.newLOD = newLOD;

	m_Changes.Append( change );

	// done	
}

