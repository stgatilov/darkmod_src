// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4618 $
 * $Date: 2011-02-20 19:10:42 +0100 (Sun, 20 Feb 2011) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2004 Id Software, Inc.
// Copyright (C) 2011 The Dark Mod

#ifndef __GAME_TDM_EMITTER_H__
#define __GAME_TDM_EMITTER_H__

#include "misc.h"

/*
===============================================================================

idFuncEmitter

===============================================================================
*/

class idFuncEmitter : public idStaticEntity {
public:
	CLASS_PROTOTYPE( idFuncEmitter );

						idFuncEmitter( void );
						~idFuncEmitter( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	void				Event_Activate( idEntity *activator );

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void		Think( void );
	virtual void		Present( void );

	// switch to a new model
	virtual void		SetModel( const char *modelname );

protected:

	// add an extra model
	void				SetModel( int id, const idStr &modelName, const idVec3 &offset );

private:
	bool				hidden;

	idList<int>				m_modelDefHandles;	// modelDefHandle for each extra model
	idList<idVec3>			m_modelOffsets;		// offset for each extra model (relative to our origin)
	idList<idRenderModel *>	m_modelHandles;		// handles of the extra models
};

/*
===============================================================================

idFuncSplat

===============================================================================
*/

class idFuncSplat : public idFuncEmitter {
public:
	CLASS_PROTOTYPE( idFuncSplat );

	idFuncSplat( void );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_Splat();
};


#endif /* !__GAME_TDM_EMITTER_H__ */

