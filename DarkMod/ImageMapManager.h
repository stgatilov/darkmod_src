// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4069 $
 * $Date: 2010-07-18 13:07:48 +0200 (Sun, 18 Jul 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

// Copyright (C) 2011 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_IMAGEMAPMANAGER_H__
#define __DARKMOD_IMAGEMAPMANAGER_H__

// to get CImage
#include "../DarkMod/CImage.h"

/*
===============================================================================

  ImageMap Manager - manages image maps used by the SEEDsystem

  This class is a singleton and initiated/destroyed from gameLocal.

  Image maps (usually grayscale images) are loaded only once and the can
  be shared between different SEED entities.

===============================================================================
*/

// Defines data for one image map
typedef struct {
	idStr				name;		//< the filename from where the image was loaded
	CImage*				img;		//< The CImage object that loads and contains the actual data
	float				density;	//< average density (0..1.0)
} imagemap_t;

class CImageMapManager {
public:

						CImageMapManager( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	* Called by gameLocal.
	*/
	void				Init ( void );
	void				Shutdown ( void );
	void				Clear ( void );

	/**
	* Given a name of the image map (either full filename, or partial name), tries to
    * load the corrosponding file and returns the index of the map for later access.
	* Returns -1 in case of errors (like file not found).
	*/
	int					GetImageMap( idStr mapname );

	/**
	* Given the id of a formerly loaded map, returns a ptr to the given raw image data.
	* If the map was previously freed, or never loaded, returns NULL.
	*/
	unsigned char*		GetMapData( const unsigned int id );

	/**
	* Returns the width in pixels of the image map.
	*/
	unsigned int		GetMapWidth( const unsigned int id );

	/**
	* Returns the width in pixels of the image map.
	*/
	unsigned int		GetMapHeight( const unsigned int id );

	/**
	* Given the id of a formerly loaded map, returns a value between 0 and 255 for the
	* position on the map. 
	*/
	unsigned int		GetMapDataAt( const unsigned int id, const unsigned int x, const unsigned int y);

	/**
	* Frees the formerly loaded map data. Returns -1 on error, 0 on success.
	*/
	unsigned int		FreeMap( const unsigned int id );

private:

	// List of loaded image maps
	idList< imagemap_t >	m_imageMaps;

};

#endif /* !__DARKMOD_MODELGENERATOR_H__ */

