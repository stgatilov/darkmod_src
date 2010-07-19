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
Manipulate/Generate models at run time

TODO: Find out why Duplicating a model with shared data does not work
TODO: Write a model combiner (recreate the work from Ground Zero Mod team) to
	  overcome the "10000 renderentities" limit in the D3 engine.
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: ModelGenerator.cpp 4071 2010-07-18 13:57:08Z tels $", init_version);

#include "ModelGenerator.h"

/*
   ModelGenerator

===============================================================================
*/

/*
===============
CModelGenerator::CModelGenerator
===============
*/
CModelGenerator::CModelGenerator( void ) {

}

/*
===============
CModelGenerator::Save
===============
*/
void CModelGenerator::Save( idSaveGame *savefile ) const {

}

/*
===============
CModelGenerator::Restore
===============
*/
void CModelGenerator::Restore( idRestoreGame *savefile ) {
}

/*
===============
ModelGenerator::Init
===============
*/
void CModelGenerator::Init( void ) {
}

/*
===============
CModelGenerator::Clear
===============
*/
void CModelGenerator::Clear( void ) {
}

/*
===============
CModelGenerator::Shutdown
===============
*/
void CModelGenerator::Shutdown( void ) {
}

/*
===============
CModelGenerator::DuplicateModel - Duplicate a render model
===============
*/
idRenderModel * CModelGenerator::DuplicateModel ( const idRenderModel *source, const char* snapshotName, const bool dupData ) {
	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf;
	modelSurface_s newSurf;

	// allocate memory for the model
	idRenderModel *hModel = renderModelManager->AllocModel();
	// and init it as dynamic empty model
	hModel->InitEmpty( snapshotName );

	// get the number of base surfaces (minus decals) on the old model
	numSurfaces = source->NumBaseSurfaces();

	// count the tris
	numIndexes = 0; numVerts = 0;
	// for each surface
	for (int i = 0; i < numSurfaces; i++)
	{
		// get a pointer to a surface
		surf = source->Surface( i );
		if (surf)
		{
			numVerts += surf->geometry->numVerts; 
			numIndexes += surf->geometry->numIndexes;

			// copy the material
			newSurf.shader = surf->shader;
			if (dupData)
			{
				newSurf.geometry = hModel->AllocSurfaceTriangles( numVerts, numIndexes );
				// copy the data over
				for (int j = 0; j < numVerts; j++)
				{
					newSurf.geometry->verts[j] = surf->geometry->verts[j];
				}
				for (int j = 0; j < numIndexes; j++)
				{
					newSurf.geometry->indexes[j] = surf->geometry->indexes[j];
				}
				// copy the bounds, too
                newSurf.geometry->bounds[0] = surf->geometry->bounds[0];
                newSurf.geometry->bounds[1] = surf->geometry->bounds[1];
			}
			else
			{
				// caller needs to make sure that the shared data is not deallocated twice
				newSurf.geometry = surf->geometry;
			}
			newSurf.id = 0;
			hModel->AddSurface( newSurf );
		}
	}
	// not needed, interferes actually
	//hModel->FinishSurfaces();

	// TODO: copy the bounds, too

	gameLocal.Printf ("ModelGenerator: Duplicated model for %s with %i surfaces, %i verts and %i indexes.\n", snapshotName, numSurfaces, numVerts, numIndexes );

	return hModel;
}

/*
===============
CModelGenerator::FreeSharedModelData - manipulate memory of a duplicate model so that shared data does not get freed twice
===============
*/
void CModelGenerator::FreeSharedModelData ( const idRenderModel *source )
{
	const modelSurface_t *surf;

	// get the number of base surfaces (minus decals) on the old model
	int numSurfaces = source->NumBaseSurfaces();

	// for each surface
	for (int i = 0; i < numSurfaces; i++)
	{
		// get a pointer to a surface
		surf = source->Surface( i );
		if (surf)
		{
			// null out the geometry with the shared data
			surf->geometry->numVerts = 0;
			surf->geometry->verts = NULL;
			surf->geometry->numIndexes = 0;
			surf->geometry->indexes = NULL;
		}
	}
}

