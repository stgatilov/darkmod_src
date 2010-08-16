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

#ifndef __DAKRMOD_MEGAMODEL_H__
#define __DARKMOD_MEGAMODEL_H__

#include "ModelGenerator.h"

/*
===============================================================================

  Mega Model - Represents a render model generated from many small models with
  			   different LOD stages.

===============================================================================
*/

// Defines info about a change to a combined model. E.g. if a combined model was
// combined from 2 times model A, and we want to change the second model from A
// to B, we use this struct:
typedef struct {
	int				entity;					// the entity index in the offsets list this change applies to
	int				oldLOD;					// the original model combined into the megamodel
	int				newLOD;					// the new model to be combined into the megamodel
} model_changeinfo_t;

class CMegaModel {
public:

	/**
	* Construct the megamodel from the given list of LOD stages and offsets/rotations/colors.
	*/
							CMegaModel( idList<const idRenderModel*>* LODs, idList<model_ofs_t>* offsets, const idVec3 *playerPos, const idVec3 *origin,
										const idMaterial* material = NULL, const int iUpdateTime = 1000 ); 

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/**
	* Returns a pointer to the combined render model.
	*/
	idRenderModel*			GetRenderModel() const;

	/**
	* Marks the model for the entity #entity (offset into Offsets list) to be changed to newLOD.
	*/
	void					AddChange( const int entity, const int newLOD );

	/**
	* If there are enough changes or enough time has passed, updates the render model. Returns true
	* if the update did happen.
	*/
	bool					Update();

	/**
	* The entity presenting/using this model is going to get culled, so stop all updates.
	*/
	void					StopUpdating();

	/**
	* The entity presenting/using this model is going to get culled, so remove all changes.
	*/
	void					ClearChanges();

private:

	idRenderModel*				m_hModel;			//!< ptr to the combined model

	idList<model_ofs_t>			m_Offsets;			//!< list of the individual entity combined into the model

	idList<model_changeinfo_t>	m_Changes;			//!< list with changes

	int							m_iMaxChanges;		//!< maximum number of changes before we update

	int							m_iNextUpdate;		//!< time in ms when the next update should happen
	int							m_iUpdateTime;		//!< time in ms between updates

	bool						m_bActive;			//!< Is actively updating the combined model?

};

#endif /* !__DARKMOD_MEGAMODEL_H__ */

