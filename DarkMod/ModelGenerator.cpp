// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

/*
	ModelGenerator

	Manipulate, combine or generate models at run time.

	Copyright (C) 2010-2011 Tels (Donated to The Dark Mod Team)

TODO: If a material casts a shadow (but is not textures/common/shadow*), but the model
	  should not cast a shadow after combine, then clone the material (keep track of
	  all clone materials, Save/Restore/Destroy them) and set noshadows on the clone,
	  then use it in a new surface.
TODO: Call FinishSurfaces() for all orginal models, then cache their shadow vertexes,
	  and omit FinishSurfaces() on the combined model. Might speed it up a lot.
TODO: Use the new strategy/code on DuplicateModel(), too, as it would fail for the ivy
	  model and needlessly duplicate the second surface.
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "ModelGenerator.h"

// uncomment to have debug printouts
//#define M_DEBUG 1

// uncomment to get detailed timing info
//#define M_TIMINGS 1

#ifdef M_TIMINGS
static idTimer timer_combinemodels, timer_copymodeldata, timer_finishsurfaces;
int model_combines = 0;
#endif

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
	m_shadowTexturePrefix = "textures/common/shadow";
}

/*
===============
ModelGenerator::Init
===============
*/
void CModelGenerator::Init( void ) {
	m_shadowTexturePrefix = "textures/common/shadow";
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

/* Given a rendermodel and a surface index, checks if that surface is two-sided, and if, tries
   to find the bakside for this surface, e.g. the surface which was copied and flipped. Returns
   either the surface index number, or -1 for "not twosided or not found":
*/
int CModelGenerator::GetBacksideForSurface( const idRenderModel * source, const int surfaceIdx ) const {
	const modelSurface_t *firstSurf;
	const idMaterial *firstShader;

	// no source model?
	if (!source) { return -1; }
		
	int numSurfaces = source->NumSurfaces();

	if ( surfaceIdx >= numSurfaces || surfaceIdx < 0)
	{
		// surfaceIdx must be valid
		return -1;
	}

	firstSurf = source->Surface( surfaceIdx );
	if (!firstSurf) { return -1; }
	firstShader = firstSurf->shader;
	if (!firstShader) { return -1; }

	// if this is the last surface, it cannot have a flipped backside, because that
	// should come after it. We will fall through the loop and return -1 at the end:

	// Run through all surfaces, starting with the one we have + 1
	for (int s = surfaceIdx + 1; s < numSurfaces; s++)
	{
		// skip self
//		if (s == surfaceIdx) { continue; }	// can't happen here as s > surfaceIdx

		// get each surface
		const modelSurface_t *surf = source->Surface( s );

		// if the original creates backsides, the clone must to, because it uses the same shader
		if (!surf || !surf->shader->ShouldCreateBackSides()) { continue; }

		// check if they have the same shader
		const idMaterial *shader = surf->shader;
		if (surf->shader == firstShader)
		{
			// same shader, but has it the same size?
			if (surf->geometry && firstSurf->geometry && 
					surf->geometry->numIndexes == firstSurf->geometry->numIndexes &&
					surf->geometry->numVerts   == firstSurf->geometry->numVerts)
			{
				// TODO: more tests to see that this is the real flipped surface and not just a double material?
				// found it
				return s;
			}
		}
	}
	// not found
	return -1;
}

/* Given a rendermodel, returns true if this model has any surface that would cast a shadow.
*/
bool CModelGenerator::ModelHasShadow( const idRenderModel * source ) const {

	// no source model?
	if (!source) { return -1; }
		
	int numSurfaces = source->NumSurfaces();

	// Run through all surfaces
	for (int s = 0; s < numSurfaces; s++)
	{
		// get each surface
		const modelSurface_t *surf = source->Surface( s );
		if (!surf) { continue; }

		const idMaterial *shader = surf->shader;
		if ( shader->SurfaceCastsShadow() )
		{
			// found at least one surface casting a shadow
			return true;
		}
	}
	return false;
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
	bool needScale = false;

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

	if (scale && (scale->x != 1.0f || scale->y != 1.0f || scale->z != 1.0f))
	{
		needScale = true;
	}

#ifdef M_DEBUG
	gameLocal.Warning("Source with %i surfaces. snapshot %s, scaling: %s\n", numSurfaces, snapshotName, needScale ? "yes" : "no");
#endif

	// for each needed surface
	for (int i = 0; i < numSurfaces; i++)
	{
		//gameLocal.Warning("Duplicating surface %i.\n", i);
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
			//gameLocal.Warning("Duplicating %i verts and %i indexes.\n", surf->geometry->numVerts, surf->geometry->numIndexes );

			newSurf.geometry = hModel->AllocSurfaceTriangles( numVerts, numIndexes );

			int nV = 0;		// vertexes
			int nI = 0;		// indexes

			if (needScale)
			{
				// a direct copy with scaling
				for (int j = 0; j < surf->geometry->numVerts; j++)
				{
					idDrawVert *v = &(newSurf.geometry->verts[nV]);
					newSurf.geometry->verts[nV++] = surf->geometry->verts[j];
					v->xyz.MulCW(*scale);
					v->normal.DivCW(*scale);
					v->tangents[0].DivCW(*scale);
					v->tangents[1].DivCW(*scale);
					v->ScaleToUnitNormal();
				}
			}
			else
			{
				// just one direct copy
				for (int j = 0; j < surf->geometry->numVerts; j++)
				{
					newSurf.geometry->verts[nV++] = surf->geometry->verts[j];
				}
			}

			// copy indexes
			for (int j = 0; j < surf->geometry->numIndexes; j++)
			{
				newSurf.geometry->indexes[nI ++] = surf->geometry->indexes[j];
			}

			// set these so they don't get recalculated in FinishSurfaces():
	        newSurf.geometry->tangentsCalculated = true;
	        newSurf.geometry->facePlanesCalculated = true;
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

The models in LODs do not have to be LOD stages, they can be anything as long as for
each offset the corrosponding entry in LOD exists.
===============
*/
idRenderModel * CModelGenerator::DuplicateLODModels (const idList<const idRenderModel*> *LODs,
			const char* snapshotName, const idList<model_ofs_t> *offsets, const idVec3 *origin,
			const idMaterial *shader, idRenderModel* hModel ) const {
	int numSurfaces;
	int numVerts, numIndexes;
	const modelSurface_t *surf;
	modelSurface_t *newSurf;
	bool needFinish;				// do we need to call FinishSurfaces() at the end?

	// info about each model (how often used, how often shadow casting, which surfaces to copy where)
	idList<model_stage_info_t> modelStages;
	model_stage_info_t *modelStage;	// ptr to current model stage

	const model_ofs_t *op;

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

	// count the tris overall
	numIndexes = 0; numVerts = 0;

	if (NULL == offsets)
	{
		gameLocal.Error("NULL offsets in DuplicateLODModels");
	}

	int nSources = LODs->Num();
	if (nSources == 0)
	{
		gameLocal.Error("No LOD models DuplicateLODModels");
	}
    //gameLocal.Warning("Have %i LOD stages.", nSources);

	// Stages of our plan:

	/* ** 1 ** 
	* First check for each potentially-used model if it has a shadow-casting surface
	*	O(N*M) - N is number of stages/models 						 (usually 7 or less)
	*			 M is number of surfaces (on average) for each stage (usually 1..3)
	* At this point we only know that we don't need to call FinishSurfaces() if there
	* are no shadow-casting models at all, but if there, we are still not sure because
	* it can turn out the shadow-casting ones aren't actually used. So:
	*/

	/* ** 2 **
	* Then check for each offset which model stage it uses, and count them.
	* Also correct out-of-bounds entries here.
	*		O(N) where N the number of offsets.
	*/

	/* ** 3 **
	* Now that we know whether we need FinishSurfaces(), we also know whether we
	* need to skip backsides, or not. So lets map out which surface of which stage
	* gets copied to what surface of the final model.
	*	O(N*M) - N is number of stages/models 						 (usually 7 or less)
	*			 M is number of surfaces (on average) for each stage (usually 1..3)
	*/

	/* ** 4 **
	* Allocate memory for all nec. surfaces.
	*/

	/* ** 5 **
	* Finally we can copy the data together, scaling/rotating/translating models as
	* we go.
	*	O(V) - V is number of tris we need to copy
	*/


	/* Let the play begin: */
	modelStages.SetNum(nSources);
	needFinish = false;				// do we need to call FinishSurfaces() at the end?

	/* ** Stage 1 ** */
	// For each model count how many shadow casting surfaces it has
	// And init modelStages, too.

#ifdef M_DEBUG
	gameLocal.Printf("Stage #1\n");
#endif

	// TODO: If this prove to be an expensive step (which I doubt), then we could
	//		 cache the result, because the ptr to the renderModel should be stable.
	for (int i = 0; i < nSources; i++)
	{
		modelStage = &modelStages[i];

		// init fields
		modelStage->usedShadowless = 0;
		modelStage->usedShadowing = 0;
		modelStage->couldCastShadow = false;
		modelStage->noshadowSurfaces.Clear();
		modelStage->shadowSurfaces.Clear();
		modelStage->surface_info.Clear();

		if (NULL == (modelStage->source = LODs->Ptr()[i]))
		{
			// Use the default model
			if (NULL == (modelStage->source = LODs->Ptr()[0]))
			{
				gameLocal.Warning("NULL source ptr for default LOD stage (was default for %i)", i);
				// cannot use this
				continue;
			}
		}
#ifdef M_DEBUG
		// print the source model
		modelStage->source->Print();
#endif
		if (modelStage->source->NumSurfaces() == 0)
		{
			// ignore empty models
			modelStage->source = NULL;
			continue;
		}

		// false if there are no shadow-casting surfaces at all
		modelStage->couldCastShadow = ModelHasShadow( modelStage->source );
	}

#ifdef M_DEBUG
	gameLocal.Printf("Stage #2\n");
#endif

	/* ** Stage 2 ** */
	// For each offset, count what stages it uses (and with or without shadow) and also
	// correct missing (LOD) models
	const model_ofs_t *OffsetsPtr = offsets->Ptr();

	int numOffsets = offsets->Num();
	for (int i = 0; i < numOffsets; i++)
	{
		op = &OffsetsPtr[i];
		// correct entries that are out-of-bounds
		int lod = op->lod;
		if (lod >= nSources) { lod = nSources - 1; }
		// otherwise invisible
		if (lod < 0)
		{
			continue;
		}
		// uses stage op->lod
		modelStage = &modelStages[lod];

		// the stage is used with shadow only if 
		// BOTH the model could cast a shadow and the offset says it wants a shadow
		if ( modelStage->couldCastShadow && !(op->flags & SEED_MODEL_NOSHADOW) )
		{
			modelStage->usedShadowing ++;
			// can and could cast a shadow, so we need FinishSurface()
			needFinish = true;
		}
		else
		{
			modelStage->usedShadowless ++;
		}
	}

#ifdef M_DEBUG
	gameLocal.Printf("Stage #3\n");
#endif

	/* ** Stage 3 ** */
	// If we have multiple models with different surfaces, then we need to combine
	// all surfaces from all models, so first we need to build a list of needed surfaces
	// by shader name. This ensures that if LOD 0 has A and B and LOD 1 has B and C, we
	// create only three surfaces A, B and C, and not four (A, B.1, B.2 and C). This will
	// help reducing drawcalls:
	targetSurfInfo.Clear();

	for (int i = 0; i < nSources; i++)
	{
		modelStage = &modelStages[i];

		int modelUsageCount = modelStage->usedShadowless + modelStage->usedShadowing;
		// not used at all?
		if ( (0 == modelUsageCount) || (NULL == modelStage->source) )
		{
			// skip
#ifdef M_DEBUG
			gameLocal.Printf("Stage #%i is not used at all, skipping it.\n", i);
#endif
			continue;
		}
		const idRenderModel* source = modelStage->source;

		// get the number of all surfaces
		numSurfaces = source->NumSurfaces();

#ifdef M_DEBUG
		gameLocal.Printf("At stage #%i with %i surfaces, used %i times (%i shadow, %i noshadow).\n", i, numSurfaces, modelUsageCount, modelStage->usedShadowing, modelStage->usedShadowless);
#endif

		// If we have not yet filled this array, so do it now to avoid costly rechecks:
		if (modelStage->surface_info.Num() == 0)
		{
			// Do this as extra step before, so it is completely filled before we go
			// through the surfaces and decide whether to keep them or not:
			modelStage->surface_info.SetNum( numSurfaces );
			modelStage->noshadowSurfaces.SetNum( numSurfaces );
			modelStage->shadowSurfaces.SetNum( numSurfaces );
			// init to 0
			for (int s = 0; s < numSurfaces; s++)
			{
				modelStage->surface_info[s] = 0;
				modelStage->noshadowSurfaces[s] = -1;	// default: skip
				modelStage->shadowSurfaces[s] = -1;		// default: skip
			}

			for (int s = 0; s < numSurfaces; s++)
			{
				surf = source->Surface( s );
				if (!surf) { continue; }
				const idMaterial *curShader = surf->shader;

				// Surfaces that have the backside bit already set are not considered here
				// or we would find maybe their source by accident (we only want to skip
				// the backside, not the frontside)
				if ((modelStage->surface_info[s] & 1) == 0)
				{
					int backside = GetBacksideForSurface( modelStage->source, s );
					// -1 not found, 0 can't happen as we start with 0 and 0 can't be its own backside
					if (backside > 0)
					{
						// set bit 0 to true, so we know this is a backside
						modelStage->surface_info[backside] &= 1;	// 0b0001
					}
				}
				if (curShader->SurfaceCastsShadow())
				{
					// is this is a pure shadow casting surface?
					idStr shaderName = curShader->GetName();
					if (shaderName.Left( m_shadowTexturePrefix.Length() ) == m_shadowTexturePrefix )
					{
						// yes, mark it
						modelStage->surface_info[s] &= 2;			// 0b0010
					}
				}
			}
		}

#ifdef M_DEBUG
		gameLocal.Printf("Run through all %i surfaces\n", numSurfaces);
#endif
		// Run through all surfaces
		for (int s = 0; s < numSurfaces; s++)
		{
			// get each surface
			surf = source->Surface( s );
			if (!surf) { continue; }

			const idMaterial *curShader = surf->shader;
			const int flags = modelStage->surface_info[s];
		   
			/* Two cases: 
			*  1: need to call FinishSurfaces() at the end:
			*	  1a: then we need to skip backsides (the clones of two-sided surfaces)
			*	  1b: and we also need to skip pure shadow casting if the model is used with noshadows
			*  2: no need to call FinishSurfaces() at the end:
			*	  2a: need to skip ONLY pure shadow casting surfaces (they probably would not harm, but
			*		  use up memory and time to copy)
			*/
			bool pureShadow = false;

			if (needFinish)
			{
				/* case 1a */
				if ((flags & 0x1) != 0)
				{
					continue;
				}
				/* case 2a */
				if ((flags & 0x1) != 0)
				{
					pureShadow = true;
				}
			}
			else
			{
				/* case 2a: need to skip ONLY pure shadow casting surfaces */
				if ((flags & 0x2) != 0)
				{
					// this this is a pure shadow casting surface
					continue;
				}
			}

			// Now we know that we need the surface at all, and if we need it in the case of "shadow"
			// Decide to which target surface we need to append

			// TODO: Backside surfaces get automatically added to the frontside, too. Is this ok?

			// Do we have already a surface with that shader?
			// The linear search here is ok, since most models have only a few surfaces, since every
			// surface creates one expensive draw call.
			// if given a shader, use this instead, so everyting will be in one surface:
			idStr n = shader ? shader->GetName() : curShader->GetName();
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
				newTargetSurfInfo.surf.shader = shader ? shader : curShader;
				newTargetSurfInfo.surf.id = 0;
				targetSurfInfo.Append( newTargetSurfInfo );
#ifdef M_DEBUG	
				gameLocal.Warning("ModelGenerator: Need shader %s.", n.c_str() );
#endif
				found = targetSurfInfo.Num() - 1;
			}

			// Increase the nec. counts, if the stage is used X times with shadows and
			// Y times without, the surface needs to be copied X+Y times:
			int count = modelUsageCount;
			if (pureShadow)
			{
				// only count it for shadow models usage, e.g. skip it for non-shadow cases
				count = modelStage->usedShadowing;
			}
			else
			{
				// in case of noshadow, include it since it is not a pure shadow
				modelStage->noshadowSurfaces[s] = found;
				// modelStage->shadowSurfaces[s] stays as -1
			}
			// include everything in the shadow case
			modelStage->shadowSurfaces[s] = found;
			targetSurfInfo[found].numVerts += count * surf->geometry->numVerts;
			targetSurfInfo[found].numIndexes += count * surf->geometry->numIndexes;
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
#endif

#ifdef M_DEBUG
	gameLocal.Printf("Stage #4\n");
#endif

	/* ** Stage 4 - allocate surface memory */
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

#ifdef M_DEBUG
	gameLocal.Printf("Stage #5\n");
#endif

#ifdef M_TIMINGS
	timer_copymodeldata.Start();
#endif

	/* ** Stage 5 - now combine everything into one model */
	// for each offset
	for (int o = 0; o < numOffsets; o++)
	{
		op = &OffsetsPtr[o];
		// should be invisible, so skip
		int lod = op->lod;
		if (lod < 0)
		{
			continue;
		}
	   	if (lod >= nSources) { lod = nSources - 1; }

		// precompute these (stgatilov):
		// scale matrix
		idMat3 mScale(op->scale.x, 0, 0, 
					  0, op->scale.y, 0, 
					  0, 0, op->scale.z);
		// rotate matrix
		idMat3 mRot = op->angles.ToRotation().ToMat3();
		// direction transformation = scale + rotate
		idMat3 tDir = mScale * mRot;
		// normal transformation = (scale + rotate) inverse transpose
		idMat3 tNorm = tDir.Inverse().Transpose();
		// position transformation = scale + rotate + translate
		idMat4 tPos = idMat4(tDir, op->offset);

		modelStage = &modelStages[lod];

		const idRenderModel* source = modelStage->source;
		const bool noShadow = (op->flags & SEED_MODEL_NOSHADOW);

		// gameLocal.Warning("ModelGenerator: op->lod = %i si = %i", op->lod, si);

		// for each surface of the model
		numSurfaces = source->NumSurfaces();
		for (int s = 0; s < numSurfaces; s++)
		{
			//gameLocal.Warning("At surface %i of stage %i at offset %i\n", s, lod, o);

			surf = source->Surface( s );
			if (!surf) { continue; }

			// get the target surface
			int st = modelStage->shadowSurfaces[s];
			if (noShadow)
			{
				st = modelStage->noshadowSurfaces[s];
			}

			// -1 => skip this surface
			if (st < 0)
			{
#ifdef M_DEBUG
				gameLocal.Warning("Skipping surface %i.\n", s);
#endif
				continue;
			}

			newTargetSurfInfoPtr = &targetSurfInfo[st];

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

			dword newColor = op->color; 

			// copy the vertexes and modify them at the same time (scale, rotate, offset)
			for (int j = 0; j < vmax; j++)
			{
				// target
				idDrawVert *v = &(newSurf->geometry->verts[nV]);
				// source
				idDrawVert *vs = &(surf->geometry->verts[j]);

				// stgatilov: fast and proper transformation
				v->xyz = tPos * vs->xyz;
				v->normal = vs->normal * tNorm;
				v->tangents[0] = vs->tangents[0] * tNorm;
				v->tangents[1] = vs->tangents[1] * tNorm;
				v->st = vs->st;
				v->SetColor(newColor);
				// some normalization
				v->ScaleToUnitNormal();
				//NOTE (in case of non-isotropic scaling):
				//if tangents are normalized then bumpmapped surface will look different

/*				if (o == 1 || o == 2)
				{
					gameLocal.Printf ("Vert %i (%i): xyz %s st %s tangent %s %s normal %s color %i %i %i %i.\n",
						j, nV, v->xyz.ToString(), v->st.ToString(), v->tangents[0].ToString(), v->tangents[1].ToString(), v->normal.ToString(),
						v->color[0],
						v->color[1],
						v->color[2],
						v->color[3]
					   	);
				}*/
				nV ++;

			}	// end of all verts

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
        newSurf->geometry->facePlanesCalculated = true;
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


