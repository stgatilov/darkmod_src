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
   ModelGenerator

   Manipulate/Generate models at run time.

  TODO: duplicating too many verts can cause segfaults. Find out what the limit
  		is and how to tell the LODE to only combine X entities base on this.
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: ModelGenerator.cpp 4071 2010-07-18 13:57:08Z tels $", init_version);

#include "ModelGenerator.h"

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
idRenderModel * CModelGenerator::DuplicateModel (const idRenderModel* source, const char* snapshotName, bool dupData) {

	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf;
	modelSurface_s newSurf;

	if (NULL == source)
	{
		gameLocal.Error("ModelGenerator: Dup with NULL model.\n");
	}

	// allocate memory for the model
	idRenderModel *hModel = renderModelManager->AllocModel();
	// and init it as dynamic empty model
	hModel->InitEmpty( snapshotName );

	numVerts = 0;
	numIndexes = 0;
	numSurfaces = source->NumBaseSurfaces();
	// for each needed surface
	for (int i = 0; i < numSurfaces; i++)
	{
		//gameLocal.Warning("Duplicating surface %i.\n", i);
		surf = source->Surface( i );
		if (!surf)
		{
			continue;
		}

		numVerts += surf->geometry->numVerts; 
		numIndexes += surf->geometry->numIndexes;

		// copy the material
		newSurf.shader = surf->shader;
		if (dupData)
		{
			// gameLocal.Warning("Duplicating %i verts and %i indexes.\n", surf->geometry->numVerts, surf->geometry->numIndexes );
			newSurf.geometry = hModel->AllocSurfaceTriangles( numVerts, numIndexes );

			int nV = 0;		// vertexes
			int nI = 0;		// indexes

			// just one direct copy
			for (int j = 0; j < surf->geometry->numVerts; j++)
			{
				newSurf.geometry->verts[nV++] = surf->geometry->verts[j];
			}

			// copy indexes
			for (int j = 0; j < surf->geometry->numIndexes; j++)
			{
				newSurf.geometry->indexes[nI ++] = surf->geometry->indexes[j];
			}

	        newSurf.geometry->tangentsCalculated = true;
			// TODO: are these nec.?
	        newSurf.geometry->facePlanesCalculated = false;
    	    //newSurf.geometry->generateNormals = true;
        	newSurf.geometry->generateNormals = false;
			newSurf.geometry->numVerts = nV;
			newSurf.geometry->numIndexes = nI;
			// just copy bounds
			newSurf.geometry->bounds[0] = surf->geometry->bounds[0];
			newSurf.geometry->bounds[1] = surf->geometry->bounds[1];
		}
		else
		{
			// caller needs to make sure that the shared data is not deallocated twice
			newSurf.shader = surf->shader;
			newSurf.geometry = surf->geometry;
		}
		newSurf.id = 0;
		hModel->AddSurface( newSurf );

		// If this surface should create backsides, the next surface will be the automatically
		// created backsides, so skip them, as FinishSurfaces() below will recreate them:
		if (newSurf.shader->ShouldCreateBackSides())
		{
			// skip next surface
			i++;
		}
	}
	
	// generate shadow hull as well as tris for twosided materials
	hModel->FinishSurfaces();

	return hModel;
}

/*
===============
CModelGenerator::DuplicateLODModels - Duplicate a render model based on LOD stages.

If the given list of model_ofs_t is filled, the model will be copied X times, each time
offset and rotated by the given values, also filling in the right vertex color.
===============
*/
idRenderModel * CModelGenerator::DuplicateLODModels ( const idList<const idRenderModel*> *LODs,
			const char* snapshotName, const idList<model_ofs_t> *offsets, const idVec3 *playerPos, const idVec3 *origin,
			const idMaterial *shader ) {
	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf;
	modelSurface_t *newSurf;

	idList<model_ofs_t> ofs;
	model_ofs_t op;

	// f.i. model 1 with surf A and B, and model 2 with surf B,C,D would result in:
	// (model 0) (model 1)
	// 0,1,      1,2,3      in shaderIndex and 0,2 in shaderIndexStart:

	idList< int > shaderIndex;			// for each LOD model, note which source surface maps to which target surface
	idList< int > shaderIndexStart;		// where in shaderIndex starts the map for this LOD stage?

	idList< model_target_surf > targetSurfInfo;
	model_target_surf newTargetSurfInfo;
	model_target_surf* newTargetSurfInfoPtr;

	// allocate memory for the model
	idRenderModel *hModel = renderModelManager->AllocModel();
	// and init it as dynamic empty model
	hModel->InitEmpty( snapshotName );

	// count the tris
	numIndexes = 0; numVerts = 0;

	if (NULL == offsets)
	{
		gameLocal.Error("NULL offsets in DuplicateLODModels");
	}

	// If we have multiple LOD models with different surfaces, then we need to combine
	// all surfaces from all models, so first we need to build a list of needed surfaces
	// by shader name. This ensures that if LOD 0 has A and B and LOD 1 has B and C, we
	// create only three surfaces A, B and C, and not four (A, B.1, B.2 and C). This will
	// help reducing drawcalls:

	targetSurfInfo.Clear();

	int nSources = LODs->Num();
    int shaderIndexStartOfs = 0;
	for (int i = 0; i < nSources; i++)
	{
		const idRenderModel* source = LODs->Ptr()[i];
		if (NULL == source)
		{
//			gameLocal.Warning("Skipping empty LOD stage %i", i);
			continue;	// skip non-existing LOD stages
		}

		// count how many instances in offsets use this LOD model, so we can compute the
		// number of needed vertexes/indexes:
		int used = 0;
		for (int o = 0; o < offsets->Num(); o++)
		{
			op = offsets->Ptr()[o];
			if (op.lod == i)
			{
				used ++;
			}
		}

		// if not used at all, skip it
		if (used == 0)
		{
			continue;
		}

		gameLocal.Printf("ModelGenerator: LOD stage %i is used %i times.\n", i, used );

		// get the number of base surfaces (minus decals) on the source model
		numSurfaces = source->NumBaseSurfaces();

		shaderIndexStart.Append( shaderIndexStartOfs );	// where in shaderIndex starts the map for this LOD stage?

		// get each surface
		for (int i = 0; i < numSurfaces; i++)
		{
			surf = source->Surface( i );
			if (!surf)
			{
				continue;
			}

			// Do we have already a surface with that shader?

			// TODO: replace linear search with a hash for more speed in case 
			// the number of shaders gets higher than 4

			idStr n = surf->shader->GetName();
			int found = -1;
			for (int j = 0; j < targetSurfInfo.Num(); j++)
			{
				if (targetSurfInfo[j].surf.shader->GetName() == n)
				{
					found = j;
					break;
				}
			}
			if (found == -1)
			{
				newTargetSurfInfo.numVerts = 0;
				newTargetSurfInfo.numIndexes = 0;
				newTargetSurfInfo.surf.geometry = NULL;
				newTargetSurfInfo.surf.shader = surf->shader;
				newTargetSurfInfo.surf.id = 0;
				targetSurfInfo.Append( newTargetSurfInfo );
				gameLocal.Warning("ModelGenerator: Need shader %s.", n.c_str() );
				found = targetSurfInfo.Num() - 1;
			}

			// increase the nec. counts
			targetSurfInfo[found].numVerts += used * surf->geometry->numVerts;
			targetSurfInfo[found].numIndexes += used * surf->geometry->numIndexes;

			shaderIndex.Append( found );		// surface i ( X - shaderIndexStartOfs ) => shaders[Y];
			// for each LOD model, note which source surface maps to which target surface
			shaderIndexStartOfs ++;

			// If this surface should create backsides, the next surface will be the automatically
			// created backsides, so skip them, as FinishSurfaces() below will recreate them:
			if (surf->shader->ShouldCreateBackSides())
			{
				// skip next surface
				i++;
			}
		}
	}

	if (targetSurfInfo.Num() == 0)
	{
			gameLocal.Error("Dup model (%i LOD stages) with no surfaces at all.\n", nSources);
	}
//	gameLocal.Printf("ModelGenerator: Need %i surfaces on the final model.", targetSurfInfo.Num() );

	// debug: print info for each new surface
	for (int j = 0; j < targetSurfInfo.Num(); j++)
	{
		gameLocal.Printf(" Allocating for surface %i: %s numVerts %i numIndexes %i\n", 
				j, targetSurfInfo[j].surf.shader->GetName(), targetSurfInfo[j].numVerts, targetSurfInfo[j].numIndexes );
		targetSurfInfo[j].surf.geometry = hModel->AllocSurfaceTriangles( targetSurfInfo[j].numVerts, targetSurfInfo[j].numIndexes );
		targetSurfInfo[j].surf.geometry->numVerts = targetSurfInfo[j].numVerts;
		targetSurfInfo[j].surf.geometry->numIndexes = targetSurfInfo[j].numIndexes;
		// use these now to track how much we already copied
		targetSurfInfo[j].numVerts = 0;
		targetSurfInfo[j].numIndexes = 0;
	}

	// now combine everything into one mode

	// for each offset
	for (int o = 0; o < offsets->Num(); o++)
	{
		op = offsets->Ptr()[o];

		//gameLocal.Warning(" Offset %0.2f, %0.2f, %0.2f.\n", op.offset.x, op.offset.y, op.offset.z );

		// precompute these
		idMat3 a = op.angles.ToMat3();

		const idRenderModel* source = LODs->Ptr()[op.lod];
		// TODO:
		//const bool noShadow = op.noShadow;

		numSurfaces = source->NumBaseSurfaces();

		int si = shaderIndexStart[op.lod];
		// for each surface of the model
		for (int i = 0; i < numSurfaces; i++)
		{
			surf = source->Surface( i );
			if (!surf)
			{
				continue;
			}

			// TODO:
			// if (noShadow && surf->shader->GetName() == 'textures/common/shadow')
			//{
			//	continue;
			//}

			// find the surface on the target
			int st = shaderIndex[ si + i ];

		//	gameLocal.Warning("ModelGenerator: Final surface number for add: %i (%i + %i)", st, si, i);

			newTargetSurfInfoPtr = &(targetSurfInfo[st]);

			// shortcut
			newSurf = &newTargetSurfInfoPtr->surf;

			int nV = newTargetSurfInfoPtr->numVerts;
			int nI = newTargetSurfInfoPtr->numIndexes;
			int vmax = surf->geometry->numVerts;
			// copy the vertexes
			for (int j = 0; j < vmax; j++)
			{
				newSurf->geometry->verts[nV] = surf->geometry->verts[j];
				idDrawVert *v = &(newSurf->geometry->verts[nV]);

				// rotate
				v->xyz *= a;
				// then offset
				v->xyz += op.offset;
				// rotate normal and tangents
				v->normal *= a;
				v->tangents[0] *= a;
				v->tangents[1] *= a;
				// Set "per-entity" color if we have more than one entity:
				v->SetColor( op.color );
/*				if (o == 1 || o == 2)
				{
					gameLocal.Printf ("Vert %i (%i): xyz %s st %s tangent %s %s normal %s color %i %i %i %i.\n",
						j, nV, v->xyz.ToString(), v->st.ToString(), v->tangents[0].ToString(), v->tangents[1].ToString(), v->normal.ToString(),
						v->color[0],
						v->color[1],
						v->color[2],
						v->color[3]
					   	);
				}
*/				nV ++;
			}
			// copy indexes
			int no = newTargetSurfInfoPtr->numVerts;			// correction factor (before adding nV!)
			int imax = surf->geometry->numIndexes;
			for (int j = 0; j < imax; j++)
			{
				newSurf->geometry->indexes[nI ++] = surf->geometry->indexes[j] + no;
			}
			newTargetSurfInfoPtr->numVerts += vmax;
			newTargetSurfInfoPtr->numIndexes += imax;

			// If this surface should create backsides, the next surface will be the automatically
			// created backsides, so skip them, as FinishSurfaces() below will recreate them:
			if (newSurf->shader->ShouldCreateBackSides())
			{
				// skip next surface
				i++;
			}
		} // end for each surface on this offset
	}	// end for each offset

	// finish the surfaces
	for (int j = 0; j < targetSurfInfo.Num(); j++)
	{
		newSurf = &targetSurfInfo[j].surf;

		// sanity check
		if (targetSurfInfo[j].surf.geometry->numVerts != targetSurfInfo[j].numVerts)
		{
			gameLocal.Warning ("ModelGenerator: surface %i differs between allocated (%i) and created numVerts (%i)",
				j, targetSurfInfo[j].surf.geometry->numVerts, targetSurfInfo[j].numVerts );
		}
		if (targetSurfInfo[j].surf.geometry->numIndexes != targetSurfInfo[j].numIndexes)
		{
			gameLocal.Warning ("ModelGenerator: surface %i differs between allocated (%i) and created numIndexes (%i)",
				j, targetSurfInfo[j].surf.geometry->numIndexes, targetSurfInfo[j].numIndexes );
		}

        newSurf->geometry->tangentsCalculated = true;
		// TODO: are these nec.?
        newSurf->geometry->facePlanesCalculated = false;
        //newSurf->geometry->generateNormals = true;
        newSurf->geometry->generateNormals = false;
		// calculate new bounds
		SIMDProcessor->MinMax( newSurf->geometry->bounds[0], newSurf->geometry->bounds[1], newSurf->geometry->verts, newSurf->geometry->numVerts );
		newSurf->id = 0;
		hModel->AddSurface( targetSurfInfo[j].surf );
		// nec.?
		targetSurfInfo[j].surf.geometry = NULL;
	}

	// generate shadow hull as well as tris for twosided materials
	hModel->FinishSurfaces();

//	hModel->Print();

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
Returns the maximum number of models that can be combined from this model:
*/
unsigned int CModelGenerator::GetMaxModelCount( const idRenderModel* hModel ) const
{
	const modelSurface_t *surf;

	// compute vertex and index count on this model
	unsigned int numVerts = 0;
	unsigned int numIndexes = 0;

	// get the number of base surfaces (minus decals) on the old model
	int numSurfaces = hModel->NumBaseSurfaces();

	for (int i = 0; i < numSurfaces; i++)
	{
		surf = hModel->Surface( i );
		if (surf)
		{
			numVerts += surf->geometry->numVerts; 
			numIndexes += surf->geometry->numIndexes;
		}
	}

	// avoid divide by zero for empty models
	if (numVerts == 0) { numVerts = 1; }
	if (numIndexes == 0) { numIndexes = 1; }

	int v = MAX_MODEL_VERTS / numVerts;
	int i = MAX_MODEL_INDEXES / numIndexes;

	// minimum of the two
	if (v < i) { i = v; }

	return i;
}


