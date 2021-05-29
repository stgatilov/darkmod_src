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
#include "TraceLog.h"

namespace tdm
{

class UpdatePackage :
	public IniFile::SectionVisitor
{
public:
	// The archive file this package is contained in
	fs::path filename;

	// Checksum of the update archive itself
	uint32_t crc;

	// The filesize
	std::size_t filesize;

	// The version this package applies to
	std::string fromVersion;

	// The version this package will upgrade to
	std::string toVersion;

	// PK4s which are in head, but not in base => need to be added
	std::set<ReleaseFile> pk4sToBeAdded;

	// PK4s which are in base, but not in head => should be removed
	std::set<ReleaseFile> pk4sToBeRemoved;

	struct PK4Difference
	{
		// The checksum of the base PK4
		uint32_t checksumBefore;

		// The checksum of the head PK4
		uint32_t checksumAfter;

		// Files that should be added to this PK4
		std::set<ReleaseFile> membersToBeAdded;

		// Files that should be removed from this PK4
		std::set<ReleaseFile> membersToBeRemoved;

		// Files that should be replaced
		std::set<ReleaseFile> membersToBeReplaced;
	};

	typedef std::map<std::string, PK4Difference> Pk4DifferenceMap;
	Pk4DifferenceMap pk4Differences;

	struct NonArchiveFiles
	{
		// Files that should be added to darkmod/
		std::set<ReleaseFile> toBeAdded;

		// Files that should be removed from darkmod/
		std::set<ReleaseFile> toBeRemoved;

		// Files that should be replaced in darkmod/
		std::set<ReleaseFile> toBeReplaced;
	};

	NonArchiveFiles nonArchiveFiles;

	void LoadFromIniFile(const IniFile& iniFile)
	{
		iniFile.ForeachSection(*this);

		TraceLog::WriteLine(LOG_VERBOSE, "Update Package Parse results:");
		TraceLog::WriteLine(LOG_VERBOSE, 
			stdext::format("PK4s to be removed: %d, PK4s to be added: %d, PK4s to be changed: %d", pk4sToBeRemoved.size(), pk4sToBeAdded.size(), pk4Differences.size()));
		TraceLog::WriteLine(LOG_VERBOSE, 
			stdext::format("Non-Archive files to be removed: %d, to be added: %d, to be replaced: %d", nonArchiveFiles.toBeRemoved.size(), nonArchiveFiles.toBeAdded.size(), nonArchiveFiles.toBeReplaced.size()));
	}

	void VisitSection(const IniFile& iniFile, const std::string& sectionName)
	{
		if (sectionName == "Add PK4s")
		{
			// Add these PK4s
			IniFile::KeyValuePairList keyValues = iniFile.GetAllKeyValues(sectionName);

			for (IniFile::KeyValuePairList::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i)
			{
				// Add a ReleaseFile struct with filename and CRC
				pk4sToBeAdded.insert(ReleaseFile(i->first, CRC::ParseFromString(i->second)));
			}
		}
		else if (sectionName == "Remove PK4s")
		{
			// Remove these PK4s
			IniFile::KeyValuePairList keyValues = iniFile.GetAllKeyValues(sectionName);

			for (IniFile::KeyValuePairList::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i)
			{
				if (i->second != "remove")
				{
					continue; // No "remove" key, could be something else
				}

				// Add a ReleaseFile struct with filename and CRC
				pk4sToBeRemoved.insert(ReleaseFile(i->first));
			}
		}
		else if (sectionName == "Non-Archive Files")
		{
			// Check each keyvalue
			IniFile::KeyValuePairList keyValues = iniFile.GetAllKeyValues(sectionName);

			for (IniFile::KeyValuePairList::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i)
			{
				const std::string& key = i->first;
				const std::string& value = i->second;

				if (value == "remove")
				{
					nonArchiveFiles.toBeRemoved.insert(ReleaseFile(key));
				}
				else if (value == "add")
				{
					nonArchiveFiles.toBeAdded.insert(ReleaseFile(key));
				}
				else if (value == "replace")
				{
					nonArchiveFiles.toBeReplaced.insert(ReleaseFile(key));
				}
			}
		}
		else if (stdext::starts_with(sectionName, "Change "))
		{
			std::string pk4File = sectionName.substr(7);

			PK4Difference diff;

			// Check each keyvalue
			IniFile::KeyValuePairList keyValues = iniFile.GetAllKeyValues(sectionName);

			for (IniFile::KeyValuePairList::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i)
			{
				const std::string& key = i->first;
				const std::string& value = i->second;

				if (key == "checksum_after")
				{
					diff.checksumAfter = CRC::ParseFromString(value);
				}
				else if (key == "checksum_before")
				{
					diff.checksumBefore = CRC::ParseFromString(value);
				}
				else if (value == "remove")
				{
					diff.membersToBeRemoved.insert(ReleaseFile(key));
				}
				else if (value == "add")
				{
					diff.membersToBeAdded.insert(ReleaseFile(key));
				}
				else if (value == "replace")
				{
					diff.membersToBeReplaced.insert(ReleaseFile(key));
				}
			}

			pk4Differences[pk4File] = diff;
		}
	}
};

} // namespace
