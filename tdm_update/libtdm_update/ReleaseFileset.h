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

#include "StdFilesystem.h"

#include "IniFile.h"
#include "CRC.h"
#include "File.h"

namespace tdm
{

struct ReleaseFile
{
	// True if this file is a PK4 / ZIP package
	bool isArchive;

	// Filename including path
	fs::path file;

	// CRC32 checksum
	uint32_t	crc;

	// The file size in bytes
	std::size_t	filesize;

	// Members of this archive
	std::set<ReleaseFile> members;

	// This is TRUE for DoomConfig.cfg, for example. 
	// Is only used when comparing release versions in the updater code
	bool localChangesAllowed;

	// The download ID of this file (-1 == no download ID)
	int downloadId;

	ReleaseFile() :
		isArchive(false),
		localChangesAllowed(false),
		downloadId(-1)
	{}

	ReleaseFile(const fs::path& pathToFile) :
		isArchive(false),
		file(pathToFile),
		localChangesAllowed(false),
		downloadId(-1)
	{}

	ReleaseFile(const fs::path& pathToFile, uint32_t crc_) :
		isArchive(false),
		file(pathToFile),
		crc(crc_),
		localChangesAllowed(false),
		downloadId(-1)
	{}

	// Implement less operator for use in std::set or std::map
	bool operator<(const ReleaseFile& other) const
	{
		return file < other.file;
	}

	// A ReleaseFile is equal if filename, size, archive flag, CRC and all members are equal
	bool operator==(const ReleaseFile& other) const
	{
		if (file != other.file || crc != other.crc || filesize != other.filesize || isArchive != other.isArchive)
		{
			return false;
		}

		if (members.size() != other.members.size()) 
		{
			return false; // member size mismatch
		}

		// Equal member size, compare all 
		for (std::set<ReleaseFile>::const_iterator i = members.begin(), j = other.members.begin();
			 i != members.end(); ++i, ++j)
		{
			if (*i != *j) return false;
		}

		return true; // all checks passed
	}

	bool operator!=(const ReleaseFile& other) const
	{
		return !(this->operator==(other));
	}

	bool ContainsUpdater(const std::string& executable) const
	{
		for (std::set<ReleaseFile>::const_iterator m = members.begin(); m != members.end(); ++m)
		{
			if (stdext::to_lower_copy(m->file.string()) == stdext::to_lower_copy(executable))
			{
				return true;
			}
		}

		return false;
	}
};

/** 
 * A FileSet represents a full release package, including
 * PK4 files and extracted files like INI, tdmlauncher.exe, etc.
 * Each file contains a checksum so that it can be compared to 
 * an available package on the update servers.
 */
class ReleaseFileSet :
	public std::map<std::string, ReleaseFile>
{
public:
	ReleaseFileSet()
	{}

	// Construct a set by specifying the INI file it is described in
	// The INI file is usually the crc_info.txt file on the servers
	static ReleaseFileSet LoadFromIniFile(const IniFile& iniFile)
	{
		ReleaseFileSet set;

		class Visitor :
			public IniFile::SectionVisitor
		{
		private:
			ReleaseFileSet& _set;

		public:
			Visitor(ReleaseFileSet& set) :
				_set(set)
			{}
				
			void VisitSection(const IniFile& iniFile, const std::string& section)
			{
				if (stdext::istarts_with(section, "File"))
				{
					std::string filename = section.substr(5);

					std::pair<ReleaseFileSet::iterator, bool> result = _set.insert(
						ReleaseFileSet::value_type(filename, ReleaseFile(filename)));
					
					result.first->second.crc = CRC::ParseFromString(iniFile.GetValue(section, "crc"));
					result.first->second.filesize = static_cast<std::size_t>(std::stoul(iniFile.GetValue(section, "size")));

					if (stdext::iends_with(filename, "pk4") || 
						stdext::iends_with(filename, "zip"))
					{
						result.first->second.isArchive = true;
					}
					else
					{
						result.first->second.isArchive = false;
					}
				}
				else if (stdext::istarts_with(section, "Member"))
				{
					std::string combo = section.substr(7);

					// bar.zip:foo.pk4
					std::string archiveName = combo.substr(0, combo.rfind(':'));
					std::string memberName = combo.substr(combo.rfind(':') + 1);

					// Find or insert the archive
					std::pair<ReleaseFileSet::iterator, bool> result = _set.insert(
						ReleaseFileSet::value_type(archiveName, ReleaseFile(archiveName)));

					// Find or insert the member
					ReleaseFile member(memberName);

					member.isArchive = false;
					member.crc = CRC::ParseFromString(iniFile.GetValue(section, "crc"));
                    member.filesize = static_cast<std::size_t>(std::stoul(iniFile.GetValue(section, "size")));

					result.first->second.members.insert(member);
				}
			}
		} _visitor(set);

		// Traverse the settings using the ReleaseFileSet as visitor
		iniFile.ForeachSection(_visitor);

		return set;
	}

	// Construct a release file set from the given folder
	static ReleaseFileSet LoadFromFolder(const fs::path& folder, 
										 const std::set<std::string> ignoreList = std::set<std::string>())
	{
		ReleaseFileSet set;

		for (fs::path file : fs::recursive_directory_enumerate(folder))
		{
			if (fs::is_directory(file))
			{
				continue; // skip directories
			}

			std::string filename = file.filename().string();

			fs::path relativePath = File::GetRelativePath(file, folder);

			if (ignoreList.find(stdext::to_lower_copy(filename)) != ignoreList.end())
			{
				TraceLog::WriteLine(LOG_STANDARD, "Ignoring file: " + relativePath.string());
				continue;
			}
		
			TraceLog::WriteLine(LOG_STANDARD, "Found file: " + relativePath.string());

			if (File::IsArchive(file))
			{
				ReleaseFile archive(relativePath);

				archive.isArchive = true;
				archive.crc = CRC::GetCrcForFile(file);
				archive.filesize = static_cast<std::size_t>(fs::file_size(file));

				// Add all members of this archive to the ReleaseFile
				class ZipFileVisitor : 
					public ZipFileRead::Visitor
				{
				private:
					ReleaseFile& _archive;

				public:
					ZipFileVisitor(ReleaseFile& archive) :
						_archive(archive)
					{}

					void VisitFile(const ZipFileRead::MemberInfo& info)
					{
						ReleaseFile file(info.filename);

						file.crc = info.crc;
						file.filesize = info.uncompressedSize;

						TraceLog::WriteLine(LOG_VERBOSE, "  Adding archive member " + info.filename + 
							" with CRC " + stdext::format("%x", file.crc));

						_archive.members.insert(file);
					}

				} _visitor(archive);

				TraceLog::WriteLine(LOG_STANDARD, "  This is an archive, checksum is: " + stdext::format("%x", archive.crc));

				// Open this archive
				ZipFileReadPtr zipFile = Zip::OpenFileRead(file);

				if (zipFile == NULL)
				{
					TraceLog::WriteLine(LOG_STANDARD, "  Failed to open archive: " + file.string());
					continue;
				}

				zipFile->ForeachFile(_visitor);

				TraceLog::WriteLine(LOG_STANDARD, stdext::format("  Archive has %d members", archive.members.size()));
				
				set[relativePath.string()] = archive;
			}
		}

		return set;
	}
};

} // namespace
