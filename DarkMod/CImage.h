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
	enum Format
	{
		AUTO_DETECT = 0,	//only for loading!
		TGA,
		PNG,
		JPG,
		GIF,
		BMP,
	};
	/**
	 * Return image format given by string.
	 */
	static Format GetFormatFromString(const char *format);


	CImage();
	CImage(const idStr& name);
	
	~CImage();
	/**
	 * Unload will set the image to not loaded and deallocate memory.
	 */
	void Unload();

	/**
	 * Load the image from the given file (loaded via D3's filesystem) into memory 
	 * and allow access to it. Any previous image is unloaded.
	 */
	bool LoadImageFromVfs(const char* filename = NULL);
	/**
	 * Load the image from the given file (absolute OS path) into memory 
	 * and allow access to it. Any previous image is unloaded.
	 */
	bool LoadImageFromFile(const boost::filesystem::path& path);
	/**
	 * Load the image from memory buffer into memory 
	 * and allow access to it. Any previous image is unloaded.
	 * Image name is set explicitly.
	 */
	bool LoadImageFromMemory(const unsigned char *imageBuffer, unsigned int imageLength, const char *name = NULL);

	/**
	 * Saves the image to the given file (absolute OS path).
	 * Existing target file will be overwritten without further confirmation.
	 */
	bool SaveImageToFile(const boost::filesystem::path& path, Format format = TGA) const;
	/**
	 * Saves the image to the given file (filename as in D3's filesystem).
	 * Existing target file will be overwritten without further confirmation.
	 */
	bool SaveImageToVfs(const char* filename, Format format = TGA) const;

	/**
	 * GetImage returns the pointer to the actual image data.
	 * The image has to be already loaded, otherwise NULL is returned.
	 */
	const unsigned char* GetImageData() const;
	/**
	 * Returns the buffer length of the loaded image data in bytes.
	 * The data is uncompressed and without header - pure pixel data.
	 * Each pixel will have m_Bpp bytes (1 or 3 or 4, depending on format), and there are m_Width * m_Height pixel.
	 */
	unsigned int GetDataLength() const;

protected:
	// DevIL image ID (equal to -1 if image is not loaded)
	ILuint			m_ImageId;

	// Convert CImage::Format to ILenum
	static ILenum GetILTypeForImageFormat(Format format);

	bool LoadDevILFromLump(const unsigned char *imageBuffer, unsigned int imageLength);
	bool SaveDevILToFile(const char *filename, Format format) const;

public:
	idStr			m_Name;
	int				m_Width;	//!< Image width in pixel
	int				m_Height;	//!< Image height in pixel
	int				m_Bpp;		//!< Bytes per pixel (not bits!)
};

#endif
