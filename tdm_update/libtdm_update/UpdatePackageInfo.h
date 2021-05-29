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

#pragma once

#include "IniFile.h"
#include "UpdatePackage.h"
#include "TraceLog.h"

namespace tdm
{

/**
 * This object maintains information about available update packages,
 * as parsed from the server's tdm_version_info.txt INI file.
 */
class UpdatePackageInfo :
	public std::multimap<std::string, UpdatePackage>,
	public IniFile::SectionVisitor
{
public:
	void LoadFromIniFile(const IniFile& iniFile)
	{
		iniFile.ForeachSection(*this);
	}

	void VisitSection(const IniFile& iniFile, const std::string& sectionName)
	{
		// Parse the section headers, e.g. [UpdatePackage from 1.02 to 1.03]
		std::regex expr("^UpdatePackage from ([\\.0-9]+) to ([\\.0-9]+)$",
						  std::regex::ECMAScript|std::regex::icase);

		std::smatch matches;
		
		if (std::regex_match(sectionName, matches, expr))
		{
			std::string fromVersion = matches[1].str();
			std::string toVersion = matches[2].str();

			if (fromVersion.empty() || toVersion.empty())
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Discarding section " + sectionName);
				return;
			}

			UpdatePackage package;

			package.filename = iniFile.GetValue(sectionName, "package");
			package.crc = CRC::ParseFromString(iniFile.GetValue(sectionName, "crc"));
            package.filesize = static_cast<std::size_t>(std::stoul(iniFile.GetValue(sectionName, "filesize")));
			package.fromVersion = fromVersion;
			package.toVersion = toVersion;

			TraceLog::WriteLine(LOG_VERBOSE, 
				stdext::format("Found update package %s, checksum %x, from version %s to version %s", package.filename.string(), package.crc, package.fromVersion, package.toVersion));

			insert(value_type(package.fromVersion, package));
		}
	}
};

} // namespace
