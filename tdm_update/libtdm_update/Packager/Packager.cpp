#include "Packager.h"

#include "../Constants.h"

namespace tdm
{

namespace packager
{

// Pass the program options to this class
Packager::Packager(const PackagerOptions& options) :
	_options(options)
{}

void Packager::GatherBaseSet()
{
	std::set<std::string> ignoreList;
	ignoreList.insert(TDM_CRC_INFO_FILE);

	_baseSet = ReleaseFileSet::LoadFromFolder(_options.Get("basedir"), ignoreList);
}

void Packager::GatherHeadSet()
{
	std::set<std::string> ignoreList;
	ignoreList.insert(TDM_CRC_INFO_FILE);

	_headSet = ReleaseFileSet::LoadFromFolder(_options.Get("headdir"), ignoreList);
}

void Packager::CalculateSetDifference()
{
	typedef std::vector<std::pair<std::string, ReleaseFile> > DiffContainer;

	// PK4s which have been removed since base
	{
		DiffContainer toBeRemoved;
	
		// Compute the difference for files to be removed (found in base, but not in head)
		std::set_difference(_baseSet.begin(), _baseSet.end(), _headSet.begin(), _headSet.end(), 
			std::back_inserter(toBeRemoved), _baseSet.value_comp());
		
		TraceLog::WriteLine(LOG_STANDARD, (boost::format("PK4s to be removed: %d") % toBeRemoved.size()).str());

		for (DiffContainer::iterator i = toBeRemoved.begin(); i != toBeRemoved.end(); ++i)
		{
			if (!i->second.isArchive)
			{
				TraceLog::WriteLine(LOG_VERBOSE, i->first + " is not an archive, skipping that file.");
				continue;
			}

			TraceLog::WriteLine(LOG_VERBOSE, "PK4 to be removed: " + i->second.file.file_string());
			_difference.pk4sToBeRemoved.insert(i->second);
		}
	}

	// PK4s which have been added since base
	{
		DiffContainer toBeAdded;
		// Compute the difference for files to be added (found in head, but not in base)
		std::set_difference(_headSet.begin(), _headSet.end(), _baseSet.begin(), _baseSet.end(), 
			std::back_inserter(toBeAdded), _headSet.value_comp());
		
		TraceLog::WriteLine(LOG_STANDARD, (boost::format("PK4s to be added: %d") % toBeAdded.size()).str());

		for (DiffContainer::iterator i = toBeAdded.begin(); i != toBeAdded.end(); ++i)
		{
			if (!i->second.isArchive)
			{
				TraceLog::WriteLine(LOG_VERBOSE, i->first + " is not an archive, skipping that file.");
				continue;
			}

			TraceLog::WriteLine(LOG_VERBOSE, "PK4 to be added: " + i->second.file.file_string());
			_difference.pk4sToBeAdded.insert(i->second);
		}
	}

	// Calculate member difference
	{
		DiffContainer memberDiff;

		for (ReleaseFileSet::const_iterator h = _headSet.begin(); h != _headSet.end(); ++h)
		{
			const ReleaseFile& headFile = h->second;

			if (!headFile.isArchive)
			{
				TraceLog::WriteLine(LOG_VERBOSE, h->first + " is not an archive, skipping that file.");
				continue;
			}

			ReleaseFileSet::const_iterator b = _baseSet.find(h->first);

			// Check if this file is in the base set in the first place
			if (b == _baseSet.end()) 
			{
				TraceLog::WriteLine(LOG_VERBOSE, h->first + " is not in base set, skipping detailed comparison.");
				continue;
			}

			const ReleaseFile& baseFile = b->second;

			// Compare the release files (note the non-trivial ==/!= operators)
			if (headFile == baseFile)
			{
				TraceLog::WriteLine(LOG_STANDARD, (boost::format("%s (HEAD) is equal to %s (BASE)") % h->first % b->first).str());
				continue;
			}

			TraceLog::WriteLine(LOG_STANDARD, (boost::format("%s (HEAD) is different from %s (BASE)") % h->first % b->first).str());

			// In-depth PK4 comparison

			_difference.pk4Differences[h->first] = UpdatePackage::PK4Difference();

			UpdatePackage::PK4Difference& pk4Diff = _difference.pk4Differences[h->first];

			pk4Diff.checksumBefore = baseFile.crc;
			pk4Diff.checksumAfter = headFile.crc;
			
			// Find all files that are in base, but not in head, these should be removed
			std::vector<ReleaseFile> filesToBeRemoved;

			// Compute the difference for files to be removed (found in base file, but not in head file)
			std::set_difference(baseFile.members.begin(), baseFile.members.end(), headFile.members.begin(), headFile.members.end(), 
				std::back_inserter(filesToBeRemoved));

			for (std::vector<ReleaseFile>::const_iterator i = filesToBeRemoved.begin(); i != filesToBeRemoved.end(); ++i)
			{
				TraceLog::WriteLine(LOG_VERBOSE, "  PK4 member to be removed: " + i->file.string());

				pk4Diff.membersToBeRemoved.insert(*i);
			}

			std::size_t equalMembers = 0;
			
			for (std::set<ReleaseFile>::const_iterator hf = headFile.members.begin(); hf != headFile.members.end(); ++hf)
			{
				std::set<ReleaseFile>::const_iterator bf = baseFile.members.find(*hf);

				if (bf == baseFile.members.end()) 
				{
					// New file
					TraceLog::WriteLine(LOG_VERBOSE, "  PK4 member to be added: " + hf->file.string());

					pk4Diff.membersToBeAdded.insert(*hf);
				}
				else
				{
					// File in base as well as in head, compare crc and size
					if (*bf != *hf)
					{
						TraceLog::WriteLine(LOG_VERBOSE, "  PK4 member has been changed: " + hf->file.string());

						pk4Diff.membersToBeReplaced.insert(*hf);
					}
					else
					{
						TraceLog::WriteLine(LOG_VERBOSE, "  PK4 member is equal to base: " + hf->file.string());
						equalMembers++;
					}
				}
			}

			TraceLog::WriteLine(LOG_STANDARD, (boost::format("  %d files changed, %d files to be added, %d files to be removed, %d files equal.") % 
				pk4Diff.membersToBeReplaced.size() % pk4Diff.membersToBeAdded.size() % pk4Diff.membersToBeRemoved.size() % equalMembers).str());
		}
	}
}

void Packager::CreateUpdatePackage()
{
	std::string updatePackageFileName = (boost::format("tdm_update_%s_to_%s.zip") % _options.Get("baseversion") % _options.Get("headversion")).str();

	fs::path headDir = _options.Get("headdir");
	fs::path outputDir = _options.Get("outputdir");

	fs::path updatePackagePath = outputDir / updatePackageFileName; 

	_difference.filename = updatePackagePath;

	TraceLog::WriteLine(LOG_STANDARD, "Creating update package at " + updatePackagePath.file_string());

	ZipFileWritePtr updatePackage = Zip::OpenFileWrite(updatePackagePath, Zip::CREATE);

	if (updatePackage == NULL)
	{
		throw FailureException("Couldn't create " + updatePackagePath.file_string());
	}

	IniFilePtr updateDesc = IniFile::Create();

	updateDesc->SetValue("Info", "from_version", _options.Get("baseversion"));
	updateDesc->SetValue("Info", "to_version", _options.Get("headversion"));

	// Pack in new files
	for (std::set<ReleaseFile>::const_iterator i = _difference.pk4sToBeAdded.begin(); i != _difference.pk4sToBeAdded.end(); ++i)
	{
		TraceLog::WriteLine(LOG_STANDARD, "Storing PK4: " + i->file.string());

		updatePackage->DeflateFile(headDir / i->file, i->file.string(), ZipFileWrite::STORE);

		updateDesc->SetValue("Add PK4s", i->file.string(), CRC::ToString(i->crc));
	}

	// Add information about removed PK4s
	for (std::set<ReleaseFile>::const_iterator i = _difference.pk4sToBeRemoved.begin(); i != _difference.pk4sToBeRemoved.end(); ++i)
	{
		TraceLog::WriteLine(LOG_STANDARD, "PK4 marked for removal: " + i->file.string());

		updateDesc->SetValue("Remove PK4s", i->file.string(), "remove");
	}

	// Add sections for removed PK4 members
	for (UpdatePackage::Pk4DifferenceMap::const_iterator i = _difference.pk4Differences.begin(); 
		 i != _difference.pk4Differences.end(); ++i)
	{
		TraceLog::WriteLine(LOG_STANDARD, "Changed PK4: " + i->first);

		std::string section = "Change " + i->first;

		// Override for changed ZIP files: these will be extracted, so changes directly affect the filesystem
		if (File::IsZip(i->first))
		{
			section = "Non-Archive Files";
		}

		if (File::IsPK4(i->first))
		{
			// Add checksum information for PK4s
			updateDesc->SetValue(section, "checksum_before", CRC::ToString(i->second.checksumBefore));
			updateDesc->SetValue(section, "checksum_after", CRC::ToString(i->second.checksumAfter));
		}

		for (std::set<ReleaseFile>::const_iterator m = i->second.membersToBeRemoved.begin(); 
			 m != i->second.membersToBeRemoved.end(); ++m)
		{
			TraceLog::WriteLine(LOG_STANDARD, "  Member marked for removal: " + m->file.string());

			updateDesc->SetValue(section, m->file.string(), "remove");
		}

		// Quick continue if no added or changed members
		if (i->second.membersToBeAdded.empty() && i->second.membersToBeReplaced.empty())
		{
			continue;
		}

		// Open the source PK4
		ZipFileReadPtr sourcePk4 = Zip::OpenFileRead(headDir / i->first);

		for (std::set<ReleaseFile>::const_iterator m = i->second.membersToBeAdded.begin(); 
			 m != i->second.membersToBeAdded.end(); ++m)
		{
			TraceLog::WriteLine(LOG_STANDARD, "  Member added: " + m->file.string());

			updateDesc->SetValue(section, m->file.string(), "add");

			// Pack that added file into the update PK4
			updatePackage->CopyFileFromZip(sourcePk4, m->file.string(), m->file.string());
		}

		for (std::set<ReleaseFile>::const_iterator m = i->second.membersToBeReplaced.begin(); 
			 m != i->second.membersToBeReplaced.end(); ++m)
		{
			TraceLog::WriteLine(LOG_STANDARD, "  Member changed: " + m->file.string());

			updateDesc->SetValue(section, m->file.string(), "replace");

			// Pack that added file into the update PK4
			updatePackage->CopyFileFromZip(sourcePk4, m->file.string(), m->file.string());
		}
	}

	// Pack the update INI file into the package
	fs::path iniPath = outputDir / TDM_UDPATE_INFO_FILE;

	updateDesc->ExportToFile(iniPath);

	updatePackage->DeflateFile(iniPath, TDM_UDPATE_INFO_FILE);

	// Remove the ini file afterwards
	File::Remove(iniPath);
}

void Packager::CreateVersionInformation()
{
	if (_baseSet.empty())
	{
		throw FailureException("No base information, cannot create version information.");
	}

	assert(_options.IsSet("baseversion"));

	fs::path outputDir = _options.Get("outputdir");

	fs::path versionInfoFile = outputDir / TDM_VERSION_INFO_FILE;

	IniFilePtr versionInfo;

	if (fs::exists(versionInfoFile))
	{
		// Load existing version information
		TraceLog::WriteLine(LOG_STANDARD, "Loading existing version information file: " + versionInfoFile.file_string());
		versionInfo = IniFile::ConstructFromFile(versionInfoFile);
	}
	else
	{
		// Create a new ini file
		versionInfo = IniFile::Create();
	}

	// The list of files for which no CRC should be calculated during version check
	std::set<std::string> noCrcFiles;
	noCrcFiles.insert("doomconfig.cfg");
	noCrcFiles.insert("dmargs.txt");

	// Merge the information into the file
	for (ReleaseFileSet::const_iterator f = _baseSet.begin(); f != _baseSet.end(); ++f)
	{
		std::string section = (boost::format("Version%s File %s") % _options.Get("baseversion") % f->second.file.string()).str();

		if (File::IsPK4(f->second.file))
		{
			versionInfo->SetValue(section, "crc", CRC::ToString(f->second.crc));
			versionInfo->SetValue(section, "filesize", boost::lexical_cast<std::string>(f->second.filesize));
		}
		// Traverse ZIP file members (the actual ZIP files will be ignored when checking the local version)
		else if (File::IsZip(f->second.file))
		{
			for (std::set<ReleaseFile>::const_iterator m = f->second.members.begin(); m != f->second.members.end(); ++m)
			{
				std::string memberSection = (boost::format("Version%s File %s") % _options.Get("baseversion") % m->file.string()).str();

				versionInfo->SetValue(memberSection, "crc", CRC::ToString(m->crc));
				versionInfo->SetValue(memberSection, "filesize", boost::lexical_cast<std::string>(m->filesize));

				if (noCrcFiles.find(boost::algorithm::to_lower_copy(m->file.file_string())) != noCrcFiles.end())
				{
					versionInfo->SetValue(memberSection, "allow_local_modifications", "1");
				}
			}
		}
	}

	// Save the file
	TraceLog::WriteLine(LOG_STANDARD, "Saving version information file: " + versionInfoFile.file_string());
	versionInfo->ExportToFile(versionInfoFile);
}

void Packager::RegisterUpdatePackage(const fs::path& packagePath)
{
	fs::path path = packagePath;

	if (!fs::exists(path)) 
	{
		path = _options.Get("outputdir");
		path /= packagePath;
	}

	if (!fs::exists(path)) 
	{
		throw FailureException("No package found at this path.");
	}

	ZipFileReadPtr package = Zip::OpenFileRead(path);

	if (package == NULL)
	{
		throw FailureException("Cannot open this package.");
	}

	TraceLog::WriteLine(LOG_STANDARD, "Loading update info file from package: " + path.file_string());

	std::string updateInfoStr = package->LoadTextFile(TDM_UDPATE_INFO_FILE);

	IniFilePtr iniFile = IniFile::ConstructFromString(updateInfoStr);

	if (iniFile->IsEmpty())
	{
		throw FailureException("Cannot load update info file from that package.");
	}

	std::string fromVersion = iniFile->GetValue("Info", "from_version");
	std::string toVersion = iniFile->GetValue("Info", "to_version");

	if (fromVersion.empty() || toVersion.empty())
	{
		throw FailureException("Cannot find from_version and to_version information in update package.");
	}

	// Open the target file the information should be merged in
	fs::path targetPath = _options.Get("outputdir");
	targetPath /= TDM_VERSION_INFO_FILE;

	IniFilePtr targetFile = IniFile::ConstructFromFile(targetPath);

	if (targetFile == NULL)
	{
		TraceLog::WriteLine(LOG_STANDARD, "Cannot find target version info file, creating afresh: " + targetPath.file_string());

		targetFile = IniFile::Create();
	}

	TraceLog::WriteLine(LOG_STANDARD, "Registering update package from version " + fromVersion + " to version " + toVersion);

	std::string section = (boost::format("UpdatePackage from %s to %s") % fromVersion % toVersion).str();

	// Store the information
	targetFile->SetValue(section, "package", path.leaf());
	targetFile->SetValue(section, "filesize", boost::lexical_cast<std::string>(fs::file_size(path)));
	targetFile->SetValue(section, "crc", CRC::ToString(CRC::GetCrcForFile(path)));

	TraceLog::WriteLine(LOG_STANDARD, "Saving INI file: " + targetPath.file_string());

	targetFile->ExportToFile(targetPath);
}

} 

}
