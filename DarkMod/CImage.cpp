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
#include "renderpipe.h"

CImage::CImage() :
	m_BufferLength(0L),
	m_Image(NULL),
	m_ImageId((ILuint)-1),
	m_Loaded(false),
	m_defaultImageType(IL_TGA),
	m_Width(0),
	m_Height(0),
	m_Bpp(0)
{}

CImage::CImage(const idStr& name) :
	m_BufferLength(0L),
	m_Image(NULL),
	m_ImageId((ILuint)-1),
	m_Loaded(false),
	m_defaultImageType(IL_TGA),
	m_Name(name),
	m_Width(0),
	m_Height(0),
	m_Bpp(0)
{}

CImage::~CImage(void)
{
	Unload(true);
}

void CImage::SetDefaultImageType(CImage::Type type)
{
	m_defaultImageType = GetILTypeForImageType(type);
}

ILenum CImage::GetILTypeForImageType(Type type)
{
	switch (type)
	{
	case AUTO_DETECT:	return IL_TYPE_UNKNOWN;
	case TGA:			return IL_TGA;
	case PNG:			return IL_PNG;
	case JPG:			return IL_JPG;
	case GIF:			return IL_GIF;
	default:			return IL_TYPE_UNKNOWN;
	};
}

void CImage::Unload(bool FreeMemory)
{
	m_Loaded = false;
	if(FreeMemory == true)
	{
		if(m_Image != NULL)
			delete [] m_Image;

		m_Image = NULL;
	}

	if(m_ImageId != static_cast<unsigned char>(-1))
		ilDeleteImages(1, &m_ImageId);

	m_ImageId = (ILuint)-1;
}

bool CImage::LoadImage(CRenderPipe* pipe)
{
	bool rc = false;

	if(pipe != NULL)
		Unload(false);

	if(m_Loaded == false)
	{
		if(pipe != NULL)
		{
			static char pipe_buf[DARKMOD_LG_RENDERPIPE_BUFSIZE];
			unsigned int BufLen = DARKMOD_LG_RENDERPIPE_BUFSIZE;
			
#ifdef _DEBUG
			// For debugging
			memset(pipe_buf, 42, BufLen);
#endif
			
			pipe->Read(pipe_buf, &BufLen);

			if(BufLen > m_BufferLength || m_Image == NULL)
			{
				Unload(true);
				m_BufferLength = BufLen;
				if((m_Image = new unsigned char[m_BufferLength]) == NULL)
				{
					DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Out of memory while allocating %lu bytes for [%s]\r", m_BufferLength, m_Name.c_str());
					goto Quit;
				}
			}
			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("Total of %lu bytes read from renderpipe [%s]   %lu (%08lX)\r", BufLen, m_Name.c_str(), m_BufferLength, m_Image);

			memcpy(m_Image, pipe_buf, m_BufferLength);
			InitImageInfo();
		}
	}

Quit:
	if(m_Loaded == false && m_Image != NULL)
	{
		delete [] m_Image;
		m_Image = NULL;
	}

	return rc;
}

bool CImage::LoadImageFromVfs(const char* filename)
{
	bool rc = false;
	idFile *fl = NULL;

	if (filename != NULL)
	{
		Unload(false);
		m_Name = filename;
	}

	if (m_Loaded == false)
	{
		if((fl = fileSystem->OpenFileRead(m_Name)) == NULL)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to load imagefile [%s]\r", m_Name.c_str());
			goto Quit;
		}

		m_BufferLength = fl->Length();
		if((m_Image = new unsigned char[m_BufferLength]) == NULL)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Out of memory while allocating %lu bytes for [%s]\r", m_BufferLength, m_Name.c_str());
			goto Quit;
		}
		fl->Read(m_Image, m_BufferLength);
		fileSystem->CloseFile(fl);

		InitImageInfo();

		rc = true;
//		DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("ImageWidth: %u   ImageHeight: %u   ImageDepth: %u   BPP: %u   Buffer: %u\r", m_Width, m_Height, ilGetInteger(IL_IMAGE_DEPTH), m_Bpp, m_BufferLength);
	}

Quit:
	if(m_Loaded == false && m_Image != NULL)
	{
		delete [] m_Image;
		m_Image = NULL;
	}

	return rc;
}

bool CImage::LoadImageFromFile(const fs::path& path)
{
	if (!fs::exists(path))
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to load imagefile [%s]\r", path.file_string().c_str());
		return false;
	}

	m_Name = path.file_string().c_str();

	if (!m_Loaded)
	{
		// Load the image into memory
		unsigned long length = static_cast<unsigned long>(fs::file_size(path));

		if (length != m_BufferLength)
		{
			// Free the old buffer first
			Unload(true);

			m_BufferLength = length;

			// Re-allocate to match buffer size
			m_Image = new unsigned char[m_BufferLength];

			if (m_Image == NULL)
			{
				DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Out of memory while allocating %lu bytes for [%s]\r", m_BufferLength, m_Name.c_str());
				return false;
			}
		}

		// Read into buffer
		FILE* fh = fopen(path.file_string().c_str(), "rb");

		fread(m_Image, 1, m_BufferLength, fh);

		fclose(fh);

		InitImageInfo();
	}

	return true;
}

void CImage::InitImageInfo()
{
	ilGenImages(1, &m_ImageId);
	ilBindImage(m_ImageId);

	if (ilLoadL(m_defaultImageType, m_Image, m_BufferLength) == IL_FALSE)
	{
		ILenum error = ilGetError();
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Error %i while loading image [%s]\r", (int)error, m_Name.c_str());
		// Tels: Couldn't load image from memory buffer, free memory
		m_Loaded = false;
		return;
	}

	// else: loading success
	m_Loaded = true;

	m_Width = ilGetInteger(IL_IMAGE_WIDTH);
	m_Height = ilGetInteger(IL_IMAGE_HEIGHT);
	m_Bpp = ilGetInteger(IL_IMAGE_BPP);
}

unsigned long CImage::GetBufferLen()
{
	return m_BufferLength;
}

unsigned char* CImage::GetImage()
{
	if (m_Loaded && m_Image != NULL)
	{
		ilBindImage(m_ImageId);
		ilLoadL(m_defaultImageType, m_Image, m_BufferLength);

		return static_cast<unsigned char*>(ilGetData());
	}

	return NULL;
}

bool CImage::SaveToFile(const fs::path& path, Type type)
{
	if (!m_Loaded) 
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Cannot save image before loading data (%s).\r", path.file_string().c_str());
		return false;
	}
	else if (m_Loaded && m_ImageId == (ILuint)-1 && m_Image != NULL)
	{
		// Ensure buffer is bound to devIL
		ilBindImage(m_ImageId);
		ilLoadL(m_defaultImageType, m_Image, m_BufferLength);
	}

	// Overwrite option
	ilEnable(IL_FILE_OVERWRITE);

	DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("Saving image to path: %s, type %d.\r", path.file_string().c_str(), type);

	// devIL wants to have a non-const char* pointer, wtf?
	char filename[1024];

	if (path.file_string().size() > sizeof(filename))
	{
		return false;
	}

	strcpy(filename, path.file_string().c_str());

	if (!ilSave(GetILTypeForImageType(type), filename))
	{
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Could not save image to path %s (type %d): error message %s.\r", path.file_string().c_str(), type, ilGetString(ilGetError()));
		return false;
	}

	return true;
}
