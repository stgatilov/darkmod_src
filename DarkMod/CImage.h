/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef DARKMODCIMAGE_H
#define DARKMODCIMAGE_H

//TODO(stgatilov): Do not store m_ImageBuffer data at all.
//It can be be discarded after loading because IL stores all the necessary data for image
//Unless it is deleted of course.

#if defined(__linux__) || defined(MACOS_X)
#include "idlib/lib.h"
#include "sound/sound.h"
#endif

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#ifndef ILuint
typedef unsigned int ILuint;
#endif

#ifndef ILenum
typedef unsigned int ILenum;
#endif

class CImage
{
public:
	enum Type
	{
		AUTO_DETECT = 0,
		TGA,
		PNG,
		JPG,
		GIF,
	};

	CImage();
	CImage(const idStr& name);
	
	~CImage();

	/**
	 * Call this to let the image library assume a certain filetype when loading images
	 * from files or buffers.
	 */
	void SetDefaultImageType(Type type);

	/**
	 * Load the image from the given file (loaded via D3's filesystem) into memory 
	 * and allow access to it. If the filename is not NULL, it is assumed that a new 
	 * image is to be loaded from disk and the any previous one is unloaded.
	 */
	bool LoadImageFromVfs(const char* filename = NULL);

	/**
	 * Load the image from the given file (absolute OS paths) into memory 
	 * and allow access to it. If the filename is not NULL, it is assumed that a new 
	 * image is to be loaded from disk and the any previous one is unloaded.
	 */
	bool LoadImageFromFile(const boost::filesystem::path& path);

	/**
	 * Load the image into memory and allow access to it, reading the image
	 * data from memory buffer.
	 */
	bool LoadImage(const idList<char> &imageBuffer);

	/**
	 * greebo: Saves the loaded image to the given path. Existing target files
	 * will be overwritten without further confirmation.
	 */
	bool SaveToFile(const boost::filesystem::path& path, Type type = TGA);

	/**
	 * Initialize Imageinfo like bitmap width, height and other stuff.
	 */
	void InitImageInfo();

	/**
	 * GetImage returns the pointer to the actual image data. the image has to be already
	 * loaded, otherwise NULL is returned.
	 */
	unsigned char* GetImageData();

	/**
	 * Returns the buffer length of the loaded image data in bytes.
	 * The data is uncompressed and without header - pure pixel data.
	 * Each pixel will have m_Bpp bytes (1 or 3 or 4, depending on format), and there are m_Width * m_Height pixel.
	 */
	unsigned long GetDataLength();

	/**
	 * Unload will set the image to not loaded.
	 * If FreeMemory == false then the memory is not deallocated.
	 * The next loaded image is loaded into the same memory space.
	 * Do not use it! For renderpipe purpose only.
	 */
	void Unload(bool FreeMemory = true);

protected:
	// Buffer containing the image file contents (including header, compression, etc.)
	unsigned long	m_ImageBufferLength;
	unsigned char*	m_ImageBuffer;

	ILuint			m_ImageId;
	bool			m_Loaded;

	// The image type used when opening files (defaults to IL_TGA)
	ILenum			m_defaultImageType;

	// Convert CImage::Type to ILenum
	ILenum GetILTypeForImageType(Type type);

public:
	idStr			m_Name;
	int				m_Width;	//!< Image width in pixel
	int				m_Height;	//!< Image height in pixel
	int				m_Bpp;		//!< Bytes per pixel (not bits!)
};

#endif
