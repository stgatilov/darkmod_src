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

#define IL_IMAGE_NONE ((ILuint)-1)

CImage::CImage() :
	m_ImageBufferLength(0L),
	m_ImageBuffer(NULL),
	m_ImageId(IL_IMAGE_NONE),
	m_Loaded(false),
	m_defaultImageType(IL_TGA),
	m_Width(0),
	m_Height(0),
	m_Bpp(0)
{}

CImage::CImage(const idStr& name) :
	m_ImageBufferLength(0L),
	m_ImageBuffer(NULL),
	m_ImageId(IL_IMAGE_NONE),
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
		if(m_ImageBuffer != NULL)
			delete [] m_ImageBuffer;

		m_ImageBuffer = NULL;
	}

	if(m_ImageId != IL_IMAGE_NONE)
		ilDeleteImages(1, &m_ImageId);

	m_ImageId = IL_IMAGE_NONE;
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

			if(BufLen > m_ImageBufferLength || m_ImageBuffer == NULL)
			{
				Unload(true);
				m_ImageBufferLength = BufLen;
				if((m_ImageBuffer = new unsigned char[m_ImageBufferLength]) == NULL)
				{
					DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Out of memory while allocating %lu bytes for [%s]\r", m_ImageBufferLength, m_Name.c_str());
					goto Quit;
				}
			}
			DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("Total of %lu bytes read from renderpipe [%s]   %lu (%08lX)\r", BufLen, m_Name.c_str(), m_ImageBufferLength, m_ImageBuffer);

			memcpy(m_ImageBuffer, pipe_buf, m_ImageBufferLength);
			InitImageInfo();
		}
	}

Quit:
	if(m_Loaded == false && m_ImageBuffer != NULL)
	{
		delete [] m_ImageBuffer;
		m_ImageBuffer = NULL;
	}

	return rc;
}

bool CImage::LoadImageFromVfs(const char* filename)
{
	bool rc = false;
	idFile *fl = NULL;

	if (filename != NULL)
	{
		Unload(true);
		m_Name = filename;
	}

	if (m_Loaded == false)
	{
		if((fl = fileSystem->OpenFileRead(m_Name)) == NULL)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to load imagefile [%s]\r", m_Name.c_str());
			goto Quit;
		}

		m_ImageBufferLength = fl->Length();
		if((m_ImageBuffer = new unsigned char[m_ImageBufferLength]) == NULL)
		{
			DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Out of memory while allocating %lu bytes for [%s]\r", m_ImageBufferLength, m_Name.c_str());
			goto Quit;
		}
		fl->Read(m_ImageBuffer, m_ImageBufferLength);
		fileSystem->CloseFile(fl);

		InitImageInfo();

		rc = true;
//		DM_LOG(LC_SYSTEM, LT_INFO)LOGSTRING("ImageWidth: %u   ImageHeight: %u   ImageDepth: %u   BPP: %u   Buffer: %u\r", m_Width, m_Height, ilGetInteger(IL_IMAGE_DEPTH), m_Bpp, m_ImageBufferLength);
	}

Quit:
	if(m_Loaded == false && m_ImageBuffer != NULL)
	{
		delete [] m_ImageBuffer;
		m_ImageBuffer = NULL;
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

		if (length != m_ImageBufferLength)
		{
			// Free the old buffer first
			Unload(true);

			m_ImageBufferLength = length;

			// Re-allocate to match buffer size
			m_ImageBuffer = new unsigned char[m_ImageBufferLength];

			if (m_ImageBuffer == NULL)
			{
				DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Out of memory while allocating %lu bytes for [%s]\r", m_ImageBufferLength, m_Name.c_str());
				return false;
			}
		}

		// Read into buffer
		FILE* fh = fopen(path.file_string().c_str(), "rb");

		fread(m_ImageBuffer, 1, m_ImageBufferLength, fh);

		fclose(fh);

		InitImageInfo();
	}

	return true;
}

void CImage::InitImageInfo()
{
	ilGenImages(1, &m_ImageId);
	ilBindImage(m_ImageId);

	if (ilLoadL(m_defaultImageType, m_ImageBuffer, m_ImageBufferLength) == IL_FALSE)
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

unsigned long CImage::GetDataLength()
{
	if (m_Loaded && m_ImageBuffer != NULL)
	{
		ilBindImage(m_ImageId);
		return static_cast<unsigned long>(ilGetInteger(IL_IMAGE_SIZE_OF_DATA));
	}

	return 0;
}

unsigned char* CImage::GetImageData()
{
	if (m_Loaded && m_ImageBuffer != NULL)
	{
		ilBindImage(m_ImageId);
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
	else if (m_Loaded && m_ImageId == (ILuint)-1 && m_ImageBuffer != NULL)
	{
		// Ensure buffer is bound to devIL
		ilBindImage(m_ImageId);
		ilLoadL(m_defaultImageType, m_ImageBuffer, m_ImageBufferLength);
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
