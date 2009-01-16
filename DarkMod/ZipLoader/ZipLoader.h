/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 3079 $
 * $Date: 2008-12-06 09:28:50 +0100 (Sa, 06 Dez 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef ZIPLOADER_H_
#define ZIPLOADER_H_

#pragma hdrstop

#include <boost/shared_ptr.hpp>
#include "minizip/unzip.h"

class CZipFile
{
	// The handle for the zip archive
	unzFile _handle;
public:
	CZipFile(unzFile handle);

	CZipFile::~CZipFile();

	/**
	 * greebo: returns TRUE when this archive contains the given file.
	 */
	bool ContainsFile(const idStr& fileName);

	/**
	 * Attempts to load the given text file. The string will be empty
	 * if the file failed to load.
	 */
	idStr LoadTextFile(const idStr& fileName);
};
typedef boost::shared_ptr<CZipFile> CZipFilePtr;

/**
 * greebo: This service class can be used to load and inspect zip files and
 * retrieve specific files from the archive. D3 filesystem doesn't expose
 * ZIP loading functionality, so this is the do-it-yourself approach.
 */
class CZipLoader
{
	// Private constructor
	CZipLoader();
public:

	/**
	 * Tries to load the given file (path is absolute, use D3 VFS functions
	 * to resolve a relative D3 path to a full OS path).
	 * 
	 * @returns: NULL on failure, the file object on success.
	 */
	CZipFilePtr LoadFile(const idStr& fullOSPath);

	// Singleton instance
	static CZipLoader& Instance();
};

#endif /* ZIPLOADER_H_ */
