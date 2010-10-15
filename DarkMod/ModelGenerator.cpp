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

   Manipulate, combine or generate models at run time.

TODO: use the supplied scale value when duplicating/combining models
TODO: skip nodraw/_emptyname shaders when combining models
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: ModelGenerator.cpp 4071 2010-07-18 13:57:08Z tels $", init_version);

#include "ModelGenerator.h"

// uncomment to have debug printouts
//#define M_DEBUG 1

// uncomment to get detailed timing info
//#define M_TIMINGS 1

#ifdef M_TIMINGS
static idTimer timer_combinemodels, timer_copymodeldata, timer_finishsurfaces;
int model_combines = 0;
#endif

// This enables the code to test (and skip) automaticaly created backsides. Since NumBaseSurfaces()
// seems to return only the first N original surfaces, this code is not neccessary and thus disabled:
//#define M_TEST_BACKSIDES

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

If given a target model, will replace the contents of this model, otherwise allocate a new model.

Returns the given (or allocated) model.

===============
*/
idRenderModel* CModelGenerator::DuplicateModel (const idRenderModel* source, const char* snapshotName, bool dupData, idRenderModel* hModel, const idVec3 *scale) const {

	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf;
	modelSurface_s newSurf;

	if (NULL == source)
	{
		gameLocal.Error("ModelGenerator: Dup with NULL source model (snapshotName = %s).\n", snapshotName);
	}

	// allocate memory for the model?
	if (NULL == hModel)
	{
		hModel = renderModelManager->AllocModel();
		if (NULL == hModel)
		{
			gameLocal.Error("ModelGenerator: Could not allocate new model.\n");
		}
	}

	// and init it as dynamic empty model
	hModel->InitEmpty( snapshotName );

	numVerts = 0;
	numIndexes = 0;
	numSurfaces = source->NumBaseSurfaces();

#ifdef M_DEBUG
	gameLocal.Printf("Source with %i surfaces. snapshot %s\n", numSurfaces, snapshotName);
#endif

	// for each needed surface
	for (int i = 0; i < numSurfaces; i++)
	{
		// gameLocal.Warning("Duplicating surface %i.\n", i);
		surf = source->Surface( i );
		if (!surf)
		{
			continue;
		}

#ifdef M_TEST_BACKSIDES

		if (!surf->shader->ShouldCreateBackSides())
		{

			// skip this surface if it was doubled because the original surface was two-sided
			// it will be automatically recreated by FinishSurfaces() below.
#ifdef M_DEBUG
			gameLocal.Printf("Surface %i creates no backsides, checking if we should skip it.\n", i);
#endif
			bool found = false;

			idStr shaderName = surf->shader->GetName();
			// the original surface comes before this if this is a duplicated surface
			for (int s2 = 0; s2 < i; s2++)
			{
				const modelSurface_t *surf_org = source->Surface( s2 );
				if (!surf_org)
				{
					continue;
				}
				if (surf_org->shader->ShouldCreateBackSides() &&
						surf_org->geometry->numIndexes == surf->geometry->numIndexes &&
						surf_org->geometry->numVerts == surf->geometry->numVerts &&
						shaderName == surf_org->shader->GetName() )
				{
#ifdef M_DEBUG
					gameLocal.Printf("Found perfect match at %i\n", s2);
#endif
					found = true;
					break;
				}
			}
			// found a matching original surface for this one, so skip it, as FinishSurfaces will
			// recreate it:
			if (found)
			{
				continue;
			}
		} // end backside test
#endif // M_TEST_BACKSIDES

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

			// set these so they don't get recalculated in FinishSurfaces():
	        newSurf.geometry->tangentsCalculated = true;
	        newSurf.geometry->facePlanesCalculated = false;
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
			// by calling FreeSharedModelSurfaces() before destroy:
			newSurf.shader = surf->shader;
			newSurf.geometry = surf->geometry;
		}
		newSurf.id = 0;
		hModel->AddSurface( newSurf );
	}

	// generate shadow hull as well as tris for twosided materials
	hModel->FinishSurfaces();

#ifdef M_DEBUG
	hModel->Print();
#endif

	return hModel;
}

/*
===============
CModelGenerator::DuplicateLODModels - Duplicate a render model based on LOD stages.

If the given list of model_ofs_t is filled, the model will be copied X times, each time
offset and rotated by the given values, scaled, and also fills in the right vertex color.
===============
*/
idRenderModel * CModelGenerator::DuplicateLODModels (const idList<const idRenderModel*> *LODs,
			const char* snapshotName, const idList<model_ofs_t> *offsets, const idVec3 *origin,
			const idMaterial *shader, idRenderModel* hModel ) const {
	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf, *surf_org;
	modelSurface_t *newSurf;
	bool needFinish = false;

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

#ifdef M_TIMINGS
	timer_combinemodels.Start();
	model_combines ++;
#endif

	// allocate memory for the model
	if (NULL == hModel)
	{
		hModel = renderModelManager->AllocModel();
		if (NULL == hModel)
		{
			gameLocal.Error("ModelGenerator: Could not allocate new model.\n");
		}
	}
	// and init it as dynamic empty model
	hModel->InitEmpty( snapshotName );

	// count the tris
	numIndexes = 0; numVerts = 0;

	if (NULL == offsets)
	{
		gameLocal.Error("NULL offsets in DuplicateLODModels");
	}

	// for each offset, correct missing LOD models
	int nSources = LODs->Num();
	if (nSources == 0)
	{
		gameLocal.Error("No LOD models DuplicateLODModels");
	}
//    gameLocal.Warning("Have %i LOD stages.", nSources);

	// correct entries that are out-of-bounds
	for (int i = 0; i < offsets->Num(); i++)
	{
		op = offsets->Ptr()[i];
//		if (op.lod < 0)	// invisible
		if (op.lod >= nSources)
		{
			op.lod = nSources - 1;
		}
	}

	// If we have multiple LOD models with different surfaces, then we need to combine
	// all surfaces from all models, so first we need to build a list of needed surfaces
	// by shader name. This ensures that if LOD 0 has A and B and LOD 1 has B and C, we
	// create only three surfaces A, B and C, and not four (A, B.1, B.2 and C). This will
	// help reducing drawcalls:

	targetSurfInfo.Clear();

    int shaderIndexStartOfs = 0;
	for (int i = 0; i < nSources; i++)
	{
		const idRenderModel* source = LODs->Ptr()[i];
		if (NULL == source)
		{
			// Use the default model
			source = LODs->Ptr()[0];
			if (NULL == source)
			{
				gameLocal.Warning("NULL source ptr for default LOD stage (was default for %i)", i);
				continue;
			}
//			gameLocal.Warning("Using default for empty LOD stage %i", i);
//			continue;	// skip non-existing LOD stages
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
			shaderIndexStart.Append( -1 );	// dummy value, not used but need to reserve the spot
			continue;
		}

		// get the number of base surfaces (minus decals and minus automatically created backsides) on the source model
		numSurfaces = source->NumBaseSurfaces();

#ifdef M_DEBUG
		gameLocal.Printf("ModelGenerator: LOD stage %i (%i base surfaces) is used %i times.\n", i, numSurfaces, used );
#endif

		shaderIndexStart.Append( shaderIndexStartOfs );	// where in shaderIndex starts the map for this LOD stage?

		// get each surface
		for (int s = 0; s < numSurfaces; s++)
		{
			surf = source->Surface( s );
			if (!surf)
			{
				continue;
			}

			// we need FinishSurfaces() to create the backsides
			if (surf->shader->ShouldCreateBackSides())
			{
				needFinish = true;
			}

#ifdef M_TEST_BACKSIDES
			// see if we must skip one later surface for this one. For instance a model with:
			//    verts  tris material
			//		 0:   813   639 bc_lily
			//		 1:   144   108 bc_lilyleaf
			//		 2:   111   108 orchid
			//		 3:   264    96 textures/common/shadow2
			//		 4:   813   639 bc_lily						SKIP!
			//		 5:   144   108 bc_lilyleaf					SKIP!

			// this surface creates no back sides
			if (!surf->shader->ShouldCreateBackSides())
			{
#ifdef M_DEBUG
				gameLocal.Printf("Surface %i creates no backsides, checking if we should skip it.\n", s);
#endif
				idStr shaderName = surf->shader->GetName();
				bool found = false;
				// the original surface comes before this if this is a duplicated surface
				for (int s2 = 0; s2 < s; s2++)
					{
					surf_org = source->Surface( s2 );
					if (!surf_org)
					{
						continue;
					}
					gameLocal.Printf("Testing %s against %s (index=%i, ShouldCreateBacksides=%i, v: %i == %i, i: %i == %i)\n", 
								surf->shader->GetName(), surf_org->shader->GetName(), s2,
								surf_org->shader->ShouldCreateBackSides(),
								surf_org->geometry->numIndexes, surf->geometry->numIndexes, surf_org->geometry->numVerts, surf->geometry->numVerts);
#ifdef M_DEBUG
#endif
					if (surf_org->shader->ShouldCreateBackSides() &&
							surf_org->geometry->numIndexes == surf->geometry->numIndexes &&
							surf_org->geometry->numVerts == surf->geometry->numVerts &&
							shaderName == surf_org->shader->GetName() )
					{
#ifdef M_DEBUG
						gameLocal.Printf("Found perfect match at %i\n", s2);
#endif
						found = true;
						break;
					}
				}
				// found a matching original surface for this one, so skip it, as FinishSurfaces will
				// recreate it:
				if (found)
				{
					// append a dummy entry that says "skip this"
					shaderIndex.Append( -1 );		// surface i ( X - shaderIndexStartOfs ) => shaders[Y];
					// for each LOD model, note which source surface maps to which target surface
					shaderIndexStartOfs ++;
					continue;
				}
			}
#endif // M_TEST_BACKSIDES

			// Do we have already a surface with that shader?

			// The linear search here is ok, since most models have only a few surfaces, since every
			// surface creates one expensive draw call.

			// if given a shader, use this instead, so everyting will be in one surface:
			idStr n = shader ? shader->GetName() : surf->shader->GetName();
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
				// if given a shader, use this instead.
				newTargetSurfInfo.surf.shader = shader ? shader : surf->shader;
				newTargetSurfInfo.surf.id = 0;
				targetSurfInfo.Append( newTargetSurfInfo );
#ifdef M_DEBUG	
				gameLocal.Warning("ModelGenerator: Need shader %s.", n.c_str() );
#endif
				found = targetSurfInfo.Num() - 1;
			}

			// increase the nec. counts
			targetSurfInfo[found].numVerts += used * surf->geometry->numVerts;
			targetSurfInfo[found].numIndexes += used * surf->geometry->numIndexes;

			shaderIndex.Append( found );		// surface i ( X - shaderIndexStartOfs ) => shaders[Y];
			// for each LOD model, note which source surface maps to which target surface
			shaderIndexStartOfs ++;
		}
	}

	if (targetSurfInfo.Num() == 0)
	{
		// TODO: this can happen if the model contains no visible LOD stages, but wasn't
		//		 culled or made invisible yet. So allow it.
		gameLocal.Error("Dup model (%i LOD stages) with no surfaces at all.\n", nSources);
	}

#ifdef M_DEBUG	
	gameLocal.Printf("ModelGenerator: Need %i surfaces on the final model.\n", targetSurfInfo.Num() );

	// debug: print shaderIndex array
	gameLocal.Printf("ModelGenerator: Dumping shaderIndex:\n");
	for (int i = 0; i < shaderIndex.Num(); i++)
	{
		gameLocal.Printf(" %i = %i\n", i, shaderIndex[i] );
	}
	gameLocal.Printf("ModelGenerator: Dumping shaderIndexStart:\n");
	for (int i = 0; i < shaderIndexStart.Num(); i++)
	{
		gameLocal.Printf(" %i = %i\n", i, shaderIndexStart[i] );
	}
#endif

	for (int j = 0; j < targetSurfInfo.Num(); j++)
	{
#ifdef M_DEBUG	
		// debug: print info for each new surface
		gameLocal.Printf(" Allocating for surface %i: %s numVerts %i numIndexes %i\n", 
			j, targetSurfInfo[j].surf.shader->GetName(), targetSurfInfo[j].numVerts, targetSurfInfo[j].numIndexes );
#endif

		targetSurfInfo[j].surf.geometry = hModel->AllocSurfaceTriangles( targetSurfInfo[j].numVerts, targetSurfInfo[j].numIndexes );
		targetSurfInfo[j].surf.geometry->numVerts = targetSurfInfo[j].numVerts;
		targetSurfInfo[j].surf.geometry->numIndexes = targetSurfInfo[j].numIndexes;
		// use these now to track how much we already copied
		targetSurfInfo[j].numVerts = 0;
		targetSurfInfo[j].numIndexes = 0;
	}

	// now combine everything into one model
#ifdef M_TIMINGS
	timer_copymodeldata.Start();
#endif

	const model_ofs_t *OffsetsPtr = offsets->Ptr();

	// for each offset
	for (int o = 0; o < offsets->Num(); o++)
	{
		op = OffsetsPtr[o];
		// should be invisible, so skip
		if (op.lod < 0)
		{
			continue;
		}
		//gameLocal.Warning(" Offset %0.2f, %0.2f, %0.2f LOD %i.\n", op.offset.x, op.offset.y, op.offset.z, op.lod );

		// precompute these
		idMat3 a = op.angles.ToMat3();

		const idRenderModel* source = LODs->Ptr()[op.lod];

		if (!source)
		{
			// use the default model
			//gameLocal.Warning(" Using LOD model 0 as replacement." );
			source = LODs->Ptr()[0];

			if (!source)
			{
				gameLocal.Warning(" LOD model 0 as replacement for stage %i is empty.", op.lod );
				continue;
			}
		}

		const bool noShadow = (op.flags & LODE_MODEL_NOSHADOW);

		// if this model has shadows, we need FinishSurfaces() below:
		if (!noShadow)
		{
			needFinish = true;
		}

		numSurfaces = source->NumBaseSurfaces();
		int si = shaderIndexStart[op.lod];

		// gameLocal.Warning("ModelGenerator: op.lod = %i si = %i", op.lod, si);

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

#ifdef M_DEBUG
			gameLocal.Warning("ModelGenerator: Final surface number for add: %i (%i + %i)", st, si, i);
#endif

			// -1 => skip this surface
			if (st < 0)
			{
				gameLocal.Warning("Skipping surface %i.\n", i);
#ifdef M_DEBUG
#endif
				continue;
			}

			newTargetSurfInfoPtr = &(targetSurfInfo[st]);

			if (!newTargetSurfInfoPtr)
			{
				gameLocal.Warning("newTargetSurfInfoPtr = NULL");
				continue;
			}

			// shortcut
			newSurf = &newTargetSurfInfoPtr->surf;

			int nV = newTargetSurfInfoPtr->numVerts;
			int nI = newTargetSurfInfoPtr->numIndexes;
			int vmax = surf->geometry->numVerts;
			// copy the vertexes and modify them at the same time (scale, rotate, offset)
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
		} // end for each surface on this offset
	}	// end for each offset

#ifdef M_TIMINGS
	timer_copymodeldata.Stop();
#endif

	// finish the surfaces
	for (int j = 0; j < targetSurfInfo.Num(); j++)
	{
		newSurf = &targetSurfInfo[j].surf;

#ifdef M_DEBUG
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
#endif

        newSurf->geometry->tangentsCalculated = true;
		// TODO: are these nec.?
        //newSurf->geometry->facePlanesCalculated = false;
        newSurf->geometry->facePlanesCalculated = true;
        //newSurf->geometry->generateNormals = true;
        newSurf->geometry->generateNormals = false;
		// calculate new bounds
		SIMDProcessor->MinMax( newSurf->geometry->bounds[0], newSurf->geometry->bounds[1], newSurf->geometry->verts, newSurf->geometry->numVerts );
		newSurf->id = 0;
		hModel->AddSurface( targetSurfInfo[j].surf );
		// nec.?
		targetSurfInfo[j].surf.geometry = NULL;
	}

#ifdef M_TIMINGS
	timer_finishsurfaces.Start();
#endif
	if (needFinish)
	{	
		// generate shadow hull as well as tris for twosided materials
		hModel->FinishSurfaces();
	}

#ifdef M_TIMINGS
	timer_finishsurfaces.Stop();
    timer_combinemodels.Stop();

	if (model_combines % 10 == 0)
	{
		gameLocal.Printf( "ModelGenerator: combines %i, total time %0.2f ms (for each %0.2f ms), copy data %0.2f ms (for each %0.2f ms), finish surfaces %0.2f ms (for each %0.2f ms)\n",
				model_combines,
				timer_combinemodels.Milliseconds(),
				timer_combinemodels.Milliseconds() / model_combines,
				timer_copymodeldata.Milliseconds(),
				timer_copymodeldata.Milliseconds() / model_combines,
				timer_finishsurfaces.Milliseconds(),
				timer_finishsurfaces.Milliseconds() / model_combines );
	}
#endif

#ifdef M_DEBUG
	hModel->Print();
#endif

	return hModel;
}

/*
===============
CModelGenerator::FreeSharedModelData - manipulate memory of a duplicate model so that shared data does not get freed twice
===============
*/
void CModelGenerator::FreeSharedModelData ( const idRenderModel *source ) const
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


