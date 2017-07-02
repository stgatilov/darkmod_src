// vim:ts=4:sw=4:cindent
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

// Copyright (C) 2011 Tels (Donated to The Dark Mod)

#ifndef __DARKMOD_IMAGEMAPMANAGER_H__
#define __DARKMOD_IMAGEMAPMANAGER_H__

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
	idStr				name;		//!< the filename from where the image was loaded
	Image*				img;		//!< The Image object that loads and contains the actual data
	float				density;	//!< average density (0..1.0)
	unsigned int		users;		//!< How many objects currently use this image? Data can only be freed if users == 0.
} imagemap_t;

class ImageMapManager {
public:

						ImageMapManager( void );

						~ImageMapManager();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	/**
	* Called by gameLocal.
	*/
	void				Init ( void );
	void				Clear ( void );

	/**
	* Given a name of the image map (either full filename, or partial name), tries to
    * load the corrosponding file and returns the index of the map for later access.
	* Returns -1 in case of errors (like file not found).
	* This will increment the users count on the map.
	*/
	int					GetImageMap( idStr mapname );

	/**
	* Given the id of a formerly loaded map, returns a ptr to the given raw image data.
	* If the map was previously freed, or never loaded, returns NULL.
	*/
	const unsigned char*GetMapData( const unsigned int id );

	/**
	* Returns the width in pixels of the image map. Returns 0 in case of error.
	*/
	unsigned int		GetMapWidth( const unsigned int id );

	/**
	* Returns the width in pixels of the image map. Returns 0 in case of error.
	*/
	unsigned int		GetMapHeight( const unsigned int id );

	/**
	* Returns the byte-per-pixel of the image map. Returns 0 in case of error.
	*/
	unsigned int		GetMapBpp( const unsigned int id );

	/**
	* Returns the filename of the map.
	*/
	const char *		GetMapName( const unsigned int id );

	/**
	* Returns the average density of the entire image (0..1.0).
	*/
	float				GetMapDensity( const unsigned int id );

	/**
	* Given the id of a formerly loaded map, returns a value between 0 and 255 for the
	* position on the map. xr and yr run from 0 .. 1.0f;
	*/
	unsigned int		GetMapDataAt( const unsigned int id, const float xr, const float yr);

	/**
	* Decrements the users count on this map. If "users == 0", the map data can be
	* freed later. Return 0 for success, -1 for error (map not found or users already == 0)
	*/
	bool				UnregisterMap( const unsigned int id );

	/**
	* For all maps where users == 0, the map data can be freed.
	*/
	void				FreeUnusedMaps( void );

	const char *		GetLastError( void );

private:

	void				Shutdown();

	/**
	* Checks that the given map handle is valid. Returns ptr to imagemap_t or NULL.
	*/
	imagemap_t*			GetMap( unsigned int handle );

	/**
	* Loads the image from disk, makes sure BPP is 1, and computes the average
	* density. Returns false if the image could not be loaded.
	*/
	bool 				LoadImage( imagemap_t* map );

	/**
	* Assure that the image was allocated and loaded, then return a ptr to it.
	*/
	Image*				GetImage( unsigned int handle );

	/** List of loaded image maps */
	idList< imagemap_t >	m_imageMaps;

	/** Human readable message of the last error that occured */
	idStr					m_lastError;

};

#endif /* !__DARKMOD_IMAGEMAPMANAGER_H__ */

