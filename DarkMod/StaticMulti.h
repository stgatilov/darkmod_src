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
#include "../DarkMod/StimResponse/StimResponseCollection.h"
#include "../DarkMod/MegaModel.h"

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
						virtual ~CStaticMulti();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

	virtual float		ThinkAboutLOD( const lod_data_t* lod_data, const float deltaSq );

	void				SetLODData( CMegaModel* megaModel, lod_data_t *LOD);

//	virtual void		Hide( void );
//	virtual void		Show( void );
	virtual void		Think( void );

//	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
//	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	void				Event_Activate( idEntity *activator );
	
	bool						active;
	idPhysics_StaticMulti		physics;
	CMegaModel*					m_MegaModel;

	// TODO: use these from m_LOD:
	int							m_DistCheckTimeStamp;
	int							m_DistCheckInterval;
	float						m_fHideDistance;
	bool						m_bDistCheckXYOnly;

};

#endif /* !__DARKMOD_STATICMULTI_H__ */

