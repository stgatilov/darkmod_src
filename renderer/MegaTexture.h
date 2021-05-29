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

class idTextureTile {
public:
	int		x, y;
};

static const int TILE_PER_LEVEL = 4;
static const int MAX_MEGA_CHANNELS = 3;		// normal, diffuse, specular
static const int MAX_LEVELS = 12;
static const int MAX_LEVEL_WIDTH = 512;
static const int TILE_SIZE = MAX_LEVEL_WIDTH / TILE_PER_LEVEL;

class	idMegaTexture;

class idTextureLevel {
public:
	idMegaTexture	*mega;

	int				tileOffset;
	int				tilesWide;
	int				tilesHigh;

	idImage			*image;
	idTextureTile	tileMap[TILE_PER_LEVEL][TILE_PER_LEVEL];

	float			parms[4];

	void			UpdateForCenter( float center[2] );
	void			UpdateTile( int localX, int localY, int globalX, int globalY );
	void			Invalidate();
};

typedef struct {
	int		tileSize;
	int		tilesWide;
	int		tilesHigh;
} megaTextureHeader_t;


class idMegaTexture {
public:
	bool	InitFromMegaFile( const char *fileBase );
	void	SetMappingForSurface( const srfTriangles_t *tri );	// analyzes xyz and st to create a mapping
	//void	BindForViewOrigin( const idVec3 origin );	// binds images and sets program parameters

	static	void MakeMegaTexture_f( const idCmdArgs &args );
private:
friend class idTextureLevel;
	void	SetViewOrigin( const idVec3 origin );
	static void	GenerateMegaMipMaps( megaTextureHeader_t *header, idFile *file );
	static void	GenerateMegaPreview( const char *fileName );

	idFile			*fileHandle;

	const srfTriangles_t *currentTriMapping;

	idVec3			currentViewOrigin;

	float			localViewToTextureCenter[2][4];

	int				numLevels;
	idTextureLevel	levels[MAX_LEVELS];				// 0 is the highest resolution
	megaTextureHeader_t	header;

	static idCVar	r_megaTextureLevel;
	static idCVar	r_showMegaTexture;
	static idCVar	r_showMegaTextureLabels;
	static idCVar	r_skipMegaTexture;
	static idCVar	r_terrainScale;
};

