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

#ifndef __R_IMAGE_H__
#define __R_IMAGE_H__

/*
====================================================================

IMAGE

idImage have a one to one correspondance with OpenGL textures.

No texture is ever used that does not have a corresponding idImage.

no code outside this unit should call any of these OpenGL functions:

qglGenTextures
qglDeleteTextures
qglBindTexture

qglTexParameter

qglTexImage
qglTexSubImage

qglCopyTexImage
qglCopyTexSubImage

qglEnable( GL_TEXTURE_* )
qglDisable( GL_TEXTURE_* )

====================================================================
*/

#define	MAX_TEXTURE_LEVELS				14

// surface description flags
const unsigned int DDSF_CAPS           = 0x00000001l;
const unsigned int DDSF_HEIGHT         = 0x00000002l;
const unsigned int DDSF_WIDTH          = 0x00000004l;
const unsigned int DDSF_PITCH          = 0x00000008l;
const unsigned int DDSF_PIXELFORMAT    = 0x00001000l;
const unsigned int DDSF_MIPMAPCOUNT    = 0x00020000l;
const unsigned int DDSF_LINEARSIZE     = 0x00080000l;
const unsigned int DDSF_DEPTH          = 0x00800000l;

// pixel format flags
const unsigned int DDSF_ALPHAPIXELS    = 0x00000001l;
const unsigned int DDSF_FOURCC         = 0x00000004l;
const unsigned int DDSF_RGB            = 0x00000040l;
const unsigned int DDSF_RGBA           = 0x00000041l;

// dwCaps1 flags
const unsigned int DDSF_COMPLEX         = 0x00000008l;
const unsigned int DDSF_TEXTURE         = 0x00001000l;
const unsigned int DDSF_MIPMAP          = 0x00400000l;

#define DDS_MAKEFOURCC(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwFourCC;
	unsigned int dwRGBBitCount;
	unsigned int dwRBitMask;
	unsigned int dwGBitMask;
	unsigned int dwBBitMask;
	unsigned int dwABitMask;
} ddsFilePixelFormat_t;

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwHeight;
	unsigned int dwWidth;
	unsigned int dwPitchOrLinearSize;
	unsigned int dwDepth;
	unsigned int dwMipMapCount;
	unsigned int dwReserved1[11];
	ddsFilePixelFormat_t ddspf;
	unsigned int dwCaps1;
	unsigned int dwCaps2;
	unsigned int dwReserved2[3];
} ddsFileHeader_t;

struct MakeAmbientMapParam {
	byte **buffers;
	byte *outBuffer;
	int outSize;
	int samples;
	int size;
	int crutchUp;
	bool specular;
	int side;
};

// increasing numeric values imply more information is stored
typedef enum {
	TD_SPECULAR,			// may be compressed, and always zeros the alpha channel
	TD_DIFFUSE,				// may be compressed
	TD_DEFAULT,				// will use compressed formats when possible
	TD_BUMP,				// may be compressed with 8 bit lookup
	TD_HIGH_QUALITY			// either 32 bit or a component format, no loss at all
} textureDepth_t;

typedef enum {
	TT_DISABLED,
	TT_2D,
	TT_CUBIC,
} textureType_t;

typedef enum {
	CF_2D,			// not a cube map
	CF_NATIVE,		// _px, _nx, _py, etc, directly sent to GL
	CF_CAMERA		// _forward, _back, etc, rotated and flipped as needed before sending to GL
} cubeFiles_t;

#define	MAX_IMAGE_NAME	256
#define MIN_IMAGE_NAME  4

typedef enum {
	IR_NONE = 0,			// should never happen
	IR_GRAPHICS = 0x1,
	IR_CPU = 0x2,
	IR_BOTH = IR_GRAPHICS | IR_CPU,
} imageResidency_t;

typedef enum {
	IS_NONE,		// empty/foreground load
	IS_SCHEDULED,	// waiting in queue
	IS_LOADED		// data loaded, waiting for GL thread
} imageLoadState_t;

// stgatilov: represents uncompressed image data on CPU side in RGBA8 format
typedef struct imageBlock_s {
	byte *pic[6];
	int width;
	int height;
	int sides;			//six for cubemaps, one for the others

	byte *GetPic(int side = 0) const { return pic[side]; }
	bool IsValid() const { return pic[0] != nullptr; }
	bool IsCubemap() const { return sides == 6; }
	int GetSizeInBytes() const { return width * height * 4; }
	int GetTotalSizeInBytes() const { return sides * GetSizeInBytes(); }
	void Purge();
} imageBlock_t;

// stgatilov: represents compressed texture as contents of DDS file
typedef struct imageCompressedData_s {
	int fileSize;				//size of tail starting from "magic"
	int _;						//(padding)

	//----- data below is stored in DDS file -----
	dword magic;				//always must be "DDS "in little-endian
	ddsFileHeader_t header;		//DDS file header (124 bytes)
	byte contents[1];			//the rest of file (variable size)

	byte *GetFileData() const { return (byte*)&magic; }
	static int FileSizeFromContentSize(int contentSize) { return contentSize + 4 + sizeof(ddsFileHeader_t); }
	static int TotalSizeFromFileSize(int fileSize) { return fileSize + 8; }
	static int TotalSizeFromContentSize(int contentSize) { return TotalSizeFromFileSize(FileSizeFromContentSize(contentSize)); }
	int GetTotalSize() const { return TotalSizeFromFileSize(fileSize); }
	byte *ComputeUncompressedData() const;
} imageCompressedData_t;
static_assert(offsetof(imageCompressedData_s, contents) - offsetof(imageCompressedData_s, magic) == 128, "Wrong imageCompressedData_t layout");

class LoadStack;

class idImage {
public:
	idImage();

	// Makes this image active on the current GL texture unit.
	// automatically enables or disables cube mapping or texture3D
	// May perform file loading if the image was not preloaded.
	// May start a background image read.
	void		Bind();

	// checks if the texture is currently bound to the specified texture unit
	bool		IsBound( int textureUnit ) const;

	// deletes the texture object, but leaves the structure so it can be reloaded
	void		PurgeImage( bool purgeCpuData = true );

	// used by callback functions to specify the actual data
	// data goes from the bottom to the top line of the image, as OpenGL expects it
	// These perform an implicit Bind() on the current texture unit
	// FIXME: should we implement cinematics this way, instead of with explicit calls?
	void		GenerateImage( const byte *pic, int width, int height,
	                           textureFilter_t filter, bool allowDownSize,
	                           textureRepeat_t repeat, textureDepth_t depth,
	                           imageResidency_t residency = IR_GRAPHICS );
	void		GenerateCubeImage( const byte *pic[6], int size,
	                               textureFilter_t filter, bool allowDownSize,
	                               textureDepth_t depth );
	void		GenerateAttachment( int width, int height, GLenum format,
									GLenum filter = GL_LINEAR, GLenum wrapMode = GL_CLAMP_TO_EDGE );

	void		UploadScratch( const byte *pic, int width, int height );

	// just for resource tracking
	void		SetClassification( int tag );

	// estimates size of the GL image based on dimensions and storage type
	int			StorageSize() const;

	// print a one line summary of the image
	void		Print() const;

	// check for changed timestamp on disk and reload if necessary
	void		Reload( bool checkPrecompressed, bool force );

	void		AddReference()				{ refCount++; };

	//==========================================================

	void		GetDownsize( int &scaled_width, int &scaled_height ) const;
	void		MakeDefault();	// fill with a grid pattern
	void		SetImageFilterAndRepeat() const;
	void		WritePrecompressedImage();
	bool		CheckPrecompressedImage( bool fullLoad );
	void		UploadPrecompressedImage( void );
	void		ActuallyLoadImage( bool allowBackground = false );
	static int			BitsForInternalFormat( int internalFormat );
	GLenum		SelectInternalFormat( const byte **dataPtrs, int numDataPtrs, int width, int height, textureDepth_t minimumDepth ) const;
	void		ImageProgramStringToCompressedFileName( const char *imageProg, char *fileName ) const;
	int			NumLevelsForImageSize( int width, int height ) const;

	// data commonly accessed is grouped here
	static const int TEXTURE_NOT_LOADED = -1;
	GLuint				texnum;					// gl texture binding, will be TEXTURE_NOT_LOADED if not loaded
	textureType_t		type;
	int					frameUsed;				// for texture usage in frame statistics
	int					bindCount;				// incremented each bind

	// parameters that define this image
	idStr				imgName;				// game path, including extension (except for cube maps), may be an image program
	void	( *generatorFunction )( idImage *image );	// NULL for files
	bool				allowDownSize;			// this also doubles as a don't-partially-load flag
	textureFilter_t		filter;
	textureRepeat_t		repeat;
	textureDepth_t		depth;
	cubeFiles_t			cubeFiles;				// determines the naming and flipping conventions for the six images

	bool				referencedOutsideLevelLoad;
	bool				levelLoadReferenced;	// for determining if it needs to be purged
	bool				precompressedFile;		// true when it was loaded from a .d3t file
	bool				defaulted;				// true if the default image was generated because a file couldn't be loaded
	ID_TIME_T			timestamp;				// the most recent of all images used in creation, for reloadImages command

	int					imageHash;				// for identical-image checking

	int					classification;			// just for resource profiling

	// data for listImages
	int					uploadWidth, uploadHeight;	// after power of two, downsample, and MAX_TEXTURE_SIZE
	int					internalFormat;
	const GLint *		swizzleMask;			// replacement for deprecated intensity/luminance formats

	idImage *			hashNext;				// for hash chains to speed lookup

	int					refCount;				// overall ref count

	// stgatilov: storing image data on CPU side
	imageBlock_t		cpuData;				// CPU-side usable image data (usually absent)
	imageResidency_t	residency;				// determines whether cpuData and/or texnum should be valid
	imageCompressedData_t *compressedData;		// CPU-side compressed texture contents (aka DDS file)
	imageLoadState_t	backgroundLoadState;	// state of background loading (usually disabled)

	//stgatilov: information about why and how this image was loaded (may be missing)
	LoadStack *			loadStack;

	// START bindless texture support
private:
	GLuint64			textureHandle;
	bool				isBindlessHandleResident;
public:
	bool				isImmutable;
	int					lastNeededInFrame;
	void				MakeResident();
	void				MakeNonResident();
	GLuint64			BindlessHandle();
};


class idImageManager {
public:
	void				Init();
	void				Shutdown();

	// If the exact combination of parameters has been asked for already, an existing
	// image will be returned, otherwise a new image will be created.
	// Be careful not to use the same image file with different filter / repeat / etc parameters
	// if possible, because it will cause a second copy to be loaded.
	// If the load fails for any reason, the image will be filled in with the default
	// grid pattern.
	// Will automatically resample non-power-of-two images and execute image programs if needed.
	idImage *			ImageFromFile( const char *name,
	                                   textureFilter_t filter, bool allowDownSize,
	                                   textureRepeat_t repeat, textureDepth_t depth, cubeFiles_t cubeMap = CF_2D,
	                                   imageResidency_t residency = IR_GRAPHICS );

	// look for a loaded image, whatever the parameters
	idImage *			GetImage( const char *name ) const;

	// The callback will be issued immediately, and later if images are reloaded or vid_restart
	// The callback function should call one of the idImage::Generate* functions to fill in the data
	idImage *			ImageFromFunction( const char *name, void ( *generatorFunction )( idImage *image ) );

	// returns the number of bytes of image data bound in the previous frame
	int					SumOfUsedImages( int *numberOfUsed = nullptr );

	// called each frame to allow some cvars to automatically force changes
	void				CheckCvars();

	// purges all the images before a vid_restart
	void				PurgeAllImages();

	// reloads all apropriate images after a vid_restart
	void				ReloadAllImages();

	// Mark all file based images as currently unused,
	// but don't free anything.  Calls to ImageFromFile() will
	// either mark the image as used, or create a new image without
	// loading the actual data.
	// Called only by renderSystem::BeginLevelLoad
	void				BeginLevelLoad();

	// Free all images marked as unused, and load all images that are necessary.
	// This architecture prevents us from having the union of two level's
	// worth of data present at one time.
	// Called only by renderSystem::EndLevelLoad
	void				EndLevelLoad();

	// used to clear and then write the dds conversion batch file
	void				StartBuild();
	void				FinishBuild( bool removeDups = false );
	void				AddDDSCommand( const char *cmd );

	void				PrintMemInfo( MemInfo_t *mi );

	// cvars
	static idCVar		image_colorMipLevels;		// development aid to see texture mip usage
	static idCVar		image_downSize;				// controls texture downsampling
	static idCVar		image_useCompression;		// 0 = force everything to high quality
	static idCVar		image_filter;				// changes texture filtering on mipmapped images
	static idCVar		image_anisotropy;			// set the maximum texture anisotropy if available
	static idCVar		image_lodbias;				// change lod bias on mipmapped images
	static idCVar		image_usePrecompressedTextures;	// use .dds files if present
	static idCVar		image_writePrecompressedTextures; // write .dds files if necessary
	static idCVar		image_writeNormalTGA;		// debug tool to write out .tgas of the final normal maps
	static idCVar		image_writeTGA;				// debug tool to write out .tgas of the non normal maps
	static idCVar		image_useNormalCompression;	// use RGTC2 compression
	static idCVar		image_useOffLineCompression; // will write a batch file with commands for the offline compression
	static idCVar		image_preload;				// if 0, dynamically load all images
	static idCVar		image_forceDownSize;		// allows the ability to force a downsize
	static idCVar		image_downSizeSpecular;		// downsize specular
	static idCVar		image_downSizeSpecularLimit;// downsize specular limit
	static idCVar		image_downSizeBump;			// downsize bump maps
	static idCVar		image_downSizeBumpLimit;	// downsize bump limit
	static idCVar		image_ignoreHighQuality;	// ignore high quality on materials
	static idCVar		image_downSizeLimit;		// downsize diffuse limit
	static idCVar		image_blockChecksum;		// duplicate check

	// built-in images
	idImage *			defaultImage;
	idImage *			flatNormalMap;				// 128 128 255 in all pixels
	idImage *			ambientNormalMap;			// tr.ambientLightVector encoded in all pixels
	//idImage *			rampImage;					// 0-255 in RGBA in S
	//idImage *			alphaRampImage;				// 0-255 in alpha, 255 in RGB
	idImage *			alphaNotchImage;			// 2x1 texture with just 1110 and 1111 with point sampling
	idImage *			whiteImage;					// full of 0xff
	idImage *			blackImage;					// full of 0x00
	idImage *			normalCubeMapImage;			// cube map to normalize STR into RGB
	idImage *			noFalloffImage;				// all 255, but zero clamped
	idImage *			fogImage;					// increasing alpha is denser fog
	idImage *			fogEnterImage;				// adjust fogImage alpha based on terminator plane
	idImage *			cinematicImage;
	idImage *			scratchImage;
	idImage *			scratchImage2;
	idImage *			xrayImage;
	idImage *			accumImage;
	idImage *			currentRenderImage;			// for SS_POST_PROCESS shaders
	idImage *			guiRenderImage;
	idImage *			scratchCubeMapImage;
	//idImage *			specularTableImage;			// 1D intensity texture with our specular function
	//idImage *			specular2DTableImage;		// 2D intensity texture with our specular function with variable specularity
	//idImage *			borderClampImage;			// white inside, black outside


	idImage *			currentDepthImage;			// #3877. Allow shaders to access scene depth
	idImage *			shadowDepthFbo;
	idImage *			shadowAtlas;
	//idImage *			shadowAtlasHistory;
	idImage *			currentStencilFbo; // these two are only used on Intel since no one else support separate stencil

	//--------------------------------------------------------

	idImage *			AllocImage( const char *name );
	void				SetNormalPalette();
	void				ChangeTextureFilter();

	idList<idImage*>	images;
	idStrList			ddsList;
	idHashIndex			ddsHash;

	bool				insideLevelLoad;			// don't actually load images now

	byte				originalToCompressed[256];	// maps normal maps to 8 bit textures
	byte				compressedPalette[768];		// the palette that normal maps use

	// default filter modes for images
	GLenum				textureMinFilter;
	GLenum				textureMaxFilter;
	float				textureAnisotropy;
	float				textureLODBias;

	idImage *			imageHashTable[FILE_HASH_SIZE];

	void				MakeUnusedImagesNonResident();
};

extern idImageManager	*globalImages;		// pointer to global list for the rest of the system

/*
====================================================================

IMAGEPROCESS

FIXME: make an "imageBlock" type to hold byte*,width,height?
====================================================================
*/

byte *R_Dropsample( const byte *in, int inwidth, int inheight,
                    int outwidth, int outheight );
byte *R_ResampleTexture( const byte *in, int inwidth, int inheight,
                         int outwidth, int outheight );
byte *R_MipMap( const byte *in, int width, int height );

// these operate in-place on the provided pixels
void R_SetBorderTexels( byte *inBase, int width, int height, const byte border[4] );
void R_BlendOverTexture( byte *data, int pixelCount, const byte blend[4] );
void R_HorizontalFlip( byte *data, int width, int height );
void R_VerticalFlip( byte *data, int width, int height );
void R_RotatePic( byte *data, int width );

/*
====================================================================

IMAGEFILES

====================================================================
*/

void R_LoadImage( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp );
void R_LoadCompressedImage( const char *name, imageCompressedData_t **pic, ID_TIME_T *timestamp );
// pic is in top to bottom raster format
bool R_LoadCubeImages( const char *cname, cubeFiles_t extensions, byte *pic[6], int *size, ID_TIME_T *timestamp );
void R_MakeAmbientMap( MakeAmbientMapParam param );
void R_LoadImageData( idImage &image );
void R_RGBA8Image( idImage* image );

/*
====================================================================

IMAGEPROGRAM

====================================================================
*/

void R_LoadImageProgram( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp, textureDepth_t *depth = NULL );
const char *R_ParsePastImageProgram( idLexer &src );

/*
====================================================================

IMAGE READER/WRITER

====================================================================
*/

// data is RGBA
void R_WriteTGA( const char* filename, const byte* data, int width, int height, bool flipVertical = false );

class idImageWriter {
public:
	//setting common settings
	inline idImageWriter &Source(const byte *data, int width, int height, int bpp = 4) {
		srcData = data;
		srcWidth = width;
		srcHeight = height;
		srcBpp = bpp;
		return *this;
	}
	inline idImageWriter &Dest(idFile *file, bool close = true) {
		dstFile = file;
		dstClose = close;
		return *this;
	}
	inline idImageWriter &Flip(bool doFlip = true) {
		flip = doFlip;
		return *this;
	}
	//perform save (final call)
	bool WriteTGA();
	bool WriteJPG(int quality = 85);
	bool WritePNG(int level = -1);
	bool WriteExtension(const char *extension);

private:
	bool Preamble();
	void Postamble();

	const byte *srcData = nullptr;
	int srcWidth = -1, srcHeight = -1, srcBpp = -1;
	idFile *dstFile = nullptr;
	bool dstClose = true;
	bool flip = false;
};

class idImageReader {
public:
	//setting common settings
	inline idImageReader &Source(idFile *file, bool close = true) {
		srcFile = file;
		srcClose = close;
		return *this;
	}
	inline idImageReader &Dest(byte* &data, int &width, int &height) {
		dstData = &data;
		dstWidth = &width;
		dstHeight = &height;
		return *this;
	}
	//perform load (final call)
	void LoadTGA();
	void LoadJPG();
	void LoadPNG();
	void LoadExtension(const char *extension = nullptr);

private:
	bool Preamble();
	void Postamble();
	void LoadBMP();

	idFile *srcFile = nullptr;
	bool srcClose = true;
	byte* *dstData = nullptr;
	int *dstWidth = nullptr, *dstHeight = nullptr;
	byte *srcBuffer = nullptr;
	int srcLength = 0;
};

#endif /* !__R_IMAGE_H__ */
