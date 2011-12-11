/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once

#include <string>
#include <boost/regex.hpp>
#include "ReleaseFileset.h"
#include "IniFile.h"

namespace tdm
{

class ReleaseVersions :
	public std::map<std::string, ReleaseFileSet>,
	public IniFile::SectionVisitor
{
public:
	void LoadFromIniFile(const IniFile& iniFile)
	{
		iniFile.ForeachSection(*this);
	}

	void VisitSection(const IniFile& iniFile, const std::string& sectionName)
	{
		// Get the info from the section header
		boost::regex expr("^Version([\\.0-9]+) File (.*)$",
						  boost::regex::perl|boost::regex::icase);

		boost::smatch matches;
		
		if (boost::regex_match(sectionName, matches, expr))
		{
			std::string version = matches[1].str();
			std::string filename = matches[2].str();

			ReleaseFileSet& set = FindOrInsertVersion(version);

			ReleaseFile file(filename);

			file.crc = CRC::ParseFromString(iniFile.GetValue(sectionName, "crc"));
			file.filesize = boost::lexical_cast<std::size_t>(iniFile.GetValue(sectionName, "filesize"));
			file.localChangesAllowed = iniFile.GetValue(sectionName, "allow_local_modifications") == "1";

			TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Found version %s file: %s with checksum %x") %
				version % filename % file.crc).str());

			set.insert(ReleaseFileSet::value_type(filename, file));
		}
	}

private:
	ReleaseFileSet& FindOrInsertVersion(const std::string& version)
	{
		iterator found = find(version);

		if (found != end())
		{
			return found->second;
		}

		return insert(value_type(version, ReleaseFileSet())).first->second;
	}
};

} // namespace
