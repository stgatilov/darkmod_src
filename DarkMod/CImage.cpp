/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include <IL/il.h>

#define IL_IMAGE_NONE ((ILuint)-1)

CImage::CImage() :
	m_ImageId(IL_IMAGE_NONE),
	m_Width(0),
	m_Height(0),
	m_Bpp(0)
{}

CImage::CImage(const idStr& name) :
	m_ImageId(IL_IMAGE_NONE),
	m_Name(name),
	m_Width(0),
	m_Height(0),
	m_Bpp(0)
{}

CImage::~CImage(void)
{
	Unload();
}

ILenum CImage::GetILTypeForImageFormat(Format format)
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

CImage::Format CImage::GetFormatFromString(const char *format) {
	if (idStr::Icmp(format, "tga") == 0) return TGA;
	if (idStr::Icmp(format, "png") == 0) return PNG;
	if (idStr::Icmp(format, "jpg") == 0) return JPG;
	if (idStr::Icmp(format, "gif") == 0) return GIF;
	if (idStr::Icmp(format, "bmp") == 0) return BMP;
	//return unkonwn format as default
	return AUTO_DETECT;
}

void CImage::Unload()
{
	if(m_ImageId != IL_IMAGE_NONE) {
		ilDeleteImages(1, &m_ImageId);
		m_ImageId = IL_IMAGE_NONE;
	}
	m_Width = m_Height = m_Bpp = 0;
}

bool CImage::LoadDevILFromLump(const unsigned char *imageBuffer, unsigned int imageLength) {
	// generate new DevIL image
	ilGenImages(1, &m_ImageId);
	ilBindImage(m_ImageId);

	// try to load DevIL image from lump
	if (ilLoadL(GetILTypeForImageFormat(AUTO_DETECT), imageBuffer, imageLength) == IL_FALSE)
	{
		// loading failed: print log message and free DevIL image
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Could not load image (name = %s): error message %s.\r", m_Name.c_str(), ilGetString(ilGetError()));
		Unload();
		return false;
	}

	// get image parameters
	m_Width = ilGetInteger(IL_IMAGE_WIDTH);
	m_Height = ilGetInteger(IL_IMAGE_HEIGHT);
	m_Bpp = ilGetInteger(IL_IMAGE_BPP);

	return true;
}

bool CImage::SaveDevILToFile(const char *filename, Format format) const {
	// check DevIL image for existence
	if (m_ImageId == IL_IMAGE_NONE)
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Cannot save image before loading data (%s).\r", filename);
		return false;
	}
	// bind DevIL image
	ilBindImage(m_ImageId);

	// set overwrite option
	ilEnable(IL_FILE_OVERWRITE);
	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("Saving image to path: %s, type %d.\r", filename, format);

	// try to save DevIL image to file
	idStr filenameClone(filename);
	if (!ilSave(GetILTypeForImageFormat(format), const_cast<char *>(filenameClone.c_str())))
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Could not save image to path %s (type %d): error message %s.\r", filename, format, ilGetString(ilGetError()));
		return false;
	}

	return true;
}


bool CImage::LoadImageFromMemory(const unsigned char *imageBuffer, unsigned int imageLength, const char *name)
{
	//unload previous image
	Unload();
	if (!imageBuffer)
		return false;

	//set name to user-specified string
	m_Name = name;

	//load DevIL
	return LoadDevILFromLump(imageBuffer, imageLength);
}

bool CImage::LoadImageFromVfs(const char* filename)
{
	//unload previous image
	Unload();
	if (!filename)
		return false;

	//set name to vfs filename
	m_Name = filename;

	//try to open file
	idFile *file = fileSystem->OpenFileRead(m_Name);
	if (file == NULL)
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to load imagefile [%s]\r", m_Name.c_str());
		return false;
	}

	//read the whole file to buffer
	idList<unsigned char> fileData;
	fileData.Resize(file->Length());
	file->Read(&fileData[0], fileData.Num());
	//close file
	fileSystem->CloseFile(file);

	//load DevIL
	return LoadDevILFromLump(&fileData[0], fileData.Num());
}

bool CImage::LoadImageFromFile(const fs::path& path)
{
	//unload previous image
	Unload();

	//set name to boost filename
	m_Name = path.file_string().c_str();

	//try to open file
	FILE* file = NULL;
	if (!fs::exists(path) || !(file = fopen(path.file_string().c_str(), "rb")))
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to load imagefile [%s]\r", m_Name.c_str());
		return false;
	}

	//read the whole file to buffer
	idList<unsigned char> fileData;
	fileData.Resize(fs::file_size(path));
	fread(&fileData[0], 1, fileData.Num(), file);
	//close file
	fclose(file);

	//load DevIL
	return LoadDevILFromLump(&fileData[0], fileData.Num());
}

unsigned int CImage::GetDataLength() const
{
	if (m_ImageId != IL_IMAGE_NONE)
	{
		ilBindImage(m_ImageId);
		return static_cast<unsigned long>(ilGetInteger(IL_IMAGE_SIZE_OF_DATA));
	}

	return 0;
}

const unsigned char* CImage::GetImageData() const
{
	if (m_ImageId != IL_IMAGE_NONE)
	{
		ilBindImage(m_ImageId);
		return static_cast<const unsigned char*>(ilGetData());
	}

	return NULL;
}

bool CImage::SaveImageToFile(const fs::path& path, Format format) const
{
	return SaveDevILToFile(path.file_string().c_str(), format);
}

bool CImage::SaveImageToVfs(const char* filename, Format format) const
{
	return SaveDevILToFile(fileSystem->RelativePathToOSPath(filename), format);
}
