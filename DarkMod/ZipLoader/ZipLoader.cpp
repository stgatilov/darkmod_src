/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3079 $
 * $Date: 2008-12-06 09:28:50 +0100 (Sa, 06 Dez 2008) $
 * $Author: angua $
 *
 ***************************************************************************/

#pragma hdrstop
#include "ZipLoader.h"

#include "minizip/unzip.h"

CZipFile::CZipFile(unzFile handle) :
	_handle(handle)
{}

CZipFile::~CZipFile()
{
	unzClose(_handle);
}

bool CZipFile::ContainsFile(const idStr& fileName)
{
	int result = unzLocateFile(_handle, fileName.c_str(), 0);

	return (result == UNZ_OK);
}

idStr CZipFile::LoadTextFile(const idStr& fileName)
{
	int result = unzLocateFile(_handle, fileName.c_str(), 0);

	if (result != UNZ_OK) return "";

	unz_file_info info;
	unzGetCurrentFileInfo(_handle, &info, NULL, 0, NULL, 0, NULL, 0);

	unsigned long fileSize = info.uncompressed_size;

	int openResult = unzOpenCurrentFile(_handle);

	idStr returnValue;

	if (openResult == UNZ_OK)
	{
		char* buffer = new char[fileSize + 1];

		// Read and null-terminate the string
		unzReadCurrentFile(_handle, buffer, fileSize);
		buffer[fileSize] = '\0';

		returnValue = buffer;
		
		delete buffer;
	}

	unzCloseCurrentFile(_handle);

	return returnValue;
}

// --------------------------------------------------------

CZipLoader::CZipLoader()
{}

CZipFilePtr CZipLoader::LoadFile(const idStr& fullOSPath)
{
	unzFile handle = unzOpen(fullOSPath.c_str());

	return (handle != NULL) ? CZipFilePtr(new CZipFile(handle)) : CZipFilePtr();
}

CZipLoader& CZipLoader::Instance()
{
	static CZipLoader _instance;
	return _instance;
}
