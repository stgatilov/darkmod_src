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

bool CZipFile::ContainsFile(const idStr& fileName)
{
	int result = unzLocateFile(_handle, fileName.c_str(), 0);

	return (result == UNZ_OK);
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
