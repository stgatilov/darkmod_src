/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod Updater (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#ifndef _TDM_CRC_H_
#define _TDM_CRC_H_

#include <string>
#include <stdexcept>
#include <sstream>
#include <cstdio>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include <boost/crc.hpp>
#include <boost/filesystem.hpp>

#include "File.h"
#include "Zip/Zip.h"
#include "TraceLog.h"

namespace fs = boost::filesystem;

namespace tdm
{

/**
 * Service class for CRC handling.
 */
class CRC
{
public:
	static boost::uint32_t ParseFromString(const std::string& hexaStr)
	{
		boost::uint32_t out;

		std::stringstream ss;

		if (!boost::algorithm::starts_with(hexaStr, "0x"))
		{
			ss << ("0x" + hexaStr);
		}
		else
		{
			ss << hexaStr;
		}

		ss >> std::hex >> out;
		
		return out;
	}

	static std::string ToString(boost::uint32_t crc)
	{
		return (boost::format("%x") % crc).str();
	}

	/**
	 * greebo: Returns the CRC for the given file. ZIP-archives (PK4s too) will 
	 * be opened and a cumulative CRC over the archive members will be returned.
	 *
	 * @throws: std::runtime_error if something goes wrong.
	 */
	static boost::uint32_t GetCrcForFile(const fs::path& file)
	{
		try
		{
			if (File::IsArchive(file))
			{
				return GetCrcForZip(file);
			}
			else
			{
#ifdef TDM_USE_OLD_CRC
				return GetCrcForNonZipFileOld(file);
#else
				return GetCrcForNonZipFile(file);
#endif
			}
		}
		catch (std::runtime_error& ex)
		{
			TraceLog::Write(LOG_ERROR, ex.what());
			throw ex;
		}
	}
	
	// This is the algorithm as used in the TDM 1.02 tdm_update.pl version 0.46
	// It fails to produce the same CRC as the one found in the ZIP archives
	// See http://modetwo.net/darkmod/index.php?/topic/11473-problem-with-crcs-and-the-updater/

	static boost::uint32_t GetCrcForNonZipFileOld(const fs::path& file)
	{
		// Open the file for reading
		FILE* fh = fopen(file.string().c_str(), "rb");

		if (fh == NULL) throw std::runtime_error("Could not open file: " + file.string());

		boost::uint32_t crc = 0;
		
		while (true)
		{
			// Read the file in 32kb chunks
			char buf[32*1024];

			size_t bytesRead = fread(buf, 1, sizeof(buf), fh);

			if (bytesRead > 0)
			{
				boost::crc_32_type processor;
				processor.process_bytes(buf, bytesRead);

				crc ^= processor.checksum();

				continue;
			}
			
			break;
		}

		TraceLog::WriteLine(LOG_VERBOSE, "CRC calculated for file " + file.string() + " = " + (boost::format("%x") % crc).str());

		fclose(fh);

		return crc;
	}
	
	static boost::uint32_t GetCrcForNonZipFile(const fs::path& file)
	{
		// Open the file for reading
		FILE* fh = fopen(file.string().c_str(), "rb");

		if (fh == NULL) throw std::runtime_error("Could not open file: " + file.string());

		boost::uint32_t crc = 0;
		boost::crc_32_type processor;
		
		while (true)
		{
			// Read the file in 32kb chunks
			char buf[32*1024];

			size_t bytesRead = fread(buf, 1, sizeof(buf), fh);

			if (bytesRead > 0)
			{
				processor.process_bytes(buf, bytesRead);
				continue;
			}
			
			break;
		}

		crc = processor.checksum();

		TraceLog::WriteLine(LOG_VERBOSE, "CRC calculated for file " + file.string() + " = " + (boost::format("%x") % crc).str());

		fclose(fh);

		return crc;
	}

	static boost::uint32_t GetCrcForZip(const fs::path& file)
	{
		// Open the file for reading
		ZipFileReadPtr zipFile = Zip::OpenFileRead(file);

		if (zipFile == NULL) throw std::runtime_error("Could not open ZIP file: " + file.string());

		boost::uint32_t crc = zipFile->GetCumulativeCrc();

		TraceLog::WriteLine(LOG_VERBOSE, "CRC calculated for zip file " + file.string() + " = " + (boost::format("%x") % crc).str());

		return crc;
	}
};

} // namespace

#endif /* _TDM_CRC_H_ */
