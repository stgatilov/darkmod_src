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

#include "../compiler_common.h"
#include "../../../renderer/Image.h"


class PrtCollision {
public:
	~PrtCollision();
	void Run(const char *mapFileName);

private:
	void ProcessMap();
	void ProcessModel(const char *modelName, const idVec3 &origin, const idMat3 &axis, bool disabled);
	bool ProcessSurfaceEmitter(const srfTriangles_t *geom, const idVec3 &origin, const idMat3 &axis, const idParticleStage *prtStage, const char *ident, const char *prtName, int stageIdx);
	idRenderWorld *RenderWorld();

	// path to .map file
	const char *mapFileName = nullptr;
	// parsed .map file
	idMapFile *mapFile = nullptr;
	// used to parse map/proc file and to compute traces
	idRenderWorld *renderWorld = nullptr;

	int numSurfsProcessed = 0;
	int numSurfsDisabled = 0;
	int64 numRaysCasted = 0;
	double timeStarted = 0.0;
	double timeEnded = 0.0;
	double timeElapsed = 0.0;
};

PrtCollision::~PrtCollision() {
	delete mapFile;
	if (renderWorld)
		renderSystem->FreeRenderWorld(renderWorld);
}

bool PrtCollision::ProcessSurfaceEmitter(const srfTriangles_t *geom, const idVec3 &origin, const idMat3 &axis, const idParticleStage *prtStage, const char *outputFilename, const char *prtName, int stageIdx) {
	assert(prtStage->collisionStatic);

	bool supportedWithTextureLayout = (
		((idVec3*)prtStage->distributionParms)->LengthSqr() == 0.0f &&
	true);
	bool singleTraceOptimization = (
		prtStage->customPathType == prtCustomPth_t::PPATH_STANDARD &&
		prtStage->directionType == prtDirection_t::PDIR_CONE && prtStage->directionParms[0] == 0.0f &&
		prtStage->gravity == 0.0f &&
		prtStage->speed.from == prtStage->speed.to && !prtStage->speed.table &&
	true);

	if (prtStage->mapLayoutType != PML_TEXTURE) {
		common->Error("Particle %s stage %d: only 'texture' is supported as 'mapLayout' yet");
	}
	if (prtStage->worldAxis) {
		common->Error("Particle %s stage %d: worldAxis not supported yet");
	}
	if (!supportedWithTextureLayout) {
		common->Error("Particle %s stage %d: nonzero distribution not supported with texture mapLayout", prtName, stageIdx);
	}
	if (!singleTraceOptimization) {
		common->Error("Particle %s stage %d: generic collision detection not implemented yet", prtName, stageIdx);
	}

	//evaluate bounding box for texture coords
	idBounds texBounds;
	texBounds.Clear();
	for (int i = 0; i < geom->numIndexes; i++) {
		idVec2 tc = geom->verts[geom->indexes[i]].st;
		texBounds.AddPoint(idVec3(tc.x, tc.y, 0.0));
	}
	texBounds.IntersectsBounds(bounds_zeroOneCube);
	if (texBounds.IsBackwards()) {
		common->Warning("Particle %s collisionStatic %s: texture coordinates out of [0..1] x [0..1] domain", prtName, outputFilename);
		texBounds.Zero();
	}

	//find minimal subrectangle to be computed and saved
	int w = prtStage->mapLayoutSizes[0], h = prtStage->mapLayoutSizes[1];
	int xBeg = (int)idMath::Floor(texBounds[0].x * w);
	int xEnd = (int)idMath::Ceil (texBounds[1].x * w);
	int yBeg = (int)idMath::Floor(texBounds[0].y * h);
	int yEnd = (int)idMath::Ceil (texBounds[1].y * h);
	if (xEnd == xBeg)
		(xEnd < w ? xEnd++ : xBeg--);
	if (yEnd == yBeg)
		(yEnd < h ? yEnd++ : yBeg--);
	assert(xBeg >= 0 && xBeg < xEnd && xEnd <= w);
	assert(yBeg >= 0 && yBeg < yEnd && yEnd <= h);
	int xSz = xEnd - xBeg;
	int ySz = yEnd - yBeg;

	int bytes = xSz * ySz * 4;
	uint32 *texels = (uint32*)Mem_Alloc(bytes);
	memset(texels, 0, bytes);
	int hitsCount = 0;
	for (int y = yBeg; y < yEnd; y++)
		for (int x = xBeg; x < xEnd; x++) {
			idVec2 texCoord((x + 0.5) / w, (y + 0.5) / h);

			int usedTriNum = 0;
			int insideTriNum = 0;
			int triNum = geom->numIndexes/3;
			for (int t = 0; t < triNum; t++) {
				const idDrawVert &v0 = geom->verts[geom->indexes[3 * t + 0]];
				const idDrawVert &v1 = geom->verts[geom->indexes[3 * t + 1]];
				const idDrawVert &v2 = geom->verts[geom->indexes[3 * t + 2]];
				const idVec2 &st0 = v0.st, &st1 = v1.st, st2 = v2.st;

				static const float TEXAREA_SINGULAR_EPS = 1e-5f;
				static const float BARY_BOUNDARY_EPS = 1e-5f;
				static const float BARY_OVERLAP_EPS = 1e-3f;

				//compute barycentric coordinates for texture
				float bary0 = (texCoord - st1).Cross(st2 - st1);
				float bary1 = (texCoord - st2).Cross(st0 - st2);
				float bary2 = (texCoord - st0).Cross(st1 - st0);
				float totalArea = bary0 + bary1 + bary2;
				if (idMath::Fabs(totalArea) <= TEXAREA_SINGULAR_EPS)
					continue;
				totalArea = 1.0f / totalArea;
				bary0 *= totalArea;
				bary1 *= totalArea;
				bary2 = 1.0f - bary0 - bary1;

				float minBary = idMath::Fmin(idMath::Fmin(bary0, bary1), bary2);
				if (minBary < -BARY_BOUNDARY_EPS)
					continue;
				usedTriNum++;
				if (minBary >= BARY_OVERLAP_EPS)
					insideTriNum++;

				//compute position and axis (exactly as in R_ParticleDeform)
				particleGen_t g;
				memset(&g, 0, sizeof(g));
				g.origin  = bary0 * v0.xyz         + bary1 * v1.xyz         + bary2 * v2.xyz        ;
				g.axis[0] = bary0 * v0.tangents[0] + bary1 * v1.tangents[0] + bary2 * v2.tangents[0];
				g.axis[1] = bary0 * v0.tangents[1] + bary1 * v1.tangents[1] + bary2 * v2.tangents[1];
				g.axis[2] = bary0 * v0.normal      + bary1 * v1.normal      + bary2 * v2.normal     ;
				g.age = g.frac = 0.0f;
				assert(!prtStage->worldAxis);	//TODO: pass matrix instead of whole render entity

				//compute travel path (must be a line segment)
				idVec3 start, end;
				prtStage->ParticleOrigin(&g, start);
				g.frac = 1.0f;
				g.age = prtStage->particleLife;
				prtStage->ParticleOrigin(&g, end);
				//convert to world coordinates
				start = start * axis + origin;
				end = end * axis + origin;

				//find collision
				modelTrace_t mt;
				//hitsCount += renderWorld->Trace(mt, start, end, 0.0f);
				//hitsCount += renderWorld->FastWorldTrace(mt, start, end);
				auto traceFilter = [&](const renderEntity_t *rent, const idRenderModel *model, const idMaterial *material) -> bool {
					if (prtStage->collisionStaticWorldOnly)
						return false;		//note: world is included automatically (fastWorld = true)
					if (material) {
						if (material->Deform() != DFRM_NONE)
							return false;		//most importantly, skip particle systems
						if (!(material->GetContentFlags() & (CONTENTS_WATER | CONTENTS_SOLID)))
							return false;		//skip light flares (but don't skip water)
					}
					else if (rent) {
						idRenderModel *hModel = rent->hModel;
						if (hModel && hModel->IsDynamicModel() != DM_STATIC)
							return false;		//we should not load dynamic models, but just in case
					}
					return true;
				};
				hitsCount += renderWorld->TraceAll(mt, start, end, true, 0.0f, LambdaToFuncPtr(traceFilter), &traceFilter);
				numRaysCasted++;

				//convert into color
				int digits[4] = {0, 0, 0, 255};
				float rem = mt.fraction;
				for (int d = 0; d < 3; d++) {
					rem *= 256.0f;
					digits[d] = idMath::ClampInt(0, 255, int(rem));
					rem -= digits[d];
				}
				texels[(y - yBeg) * xSz + (x - xBeg)] = digits[0] + (digits[1] << 8) + (digits[2] << 16) + (digits[3] << 24);
			}

			if (insideTriNum > 1) {
				common->Warning("Particle %s collisionStatic %s: texture coords wrap over themselves at (%s)", prtName, outputFilename, texCoord.ToString());
				return false;
			}
		}

	R_WriteTGA(outputFilename, (byte*)texels, xSz, ySz);
	Mem_Free(texels);

	return true;
}

void PrtCollision::ProcessModel(const char *modelName, const idVec3 &origin, const idMat3 &axis, bool disabled) {
	idRenderModel *model = renderModelManager->CheckModel(modelName);
	if (!model)
		return;
	assert(strcmp(modelName, model->Name()) == 0);

	// look for surfaces with "deform particle" materials
	int surfNum = model->NumSurfaces();
	for (int s = 0; s < surfNum; s++) {
		const modelSurface_t *surf = model->Surface(s);
		const idMaterial *material = surf->material;

		if (material->Deform() == DFRM_PARTICLE || material->Deform() == DFRM_PARTICLE2) {
			const idDeclParticle *particleDecl = (idDeclParticle *)material->GetDeformDecl();
			const auto &prtStages = particleDecl->stages;

			// look for particle stages with "collisionStatic"
			for (int g = 0; g < prtStages.Num(); g++) {
				const idParticleStage *stage = prtStages[g];
				if (stage->collisionStatic) {
					idStr imageName = idParticleStage::GetCollisionStaticImagePath(modelName, s, g);
					numSurfsProcessed++;
					if (!disabled)
						ProcessSurfaceEmitter(surf->geometry, origin, axis, stage, imageName.c_str(), particleDecl->GetName(), g);
					else {
						numSurfsDisabled++;
						byte white[4] = {255, 255, 255, 0};
						R_WriteTGA(imageName, white, 1, 1);
					}
				}
			}
		}
	}
}

static idDict GetSpawnArgsOfMapEntity(idMapEntity *ent) {
	const char *classname = ent->epairs.GetString("classname", "");
	auto def = (const idDeclEntityDef*)declManager->FindType(DECL_ENTITYDEF, classname, false);
	if (!def)
		return ent->epairs;
	idDict args = def->dict;
	args.Copy(ent->epairs);
	return args;
}

void PrtCollision::ProcessMap() {
	// create render world
	RenderWorld();

	common->Printf("Processing collisionStatic particle systems...\n");
	common->Printf("");
	// find all particle-emitting surfaces with "collisionStatic" particle stages
	// only brushes/patches are looked as candidates, models are ignored completely
	int numEnts = mapFile->GetNumEntities();
	for (int e = 0; e < numEnts; e++) {
		idMapEntity *ent = mapFile->GetEntity(e);
		const char *name = ent->epairs.GetString("name", NULL);
		idDict spawnArgs = GetSpawnArgsOfMapEntity(ent);
		bool isWorldspawn = strcmp(spawnArgs.GetString("classname"), "worldspawn") == 0;

		if (isWorldspawn) {
			int areasNum = renderWorld->NumAreas();
			// check worldspawn geometry of every portal-area
			for (int a = 0; a < areasNum; a++) {
				idStr modelName;
				sprintf(modelName, "_area%d", a);
				ProcessModel(modelName.c_str(), idVec3(), mat3_identity, false);
			}
		}
		else {
			if (!name)
				continue;	// not an entity?...
			int numPrims = ent->GetNumPrimitives();
			if (!numPrims)
				continue;	// empty entity or only model
			// use model from .proc file, i.e. surfaces compiled from brushes/patches
			// idRenderWorld::InitFromMap has already loaded them with entity name = model name
			int isEmitter = spawnArgs.GetInt("particle_collision_static_emitter", "-1");
			idVec3 origin = spawnArgs.GetVector("origin");
			idMat3 axis = spawnArgs.GetMatrix("rotation");
			ProcessModel(name, origin, axis, isEmitter == 0);
		}
	}
}

idRenderWorld *PrtCollision::RenderWorld() {
	if (renderWorld)
		return renderWorld;

	common->Printf("Loading .proc file...\n");
	renderWorld = renderSystem->AllocRenderWorld();
	// add static world models (aka "_areaN")
	renderWorld->InitFromMap(mapFileName);

	common->Printf("Loading map entities...\n");
	int numEnts = mapFile->GetNumEntities();
	for (int e = 0; e < numEnts; e++) {
		idMapEntity *ent = mapFile->GetEntity(e);
		const char *name = ent->epairs.GetString("name", NULL);
		idDict spawnArgs = GetSpawnArgsOfMapEntity(ent);
		const char *classname = spawnArgs.GetString("classname", "");
		if (!name)
			continue;	//includes worldspawn

		//check for special spawnarg which can override our decision
		int isBlocker = spawnArgs.GetInt("particle_collision_static_blocker", "-1");
		if (isBlocker >= 0) {
			//value 0 or 1: override decision
			isBlocker = (isBlocker != 0);
		}
		else {
			//by default, decide if this is a blocker ourselves
			isBlocker = true;
			if (spawnArgs.FindKey("frobable")) {
				//frobbing entity often makes it disappear
				//examples: moveable_base, atdm:frobable_base
				isBlocker = false;
			}
			if (spawnArgs.FindKey("movetype")) {
				//how entity is moved (e.g. animate)
				//examples: atdm:ai_base
				isBlocker = false;
			}
			if (spawnArgs.FindKey("bind")) {
				//the master which controls movement of this object
				isBlocker = false;
			}
			if (spawnArgs.FindKey("speed")) {
				//how fast object moves
				//examples: func_rotating, func_mover
				isBlocker = false;
			}
		}
		if (!isBlocker)
			continue;

		renderEntity_t rent;
		memset(&rent, 0, sizeof(rent));
		gameEdit->ParseSpawnArgsToRenderEntity(&spawnArgs, &rent);
		if (rent.hModel) {
			//BEWARE: entityNum means index in .map file here, no game is running!
			rent.entityNum = e;
			renderWorld->AddEntityDef(&rent);
		}
	}

	return renderWorld;
}

void PrtCollision::Run(const char *mapFileName) {
	this->mapFileName = mapFileName;
	timeStarted = sys->GetClockTicks();

	common->Printf("Loading mapfile %s...\n", mapFileName);
	mapFile = new idMapFile;
	if (!mapFile->Parse(mapFileName))
		common->Error( "Couldn't load map file: '%s'", mapFileName );

	const char *prtGenDir = idParticleStage::GetCollisionStaticDirectory();
	common->Printf("Cleaning %s...\n", prtGenDir);
	//clear _prt_gen directory
	idFileList *allPrtGenFiles = fileSystem->ListFiles(prtGenDir, "", false, true);
	for (int i = 0; i < allPrtGenFiles->GetNumFiles(); i++) {
		idStr fn = allPrtGenFiles->GetFile(i);
		fileSystem->RemoveFile(fn);
	}
	fileSystem->FreeFileList(allPrtGenFiles);

	ProcessMap();

	timeEnded = sys->GetClockTicks();
	timeElapsed = (timeEnded - timeStarted) / sys->ClockTicksPerSecond();
	common->Printf("Finished in %0.3lf seconds (%d emitters, %d disabled, %lld rays)\n", timeElapsed, numSurfsProcessed, numSurfsDisabled, numRaysCasted);
	common->Printf("\n");
}



//==============================================================================
//==============================================================================
//==============================================================================


void RunParticle_f(const idCmdArgs &args) {
	if (args.Argc() < 2) {
		common->Printf(
			"Usage: runParticle mapfile\n"
		);
		return;
	}

	idStr mapfn = args.Argv(1);
	FindMapFile(mapfn);


	common->ClearWarnings( "running runparticle" );

	// refresh the screen each time we print so it doesn't look
	// like it is hung
	common->SetRefreshOnPrint( true );
	com_editors |= EDITOR_RUNPARTICLE;
	{
		PrtCollision processor;
		processor.Run(mapfn);
	}
	com_editors &= ~EDITOR_RUNPARTICLE;
	common->SetRefreshOnPrint( false );

	common->PrintWarnings();
}
