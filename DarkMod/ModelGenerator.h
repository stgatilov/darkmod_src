// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4069 $
 * $Date: 2010-07-18 13:07:48 +0200 (Sun, 18 Jul 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2010 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_MODELGENERATOR_H__
#define __DARKMOD_MODELGENERATOR_H__

/*
===============================================================================

  Model Generator - Generate/combine/scale models at run-time

  This class is a singleton and initiatied/destroyed from gameLocal.

  At the moment it does not use any memory, but this might change later.

===============================================================================
*/

// Tels: If set to 2 << 20, it crashes on my system
#define MAX_MODEL_VERTS		(2 << 18)		// never combine more than this into one model
#define MAX_MODEL_INDEXES	(2 << 18)		// never combine more than this into one model

// Defines offset, rotation and vertex color for a model combine operation
typedef struct {
	idVec3				offset;
	idAngles			angles;
	dword				color;	// packed color
	int					lod; 	// which LOD model stage to use?
} model_ofs_t;

class CModelGenerator {
public:
	//CLASS_PROTOTYPE( CModelGenerator );

						CModelGenerator( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	* Called by gameLocal.
	*/
	void				Init ( void );
	void				Shutdown ( void );
	void				Clear ( void );

	/**
	* Given a pointer to a render model, calls AllocModel() on the rendermanager, then
	* copies all surface data from the old model to the new model. Used to construct a
	* copy of an existing model, so it can then be used as blue-print for other models,
	* which will share the same data. If dupData is true, memory for verts and indexes
	* is duplicated, otherwise the new model shares the data of the old model. In this
	* case the memory of the new model needs to be freed differently, of course :)
	*/
	idRenderModel*			DuplicateLODModels( const idList<const idRenderModel*> *LODs, const char* snapshotName, bool dupData = true,
												const idList<model_ofs_t>* offsets = NULL, const idVec3 *playerPos = NULL, const idVec3 *origin = NULL);

	/**
	* Same as DuplicateModel, but with only one LOD stage as source
	*/
	idRenderModel*			DuplicateModel( const idRenderModel* source, const char* snapshotName, bool dupData = true);

	/**
	* Returns the maximum number of models that can be combined from this model:
	*/
	unsigned int			GetMaxModelCount( const idRenderModel* hModel ) const;

	/**
	* Given the info CombineModels(), sep. the given model out again.
	*/
	//void					RemoveModel( const idRenderModel *source, const model_combineinfo_t *info);

	/**
	* Manipulate memory of a duplicate of a model so that the shared data does not get
	* freed twice.
	*/
	void					FreeSharedModelData ( const idRenderModel *model );

private:

};

#endif /* !__DARKMOD_MODELGENERATOR_H__ */

