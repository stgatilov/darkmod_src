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

#include "precompiled.h"
#pragma hdrstop

#include "Image.h"
#include "../ExtLibs/devil.h"

#define IL_IMAGE_NONE ((ILuint)-1)

Image::Image() :
	m_ImageId(IL_IMAGE_NONE),
	m_Width(0),
	m_Height(0),
	m_Bpp(0)
{}

Image::Image(const idStr& name) :
	m_ImageId(IL_IMAGE_NONE),
	m_Name(name),
	m_Width(0),
	m_Height(0),
	m_Bpp(0)
{}

Image::~Image(void)
{
	Unload();
}

ILenum Image::GetILTypeForImageFormat(Format format)
{
	switch (format)
	{
	case AUTO_DETECT:	return IL_TYPE_UNKNOWN;
	case TGA:			return IL_TGA;
	case PNG:			return IL_PNG;
	case JPG:			return IL_JPG;
	case GIF:			return IL_GIF;
	case BMP:			return IL_BMP;
	default:			return IL_TYPE_UNKNOWN;
	};
}

Image::Format Image::GetFormatFromString(const char *format) {
	if (idStr::Icmp(format, "tga") == 0) return TGA;
	if (idStr::Icmp(format, "png") == 0) return PNG;
	if (idStr::Icmp(format, "jpg") == 0) return JPG;
	if (idStr::Icmp(format, "gif") == 0) return GIF;
	if (idStr::Icmp(format, "bmp") == 0) return BMP;
	//return unkonwn format as default
	return AUTO_DETECT;
}

void Image::Unload()
{
	if(m_ImageId != IL_IMAGE_NONE) {
		ilDeleteImages(1, &m_ImageId);
		m_ImageId = IL_IMAGE_NONE;
	}
	m_Width = m_Height = m_Bpp = 0;
}

bool Image::Init(int width, int height, int bpp)
{
	if (m_Width == width && m_Height == height && m_Bpp == bpp && m_ImageId != IL_IMAGE_NONE)
	{
		return true; // nothing to do
	}

	// Dimensions have changed or no image has been loaded
	Unload();

	// Generate new image object
	ilGenImages(1, &m_ImageId);
	ilBindImage(m_ImageId);

	ILenum format = IL_RGB;

	if (bpp == 3)
	{
		format = IL_RGB;
	}
	else if (bpp == 4)
	{
		format = IL_RGBA;
	}
	else
	{
		common->Warning("Invalid BPP value %d sent to Image::Init.", bpp);
		Unload();
		return false;
	}

	// Generate a new image using these values
	ILboolean result = ilTexImage(
		static_cast<ILuint>(width), static_cast<ILuint>(height), 1, static_cast<ILuint>(bpp),
		format, IL_UNSIGNED_BYTE, NULL); // no new data to be copied into the image, just allocate the buffer
	
	if (result == IL_FALSE)
	{
		common->Warning("Could not generate image: error message %s.", ilGetString(ilGetError()));
		Unload();
		return false;
	}

	m_Width = width;
	m_Height = height;
	m_Bpp = bpp;

	return true;
}

bool Image::LoadDevILFromLump(const unsigned char *imageBuffer, unsigned int imageLength) {
	// generate new DevIL image
	ilGenImages(1, &m_ImageId);
	ilBindImage(m_ImageId);

	// try to load DevIL image from lump
	if (ilLoadL(GetILTypeForImageFormat(AUTO_DETECT), imageBuffer, imageLength) == IL_FALSE)
	{
		// loading failed: print log message and free DevIL image
		common->Warning("Could not load image (name = %s): error message %s.", m_Name.c_str(), ilGetString(ilGetError()));
		Unload();
		return false;
	}

	// get image parameters
	m_Width = ilGetInteger(IL_IMAGE_WIDTH);
	m_Height = ilGetInteger(IL_IMAGE_HEIGHT);
	m_Bpp = ilGetInteger(IL_IMAGE_BPP);

	return true;
}

bool Image::SaveDevILToFile(const char *filename, Format format) const {
	// check DevIL image for existence
	if (m_ImageId == IL_IMAGE_NONE)
	{
		common->Warning("Cannot save image before loading data (%s).", filename);
		return false;
	}
	// bind DevIL image
	ilBindImage(m_ImageId);

	// set overwrite option
	ilEnable(IL_FILE_OVERWRITE);
	//DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("Saving image to path: %s, type %d.\r", filename, format);

	// try to save DevIL image to file
	idStr filenameClone(filename);

	if (!ilSave(GetILTypeForImageFormat(format), const_cast<char *>(filenameClone.c_str())))
	{
		common->Warning("Could not save image to path %s (type %d): error message %s.", filename, format, ilGetString(ilGetError()));
		return false;
	}

	return true;
}


bool Image::LoadImageFromMemory(const unsigned char *imageBuffer, unsigned int imageLength, const char *name)
{
	//unload previous image
	Unload();
	if ( !imageBuffer ) {
		return false;
	}

	//set name to user-specified string
	m_Name = name;

	//load DevIL
	return LoadDevILFromLump(imageBuffer, imageLength);
}

bool Image::LoadImageFromVfs(const char* filename)
{
	//unload previous image
	Unload();
	if ( !filename ) {
		return false;
	}

	//set name to vfs filename
	m_Name = filename;

	//try to open file
	idFile *file = fileSystem->OpenFileRead(m_Name);
	if ( !file )
	{
		common->Warning("Unable to load imagefile [%s]", m_Name.c_str());
		return false;
	}

	//read the whole file to buffer
	idList<unsigned char> fileData;
	fileData.SetNum(file->Length());
	file->Read(&fileData[0], fileData.Num());
	//close file
	fileSystem->CloseFile(file);

	//load DevIL
	return LoadDevILFromLump(&fileData[0], fileData.Num());
}

bool Image::LoadImageFromFile(const fs::path& path)
{
	//unload previous image
	Unload();

	//set name to std::filesystem filename
	m_Name = path.string().c_str();

	//try to open file
	FILE* file = fopen(path.string().c_str(), "rb");
	if ( !file || !fs::exists(path) )
	{
		common->Warning("Unable to load imagefile [%s]", m_Name.c_str());
		return false;
	}

	//read the whole file to buffer
	idList<unsigned char> fileData;
	fileData.SetNum(fs::file_size(path));
	fread(&fileData[0], 1, fileData.Num(), file);
	//close file
	fclose(file);

	//load DevIL
	return LoadDevILFromLump(&fileData[0], fileData.Num());
}

unsigned int Image::GetDataLength() const
{
	if (m_ImageId != IL_IMAGE_NONE)
	{
		ilBindImage(m_ImageId);
		return static_cast<unsigned int>(ilGetInteger(IL_IMAGE_SIZE_OF_DATA));
	}

	return 0;
}

unsigned char* Image::GetImageData()
{
	if (m_ImageId != IL_IMAGE_NONE)
	{
		ilBindImage(m_ImageId);
		return static_cast<unsigned char*>(ilGetData());
	}

	return NULL;
}

bool Image::SaveImageToFile(const fs::path& path, Format format) const
{
	if (fs::is_directory(path))
	{
		common->Warning("Cannot save image: file [%s] is directory", path.string().c_str());
		return false;
	}

	//create directories if necessary
	fs::create_directories(path.parent_path());
	//write image file
	return SaveDevILToFile(path.string().c_str(), format);
}

bool Image::SaveImageToVfs(const char* filename, Format format) const
{
	//create directories if necessary
	fileSystem->CloseFile(fileSystem->OpenFileWrite(filename, "fs_savepath", ""));
	//write image file
	return SaveDevILToFile(fileSystem->RelativePathToOSPath(filename, "fs_savepath", ""), format);
}
