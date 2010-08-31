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

#ifndef __DAKRMOD_STATICMULTI_H__
#define __DARKMOD_STATICMULTI_H__

#include "../game/misc.h"

/*
===============================================================================

  StaticMulti - An entity with a physics object consisting of multiple
  				clipmodels and a megamodel as rendermodel. Used by the LODE
				system to combine multiple entities.

===============================================================================
*/

class CStaticMulti : public idStaticEntity {
public:
	CLASS_PROTOTYPE( CStaticMulti );

						CStaticMulti( void );

//	void				Save( idSaveGame *savefile ) const;
//	void				Restore( idRestoreGame *savefile );

//	void				Spawn( void );
//	void				ShowEditingDialog( void );
//	virtual void		Hide( void );
//	virtual void		Show( void );
//	void				Fade( const idVec4 &to, float fadeTime );
	virtual void		Think( void );

//	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
//	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	void				Event_Activate( idEntity *activator );
	
//	int						spawnTime;
	bool					active;
//	idVec4					fadeFrom;
//	idVec4					fadeTo;
//	int						fadeStart;
//	int						fadeEnd;
//	bool					runGui;
	idPhysics_StaticMulti		physics;
};

#endif /* !__DARKMOD_STATICMULTI_H__ */

