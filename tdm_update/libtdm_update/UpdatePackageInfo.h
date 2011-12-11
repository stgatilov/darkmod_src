/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

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
		boost::regex expr("^UpdatePackage from ([\\.0-9]+) to ([\\.0-9]+)$",
						  boost::regex::perl|boost::regex::icase);

		boost::smatch matches;
		
		if (boost::regex_match(sectionName, matches, expr))
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
			package.filesize = boost::lexical_cast<std::size_t>(iniFile.GetValue(sectionName, "filesize"));
			package.fromVersion = fromVersion;
			package.toVersion = toVersion;

			TraceLog::WriteLine(LOG_VERBOSE, 
				(boost::format("Found update package %s, checksum %x, from version %s to version %s") %
				package.filename.string() % package.crc % package.fromVersion % package.toVersion).str());

			insert(value_type(package.fromVersion, package));
		}
	}
};

} // namespace
