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

// Defines info about a model combine operation
typedef struct {
	int					firstSurface;		// which was the first surface to be added, -1 for none
	int					Surfaces;			// how many surfaces where added
	int					firstVert;			// which was the first vert to be added, >= 0
	int					verts;				// how many verts where added
	int					firstIndex;			// which was the first index to be added, >= 0
	int					lastIndex;			// how many indexes where added
} model_combineinfo_t;

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
	idRenderModel*			DuplicateModel( const idRenderModel *source, const char *snapshotName, const bool dupData = true );

	/**
	* Given two render models, adds all surfaces of the first the second. Returns some
	* struct with info about the operation, which can be used to later sep. the models
	* again with RemoveModelParts().
	*/
	model_combineinfo_t		CombineModels( const idRenderModel *source, const idRenderModel *target );

	/**
	* Given the info CombineModels(), sep. the given model out again.
	*/
	void					RemoveModel( const idRenderModel *source, const model_combineinfo_t *info);

	/**
	* Manipulate memory of a duplicate of a model so that the shared data does not get
	* freed twice.
	*/
	void					FreeSharedModelData ( const idRenderModel *model );

private:

};

#endif /* !__GAME_MODELGENERATOR_H__ */

