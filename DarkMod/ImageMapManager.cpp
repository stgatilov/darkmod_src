// vim:ts=4:sw=4:cindent
/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 4071 $
 * $Date: 2010-07-18 15:57:08 +0200 (Sun, 18 Jul 2010) $
 * $Author: tels $
 *
 ***************************************************************************/

/*
	Copyright (C) 2011 Tels (Donated to The Dark Mod Team)

	ImageMapManager

	Image maps (usually grayscale images) are loaded only once and the can
	be shared between different SEED entities.

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: ImageMapManager.cpp 4458 2011-01-23 18:09:01Z tels $", init_version);

#include "ImageMapManager.h"

/*
===============
CImageMapManager::CImageMapManager
===============
*/
CImageMapManager::CImageMapManager( void ) {
	m_imageMaps.Empty();
}

/*
===============
CImageMapManager::Save
===============
*/
void CImageMapManager::Save( idSaveGame *savefile ) const {

	int num = m_imageMaps.Num();
	savefile->WriteInt( num );
	for (int i = 0; i < num; i++)
	{
		savefile->WriteString( m_imageMaps[i].name );
		savefile->WriteFloat( m_imageMaps[i].density );
		// need to load it again in Restore()?
		savefile->WriteBool( m_imageMaps[i].img ? true : false );
	}
}

/*
===============
CImageMapManager::Restore
===============
*/
void CImageMapManager::Restore( idRestoreGame *savefile ) {

	int num;

	// free old image maps
	Clear();

	savefile->ReadInt( num );
	m_imageMaps.Empty();
	m_imageMaps.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString( m_imageMaps[i].name );
		savefile->ReadFloat( m_imageMaps[i].density );
		// need to load it again in Restore()?
		bool needLoad = false;
		savefile->WriteBool( needLoad );
		if (needLoad)
		{
			// TODO:
		}
	}
}

/*
===============
CImageMapManager::Init
===============
*/
void CImageMapManager::Init( void ) {
	Clear();
}

/*
===============
CImageMapManager::Clear
===============
*/
void CImageMapManager::Clear( void ) {

	for (int i = 0; i < num; i++)
	{
		if (m_imageMaps[i].img)
		{
			// TODO:
			free m_imageMaps[i].img;
			m_imageMaps[i].img = NULL;
		}
	}
	m_imageMaps.Empty();
}

/*
===============
CImageMapManager::Shutdown
===============
*/
void CImageMapManager::Shutdown( void ) {
	Clear();
}

/*
===============
CImageMapManager::GetImageMap - Load an image map by name

===============
*/
int CImageMapManager::GetImageMap( idStr mapname )
{
	int handle = -1;

	return handle;
}


/**
* Given the id of a formerly loaded map, returns a ptr to the given raw image data.
* If the map was previously freed, or never loaded, returns NULL.
*/
unsigned char* CImageMapManager::GetMapData( const unsigned int id )
{
	return NULL;
}

/**
* Returns the width in pixels of the image map.
*/
unsigned int CImageMapManager::GetMapWidth( const unsigned int id )
{
	return 0;
}

/**
* Returns the width in pixels of the image map.
*/
unsigned int GetMapHeight( const unsigned int id )
{
	return 0;
}

/**
* Given the id of a formerly loaded map, returns a value between 0 and 255 for the
* position on the map. 
*/
unsigned int GetMapDataAt( const unsigned int id, const unsigned int x, const unsigned int y)
{
	return 0;
}

/**
* Frees the formerly loaded map data. Returns -1 on error, 0 on success.
*/
unsigned int CImageMapManager::FreeMap( const unsigned int id )
{
	return -1;
}

