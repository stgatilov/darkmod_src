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

#ifndef __GAME_MODELGENERATOR_H__
#define __GAME_MODELGENERATOR_H__

/*
===============================================================================

  Model Generator - Generate/combine/scale models at run-time

  This class is a singleton and initiatied/destroyed from gameLocal.

  At the moment it does not use any memory, but this might change later.

===============================================================================
*/

// Defines info about a change to a combined model. E.g. if a combined model was
// combined from 2 times model A, and we want to change the second model from A
// to B, we use this struct:
typedef struct {
	int				oldLOD;					// the original model combined into the megamodel
	int				newLOD;					// the new model to be combined into the megamodel
} model_changeinfo_t;

// Defines offset, rotation and vertex color for a model combine operation
typedef struct {
	idVec3				offset;
	idAngles			angles;
	dword				color;	// packed color
	int					lod; 	// which LOD model stage to use?
} model_ofs_t;

// Contains the info about a megamodel, e.g a model combined from many small models
// TODO: Turn this into a class
typedef struct {
	idRenderModel*				hModel;			// ptr to the combined model
	idList<model_ofs_t>			offsets;		// list of the individual entity combined into the model
	idList<model_changeinfo_t>	changes;		// list with changes
	int							lastUpdate;		// time the mode was last regenerated
} megamodel_t;

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
	idRenderModel*			DuplicateModel( const idRenderModel *source, const char* snapshotName, bool dupData = true, const idList<model_ofs_t>* offsets = NULL);

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

#endif /* !__GAME_MODELGENERATOR_H__ */

