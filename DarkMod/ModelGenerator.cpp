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
		// get a pointer to the surface
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

/*
===============
	Add the source model to the target model.
===============
*/
model_combineinfo_t	CModelGenerator::CombineModels( const idRenderModel *source, const idVec3 *ofs, const idAngles *angles, idRenderModel *target )
{
	model_combineinfo_t info;

	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf, *targetSurf;
	modelSurface_s newSurf;

	// get the number of base surfaces (minus decals) on the old model
	numSurfaces = source->NumBaseSurfaces();

	// count the tris
	numIndexes = 0; numVerts = 0;
	// for each surface
	for (int i = 0; i < numSurfaces; i++)
	{
		// get a pointer to the surface
		surf = source->Surface( i );
		if (surf)
		{
			numVerts += surf->geometry->numVerts; 
			numIndexes += surf->geometry->numIndexes;

			// find out if the material on the old surface already exists on the target
			int numTargetSurfaces = target->NumBaseSurfaces();
			bool found = false;
			for (int j = 0; j < numTargetSurfaces; j++)
			{
				// get a pointer to a surface
				targetSurf = target->Surface( j );
				if (targetSurf->shader == surf->shader)
				{
					found = true;
					break;
				}
			}

			info.numVerts = numVerts;
			info.numIndexes = numIndexes;

			if (!found)
			{
				gameLocal.Printf("ModelGenerator: Found no existing surface with shader.\n");
				// not found, allocate a new surface
				// copy the material
				newSurf.shader = surf->shader;
				// will only copy the source
				newSurf.geometry = target->AllocSurfaceTriangles( numVerts, numIndexes );
				info.firstVert = 0;
				info.firstIndex = 0;
			}
			else
			{
				gameLocal.Printf("ModelGenerator: Found existing surface, copy %i verts and %i indexes.\n", 
						targetSurf->geometry->numVerts, targetSurf->geometry->numIndexes);
				// found, need to append the data, so make a copy from target first
				newSurf.geometry = target->AllocSurfaceTriangles( numVerts + targetSurf->geometry->numVerts, numIndexes + targetSurf->geometry->numIndexes );

				// copy the old data over
				for (int j = 0; j < targetSurf->geometry->numVerts; j++)
				{
					newSurf.geometry->verts[j] = targetSurf->geometry->verts[j];
				}
				for (int j = 0; j < targetSurf->geometry->numIndexes; j++)
				{
					newSurf.geometry->indexes[j] = targetSurf->geometry->indexes[j];
				}
				// append the new data
				info.firstVert = targetSurf->geometry->numVerts;
				info.firstIndex = targetSurf->geometry->numIndexes;

				// free the old target surface
				//target->FreeSurfaceTriangles( targetSurf->geometry );

				// and swap in the new, appended version
				// not the most elegant code, but it works...
				//targetSurf->geometry = NULL; //newSurf.geometry;
			}

			// now copy the source data over, but translate/rotate it at the same time
			for (int j = 0; j < numVerts; j++)
			{
				newSurf.geometry->verts[j + info.firstVert ] = surf->geometry->verts[j];
				idDrawVert *v = &newSurf.geometry->verts[j + info.firstVert ];
				//v->Clear();
				v->xyz += *ofs;
				//v->st[0] = winding[0].s;
				//v->st[1] = winding[0].t;
				//v->normal = tangents[0];
				//v->tangents[0] = tangents[1];
				//v->tangents[1] = tangents[2];
				//v->SetColor( packedColor );
			}
			// we might need to shift the indexes, so add info.firstIndex
			for (int j = 0; j < numIndexes; j++)
			{
				newSurf.geometry->indexes[j + info.firstIndex] = surf->geometry->indexes[j] + info.firstIndex;
			}

			// copy the bounds, too
			// TODO: calculate the new bounds
			newSurf.geometry->bounds[0] = surf->geometry->bounds[0];
			newSurf.geometry->bounds[1] = surf->geometry->bounds[1];
			
			//if (!found)
			{
				// was not found, need to create the new surface
				newSurf.id = 0;
				// TODO: find out what happens if we add a surface with id != 0 and the same id again,
				// does it replace an already existing surface? So far it seems we can only add surfaces
				// but not delete/replace them...
				target->AddSurface( newSurf );
			}
		}
	}

	// TODO: copy the bounds, too
	return info;
}

/*
===============
	Given the info CombineModels(), sep. the given model out again.
===============
*/
void CModelGenerator::RemoveModel( const idRenderModel *source, const model_combineinfo_t *info)
{
	return;
}

