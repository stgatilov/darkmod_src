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

#include "Updater.h"

#include "../TraceLog.h"
#include "../ReleaseFileset.h"
#include "../Http/MirrorDownload.h"
#include "../Http/HttpRequest.h"
#include "../Constants.h"
#include "../File.h"
#include "../Util.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#ifndef WIN32
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace tdm
{

namespace updater
{

Updater::Updater(const UpdaterOptions& options, const fs::path& executable) :
	_options(options),
	_downloadManager(new DownloadManager),
	_executable(boost::algorithm::to_lower_copy(executable.string())), // convert that file to lower to be sure
	_updatingUpdater(false)
{
	// Set up internet connectivity
	_conn.reset(new HttpConnection);

	// Assign the proxy settings to the connection
	_options.CheckProxy(_conn);

	_ignoreList.insert("doomconfig.cfg");
	_ignoreList.insert("darkmod.cfg");

	MirrorDownload::InitRandomizer();

#ifdef WIN32
	if (fs::extension(_executable).empty())
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Adding EXE extension to executable: " + _executable.string());
		_executable = _executable.string() + ".exe";
	}
#endif
}

void Updater::CleanupPreviousSession()
{
	// Remove batch file from previous run
	File::Remove(GetTargetPath() / TDM_UPDATE_UPDATER_BATCH_FILE);
}

bool Updater::MirrorsNeedUpdate()
{
	fs::path mirrorPath = GetTargetPath() / TDM_MIRRORS_FILE;

	if (!boost::filesystem::exists(mirrorPath))
	{
		// No mirrors file
		TraceLog::WriteLine(LOG_VERBOSE, "No mirrors file present on this machine.");
		return true;
	}

	// File exists, check options
	if (_options.IsSet("keep-mirrors"))
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Skipping mirrors update (--keep-mirrors is set).");
		return false;
	}

	// Update by default
	return true;
}

void Updater::UpdateMirrors()
{
	std::string mirrorsUrl = TDM_MIRRORS_SERVER;
	mirrorsUrl += TDM_MIRRORS_FILE;

//	TraceLog::Write(LOG_VERBOSE, " Downloading mirror list from %s...", mirrorsUrl.c_str() ); // grayman - NG: too many args
	TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Downloading mirror list from %s...") % mirrorsUrl).str()); // grayman - fixed

	fs::path mirrorPath = GetTargetPath() / TDM_MIRRORS_FILE;

	HttpRequestPtr request = _conn->CreateRequest(mirrorsUrl, mirrorPath.string());

	request->Perform();

	if (request->GetStatus() == HttpRequest::OK)
	{
		TraceLog::Write(LOG_VERBOSE, "Done. ");

		// Load the mirrors from the file
		LoadMirrors();
	}
	else
	{
		TraceLog::Error("Mirrors download failed: " + request->GetErrorMessage());
	}
}

void Updater::LoadMirrors()
{
	fs::path mirrorPath = GetTargetPath() / TDM_MIRRORS_FILE;

	// Load the tdm_mirrors.txt into an INI file
	IniFilePtr mirrorsIni = IniFile::ConstructFromFile(mirrorPath);

	// Interpret the info and build the mirror list
	_mirrors = MirrorList(*mirrorsIni);

	TraceLog::WriteLine(LOG_VERBOSE, "Found " + boost::lexical_cast<std::string>(_mirrors.size()) + " mirrors.");
}

std::size_t Updater::GetNumMirrors()
{
	return _mirrors.size();
}

void Updater::GetCrcFromServer()
{
	TraceLog::WriteLine(LOG_VERBOSE, " Downloading CRC information...");

	PerformSingleMirroredDownload(TDM_CRC_INFO_FILE);

	// Parse this file
	IniFilePtr releaseIni = IniFile::ConstructFromFile(GetTargetPath() / TDM_CRC_INFO_FILE);

	if (releaseIni == NULL)
	{
		throw FailureException("Could not download CRC info file from server.");
	}

	// Build the release file set
	_latestRelease = ReleaseFileSet::LoadFromIniFile(*releaseIni);
}

void Updater::GetVersionInfoFromServer()
{
	TraceLog::WriteLine(LOG_VERBOSE, " Downloading version information...");

	PerformSingleMirroredDownload(TDM_VERSION_INFO_FILE);

	// Parse this downloaded file
	IniFilePtr versionInfo = IniFile::ConstructFromFile(GetTargetPath() / TDM_VERSION_INFO_FILE);

	if (versionInfo == NULL) 
	{
		TraceLog::Error("Cannot find downloaded version info file: " + (GetTargetPath() / TDM_VERSION_INFO_FILE).string());
		return;
	}

	_releaseVersions.LoadFromIniFile(*versionInfo);
	_updatePackages.LoadFromIniFile(*versionInfo);
}

void Updater::NotifyFileProgress(const fs::path& file, CurFileInfo::Operation op, double fraction)
{
	if (_fileProgressCallback != NULL)
	{
		CurFileInfo info;
		info.operation = op;
		info.file = file;
		info.progressFraction = fraction;

		_fileProgressCallback->OnFileOperationProgress(info);
	}
}

void Updater::DetermineLocalVersion()
{
	_pureLocalVersion.clear();
	_fileVersions.clear();
	_localVersions.clear();
	_applicableDifferentialUpdates.clear();

	TraceLog::WriteLine(LOG_VERBOSE, " Trying to determine installed TDM version...");

	std::size_t totalItems = 0;

	// Get the total count of version information items, for calculating the progress
	for (ReleaseVersions::const_iterator v = _releaseVersions.begin(); v != _releaseVersions.end(); ++v)
	{
		totalItems += v->second.size();
	}

	std::size_t curItem = 0;

	for (ReleaseVersions::const_iterator v = _releaseVersions.begin(); v != _releaseVersions.end(); ++v)
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Trying to match against version: " + v->first);

		const ReleaseFileSet& set = v->second;

		// Will be true on first mismatching file
		bool mismatch = false;

		for (ReleaseFileSet::const_iterator f = set.begin(); f != set.end(); ++f)
		{
			NotifyFileProgress(f->second.file, CurFileInfo::Check, static_cast<double>(curItem) / totalItems);

			curItem++;
			
			fs::path candidate = GetTargetPath() / f->second.file;

			if (boost::algorithm::to_lower_copy(candidate.leaf().string()) == boost::algorithm::to_lower_copy(_executable.leaf().string()))
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Ignoring updater executable: %s.") % candidate.string()).str());
				continue;
			}

			if (!fs::exists(candidate))
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s is missing.") % candidate.string()).str());
				mismatch = true;
				continue;
			}

			if (f->second.localChangesAllowed) 
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s exists, local changes are allowed, skipping.") % candidate.string()).str());
				continue;
			}

			std::size_t candidateFilesize = static_cast<std::size_t>(fs::file_size(candidate));

			if (candidateFilesize != f->second.filesize)
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching size, expected %d but found %d.")
					% candidate.string() % f->second.filesize % candidateFilesize).str());
				mismatch = true;
				continue;
			}

			// Calculate the CRC of this file
			boost::uint32_t crc = CRC::GetCrcForFile(candidate);

			if (crc != f->second.crc)
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching CRC, expected %x but found %x.")
					% candidate.string() % f->second.crc % crc).str());
				mismatch = true;
				continue;
			}

			// The file is matching - record this version
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s is matching version %s.") % candidate.string() % v->first).str());

			_fileVersions[candidate.string()] = v->first;
		}

		// All files passed the check?
		if (!mismatch) 
		{
			_pureLocalVersion = v->first;
			TraceLog::WriteLine(LOG_VERBOSE, " Local installation matches version: " + _pureLocalVersion);
		}
	}

	// Sum up the totals for all files, each file has exactly one version
	for (FileVersionMap::const_iterator i = _fileVersions.begin(); i != _fileVersions.end(); ++i)
	{
		// sum up the totals for this version
		const std::string& version = i->second;
		VersionTotal& total = _localVersions.insert(LocalVersionBreakdown::value_type(version, VersionTotal())).first->second;

		total.numFiles++;
		total.filesize += static_cast<std::size_t>(fs::file_size(i->first));
	}

	TraceLog::WriteLine(LOG_VERBOSE, (boost::format("The local files are matching %d different versions.") % _localVersions.size()).str());

	if (_fileProgressCallback != NULL)
	{
		_fileProgressCallback->OnFileOperationFinish();
	}

	if (_pureLocalVersion.empty())
	{
		TraceLog::WriteLine(LOG_VERBOSE, " Could not determine local version.");
	}

	if (!_localVersions.empty())
	{
		for (LocalVersionBreakdown::const_iterator i = _localVersions.begin(); i != _localVersions.end(); ++i)
		{
			const std::string& version = i->first;

			TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Files matching version %s: %d (size: %s)") % 
				version % i->second.numFiles % Util::GetHumanReadableBytes(i->second.filesize)).str());

			// Check if this differential update is wise, from an economic point of view
			UpdatePackageInfo::const_iterator package = _updatePackages.find(version);

			if (package != _updatePackages.end())
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Some files match version " + version + ", a differential update is available for that version.");

				UpdatePackageInfo::const_iterator package = _updatePackages.find(version);

				assert(package != _updatePackages.end());

				if (package->second.filesize < i->second.filesize)
				{
					TraceLog::WriteLine(LOG_VERBOSE, "The differential package size is smaller than the total size of files needing it, this is good.");
					_applicableDifferentialUpdates.insert(version);
				}
				else
				{
					TraceLog::WriteLine(LOG_VERBOSE, "The differential package size is larger than the total size of files needing it, will not download that.");
				}
			}
			else
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Some files match version " + version + ", but no differential update is available for that version.");
			}
		}
	}
}

bool Updater::DifferentialUpdateAvailable()
{
	std::string version = "none";

	if (!_pureLocalVersion.empty())
	{
		// Local installation is pure, differential update is possible
		if (DifferentialUpdateAvailableForVersion(_pureLocalVersion))
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Local version is exactly determined, differential update is available.");
			return true;
		}

		TraceLog::WriteLine(LOG_VERBOSE, "Local version is exactly determined, but no differential update is available.");
	}

	// Check applicable differential updates
	if (!_applicableDifferentialUpdates.empty())
	{
		return true;
	}

	TraceLog::WriteLine(LOG_VERBOSE, "No luck, differential updates don't seem to be applicable.");

	return false;
}

bool Updater::DifferentialUpdateAvailableForVersion(const std::string& version)
{
	UpdatePackageInfo::const_iterator it = _updatePackages.find(version);

	return it != _updatePackages.end();
}

std::string Updater::GetNewestVersion()
{
	for (ReleaseVersions::reverse_iterator i = _releaseVersions.rbegin();
		 i != _releaseVersions.rend(); ++i)
	{
		return i->first;
	}

	return "";
}

std::size_t Updater::GetTotalDifferentialUpdateSize()
{
	std::size_t size = 0;

	if (!_pureLocalVersion.empty())
	{
		UpdatePackageInfo::const_iterator it = _updatePackages.find(_pureLocalVersion);

		while (it != _updatePackages.end())
		{
			size += it->second.filesize;

			it = _updatePackages.find(it->second.toVersion);
		}
	}
	else
	{
		std::set<std::string> visitedPackages;

		// Non-pure local install, check all differential update paths - consider duplicate visits
		for (std::set<std::string>::const_iterator i = _applicableDifferentialUpdates.begin(); 
			 i != _applicableDifferentialUpdates.end(); ++i)
		{
			UpdatePackageInfo::const_iterator p = _updatePackages.find(*i);

			while (p != _updatePackages.end())
			{
				if (visitedPackages.find(p->first) == visitedPackages.end())
				{
					visitedPackages.insert(p->first);

					size += p->second.filesize;	
				}

				// Traverse up the version path
				p = _updatePackages.find(p->second.toVersion);
			}
		}
	}

	return size;
}

DifferentialUpdateInfo Updater::GetDifferentialUpdateInfo()
{
	DifferentialUpdateInfo info;

	UpdatePackageInfo::const_iterator it = _updatePackages.find(_pureLocalVersion);

	if (it != _updatePackages.end())
	{
		info.fromVersion = it->second.fromVersion;
		info.toVersion = it->second.toVersion;
		info.filesize = it->second.filesize;
	}
	// Check applicable differential updates, return the info of the first item
	else if (!_applicableDifferentialUpdates.empty())
	{
		std::string firstApplicableVersion = *_applicableDifferentialUpdates.begin();

		it = _updatePackages.find(firstApplicableVersion);

		if (it != _updatePackages.end())
		{
			info.fromVersion = it->second.fromVersion;
			info.toVersion = it->second.toVersion;
			info.filesize = it->second.filesize;
		}
	}

	return info;
}

void Updater::DownloadDifferentialUpdate()
{
	DifferentialUpdateInfo info = GetDifferentialUpdateInfo();

	TraceLog::WriteLine(LOG_VERBOSE, " Downloading differential update package for version " + info.fromVersion);

	// Use the first package available for the local version, even if multiple ones might be registered
	UpdatePackageInfo::const_iterator it = _updatePackages.find(info.fromVersion);

	if (it == _updatePackages.end())
	{
		throw FailureException("Cannot download differential update - nothing found!");
	}

	fs::path packageFilename = it->second.filename;
	fs::path packageTargetPath = GetTargetPath() / packageFilename;

	// Check if the package is already there
	if (VerifyUpdatePackageAt(it->second, packageTargetPath))
	{
		// Skip the download
		TraceLog::WriteLine(LOG_VERBOSE, " Found intact PK4 at target location, skipping download: " + packageFilename.string());
	}
	else
	{
		// Download the file from one of our mirrors, with CRC check
		PerformSingleMirroredDownload(packageFilename.string(), it->second.filesize, it->second.crc);

		if (!VerifyUpdatePackageAt(it->second, packageTargetPath))
		{
			throw FailureException("Failed to download update package: " + packageTargetPath.string());
		}
	}
}

bool Updater::VerifyUpdatePackageAt(const UpdatePackage& info, const fs::path& package)
{
	if (!fs::exists(package))
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("VerifyUpdatePackageAt: File %s does not exist.") % package.string()).str());
		return false;
	}

	if (fs::file_size(package) != info.filesize)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching size, expected %d but found %d.")
			% package.string() % info.filesize % fs::file_size(package)).str());
		return false;
	}

	// Calculate the CRC of this file
	boost::uint32_t crc = CRC::GetCrcForFile(package);

	if (crc != info.crc)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching CRC, expected %x but found %x.")
			% package.string() % info.crc % crc).str());
		return false;
	}

	TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s is intact with checksum %x.")
			% package.string() % crc).str());
	return true; // all checks passed, file is ok
}

void Updater::PerformDifferentialUpdateStep()
{
	DifferentialUpdateInfo updateInfo = GetDifferentialUpdateInfo();

	TraceLog::WriteLine(LOG_VERBOSE, " Applying differential update package for version " + updateInfo.fromVersion);

	// Use the first package available for the local version, even if multiple ones might be registered
	UpdatePackageInfo::iterator it = _updatePackages.find(updateInfo.fromVersion);

	if (it == _updatePackages.end())
	{
		throw FailureException("Cannot apply differential update - nothing found!");
	}

	fs::path packageFilename = it->second.filename;
	fs::path targetPath = GetTargetPath();

	fs::path packageTargetPath = targetPath / packageFilename;

	if (!VerifyUpdatePackageAt(it->second, packageTargetPath))
	{
		throw FailureException("Update package not found at the expected location: " + packageTargetPath.string());
	}

	UpdatePackage& info = it->second;

	ZipFileReadPtr package = Zip::OpenFileRead(packageTargetPath);

	if (package == NULL)
	{
		throw FailureException("Update package cannot be opened: " + packageTargetPath.string());
	}

	{
		std::string updateInfoStr = package->LoadTextFile(TDM_UDPATE_INFO_FILE);

		IniFilePtr iniFile = IniFile::ConstructFromString(updateInfoStr);

		if (iniFile->IsEmpty())
		{
			throw FailureException("Cannot load update info file from that package.");
		}

		// Load the data from the INI file into the UpdatePackage structure
		info.LoadFromIniFile(*iniFile);
	}

	// Some math for the progress meter
	std::size_t totalFileOperations = 0;

	totalFileOperations += info.pk4sToBeRemoved.size();
	totalFileOperations += info.pk4sToBeAdded.size();

	for (UpdatePackage::Pk4DifferenceMap::const_iterator pk4Diff = info.pk4Differences.begin(); 
		 pk4Diff != info.pk4Differences.end(); ++pk4Diff)
	{
		const UpdatePackage::PK4Difference& diff = pk4Diff->second;

		totalFileOperations += diff.membersToBeRemoved.size();
		totalFileOperations += diff.membersToBeReplaced.size();
		totalFileOperations += diff.membersToBeAdded.size();
	}

	totalFileOperations += info.nonArchiveFiles.toBeAdded.size();
	totalFileOperations += info.nonArchiveFiles.toBeRemoved.size();
	totalFileOperations += info.nonArchiveFiles.toBeReplaced.size();

	std::size_t curOperation = 0;

	// Start working
	
	// Remove PK4s as requested
	for (std::set<ReleaseFile>::const_iterator pk4 = info.pk4sToBeRemoved.begin(); 
		 pk4 != info.pk4sToBeRemoved.end(); ++pk4)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Removing PK4: %s") % pk4->file.string()).str());

		NotifyFileProgress(pk4->file, CurFileInfo::Delete, static_cast<double>(curOperation++) / totalFileOperations);
		
		File::Remove(targetPath / pk4->file);
	}

	// Add PK4s as requested
	for (std::set<ReleaseFile>::const_iterator pk4 = info.pk4sToBeAdded.begin(); 
		 pk4 != info.pk4sToBeAdded.end(); ++pk4)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Adding PK4: %s") % pk4->file.string()).str());

		NotifyFileProgress(pk4->file, CurFileInfo::Add, static_cast<double>(curOperation++) / totalFileOperations);

		fs::path targetPk4Path = targetPath / pk4->file;

		package->ExtractFileTo(pk4->file.string(), targetPk4Path);

		if (File::IsZip(targetPk4Path))
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Extracting file after adding package: %s") % pk4->file.string()).str());

			// Extract this ZIP archive after adding it to the local inventory
			ExtractAndRemoveZip(targetPk4Path);
		}
	}

	// Perform in-depth PK4 changes
	for (UpdatePackage::Pk4DifferenceMap::const_iterator pk4Diff = info.pk4Differences.begin(); 
		 pk4Diff != info.pk4Differences.end(); ++pk4Diff)
	{
		TraceLog::Write(LOG_VERBOSE, (boost::format(" Changing PK4: %s...") % pk4Diff->first).str());

		fs::path targetPk4Path = targetPath / pk4Diff->first;

		const UpdatePackage::PK4Difference& diff = pk4Diff->second;

		NotifyFileProgress(pk4Diff->first, CurFileInfo::Check, static_cast<double>(curOperation) / totalFileOperations);
		
		bool fileIsMatching = false;

		// Double-check the PK4 checksum before doing the merge
		try
		{
			boost::uint32_t crc = CRC::GetCrcForFile(targetPk4Path);

			if (crc == diff.checksumBefore)
			{
				fileIsMatching = true;
			}
			else
			{
				TraceLog::WriteLine(LOG_VERBOSE, "  Cannot apply change, PK4 checksum is different.");
			}
		}
		catch (std::runtime_error&)
		{} // leave fileIsMatching at false

		if (!fileIsMatching)
		{
			curOperation += diff.membersToBeRemoved.size();
			curOperation += diff.membersToBeReplaced.size();
			curOperation += diff.membersToBeAdded.size();

			continue;
		}

		std::set<std::string> removeList;

		// Remove members to be removed as first measure
		for (std::set<ReleaseFile>::const_iterator m = diff.membersToBeRemoved.begin();
			 m != diff.membersToBeRemoved.end(); ++m)
		{
			removeList.insert(m->file.string());

			curOperation++;
		}

		// Remove all changed files too
		for (std::set<ReleaseFile>::const_iterator m = diff.membersToBeReplaced.begin();
			 m != diff.membersToBeReplaced.end(); ++m)
		{
			removeList.insert(m->file.string());
		}

		NotifyFileProgress(pk4Diff->first, CurFileInfo::RemoveFilesFromPK4, static_cast<double>(curOperation++) / totalFileOperations);

		// Perform the removal step here
		Zip::RemoveFilesFromArchive(targetPk4Path, removeList);

		// Open the archive for writing (append mode)
		ZipFileWritePtr targetPk4 = Zip::OpenFileWrite(targetPk4Path, Zip::APPEND);

		// Add new members
		for (std::set<ReleaseFile>::const_iterator m = diff.membersToBeAdded.begin();
			 m != diff.membersToBeAdded.end(); ++m)
		{
			targetPk4->CopyFileFromZip(package, m->file.string(), m->file.string());

			NotifyFileProgress(m->file, CurFileInfo::Add, static_cast<double>(curOperation++) / totalFileOperations);
		}
			 
		// Re-add changed members
		for (std::set<ReleaseFile>::const_iterator m = diff.membersToBeReplaced.begin();
			 m != diff.membersToBeReplaced.end(); ++m)
		{
			targetPk4->CopyFileFromZip(package, m->file.string(), m->file.string());

			NotifyFileProgress(m->file, CurFileInfo::Add, static_cast<double>(curOperation++) / totalFileOperations);
		}

		// Close the file
		targetPk4.reset();

		// Calculate CRC after patching
		boost::uint32_t crcAfter = CRC::GetCrcForFile(targetPk4Path);

		if (crcAfter != diff.checksumAfter)
		{
			TraceLog::Error("  Failed applying changes, PK4 checksum is different after patching.");
		}
		else
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" OK - Files added: %d, removed: %d, changed: %d") %
				diff.membersToBeAdded.size() % diff.membersToBeRemoved.size() % diff.membersToBeReplaced.size()).str());
		}
	}

	// Perform non-archive file changes

	// Added files
	for (std::set<ReleaseFile>::const_iterator f = info.nonArchiveFiles.toBeAdded.begin();
		 f != info.nonArchiveFiles.toBeAdded.end(); ++f)
	{
		NotifyFileProgress(f->file, CurFileInfo::Add, static_cast<double>(curOperation++) / totalFileOperations);

		if (_ignoreList.find(boost::algorithm::to_lower_copy(f->file.string())) != _ignoreList.end())
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Ignoring non-archive file: %s") % f->file.string()).str());
			continue;
		}

		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Adding non-archive file: %s") % f->file.string()).str());

		package->ExtractFileTo(f->file.string(), targetPath / f->file);
	}

	// Removed files
	for (std::set<ReleaseFile>::const_iterator f = info.nonArchiveFiles.toBeRemoved.begin();
		 f != info.nonArchiveFiles.toBeRemoved.end(); ++f)
	{
		NotifyFileProgress(f->file, CurFileInfo::Delete, static_cast<double>(curOperation++) / totalFileOperations);

		if (_ignoreList.find(boost::algorithm::to_lower_copy(f->file.string())) != _ignoreList.end())
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Ignoring non-archive file: %s") % f->file.string()).str());
			continue;
		}

		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Removing non-archive file: %s") % f->file.string()).str());

		File::Remove(targetPath / f->file);
	}

	// Changed files
	for (std::set<ReleaseFile>::const_iterator f = info.nonArchiveFiles.toBeReplaced.begin();
		 f != info.nonArchiveFiles.toBeReplaced.end(); ++f)
	{
		NotifyFileProgress(f->file, CurFileInfo::Replace, static_cast<double>(curOperation++) / totalFileOperations);

		if (_ignoreList.find(boost::algorithm::to_lower_copy(f->file.string())) != _ignoreList.end())
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Ignoring non-archive file: %s") % f->file.string()).str());
			continue;
		}

		File::Remove(targetPath / f->file);

		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Replacing non-archive file: %s") % f->file.string()).str());

		package->ExtractFileTo(f->file.string(), targetPath / f->file);
	}

#if WIN32
	std::string tdmExecutableName = "TheDarkMod.exe";
#else 
	std::string tdmExecutableName = "thedarkmod.x86";
#endif

	if (fs::exists(targetPath / tdmExecutableName))
	{
		// Set the executable bit on the TDM binary
		File::MarkAsExecutable(targetPath / tdmExecutableName);
	}

	// Close the ZIP file before removing it
	package.reset();

	if (_fileProgressCallback != NULL)
	{
		_fileProgressCallback->OnFileOperationFinish();
	}

	// Remove the update package after completion
	if (!_options.IsSet("keep-update-packages"))
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Removing package after differential update completion: %s") % packageTargetPath.string()).str());
		File::Remove(packageTargetPath);
	}
	else
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Keeping package after differential update completion: %s") % packageTargetPath.string()).str());
	}

	// grayman #3414 - remove DLL file

#if WIN32
	std::string tdmDLLName = "gamex86.dll";
#else 
	std::string tdmDLLName = "gamex86.so";
#endif

	if (fs::exists(targetPath / tdmDLLName))
	{
		// remove the DLL
		File::Remove(targetPath / tdmDLLName);
	}
}

std::string Updater::GetDeterminedLocalVersion()
{
	return _pureLocalVersion;
}

DownloadPtr Updater::PrepareMirroredDownload(const std::string& remoteFile)
{
	AssertMirrorsNotEmpty();

	fs::path targetPath = GetTargetPath() / remoteFile;

	// Remove target path first
	File::Remove(targetPath);

	// Create a mirrored download
	DownloadPtr download(new MirrorDownload(_conn, _mirrors, remoteFile, targetPath));

	if (File::IsArchive(remoteFile))
	{
		download->EnableValidPK4Check(true);
	}

	return download;
}

void Updater::PerformSingleMirroredDownload(const std::string& remoteFile)
{
	// Create a mirrored download
	DownloadPtr download = PrepareMirroredDownload(remoteFile);

	// Perform and wait for completion
	PerformSingleMirroredDownload(download);
}

void Updater::PerformSingleMirroredDownload(const std::string& remoteFile, std::size_t requiredSize, boost::uint32_t requiredCrc)
{
	DownloadPtr download = PrepareMirroredDownload(remoteFile);

	download->EnableCrcCheck(true);
	download->EnableFilesizeCheck(true);
	download->SetRequiredCrc(requiredCrc);
	download->SetRequiredFilesize(requiredSize);

	// Perform and wait for completion
	PerformSingleMirroredDownload(download);
}

void Updater::PerformSingleMirroredDownload(const DownloadPtr& download)
{
	int downloadId = _downloadManager->AddDownload(download);

	while (_downloadManager->HasPendingDownloads())
	{
		boost::this_thread::interruption_point();

		_downloadManager->ProcessDownloads();

		NotifyDownloadProgress();

		for (int i = 0; i < 50; ++i)
		{
			boost::this_thread::interruption_point();
			Util::Wait(10);
		}
	}

	if (_downloadProgressCallback)
	{
		_downloadProgressCallback->OnDownloadFinish();
	}

	_downloadManager->RemoveDownload(downloadId);
}

fs::path Updater::GetTargetPath()
{
	// Use the target directory 
	if (_options.IsSet("targetdir") && !_options.Get("targetdir").empty())
	{
		return fs::path(_options.Get("targetdir"));
	}

	// Get the current path
	fs::path targetPath = fs::current_path();

	// If the current path is the actual engine path, switch folders to "darkmod"
	// We don't want to download the PK4s into the Doom3.exe location
	/* grayman - no longer necessary
	if (Util::PathIsTDMEnginePath(targetPath))
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Doom3 found in current path, switching directories.");

		targetPath /= TDM_STANDARD_MOD_FOLDER;

		if (!fs::exists(targetPath))
		{
			TraceLog::WriteLine(LOG_VERBOSE, "darkmod/ path not found, creating folder: " + targetPath.string());

			fs::create_directory(targetPath);
		}

		TraceLog::WriteLine(LOG_VERBOSE, " Changed working directory to darkmod/, continuing update process.");
	}
	*/

	return targetPath;
}

void Updater::CheckLocalFiles()
{
	_downloadQueue.clear();

	// Get the current path
	fs::path targetPath = GetTargetPath();

	TraceLog::WriteLine(LOG_VERBOSE, "Checking target folder: " + targetPath.string());

	// List PK4 inventory to logfile, for reference
	for (fs::directory_iterator i = fs::directory_iterator(targetPath); 
		 i != fs::directory_iterator(); ++i)
	{
		if (File::IsPK4(*i))
		{
			TraceLog::WriteLine(LOG_VERBOSE, "[PK4 Inventory] Found " + i->path().string());
		}
	}

	std::size_t count = 0;

	for (ReleaseFileSet::const_iterator i = _latestRelease.begin(); i != _latestRelease.end(); ++i)
	{
		if (_fileProgressCallback != NULL)
		{
			CurFileInfo info;
			info.operation = CurFileInfo::Check;
			info.file = i->second.file;
			info.progressFraction = static_cast<double>(count) / _latestRelease.size();

			_fileProgressCallback->OnFileOperationProgress(info);
		}

		if (i->second.isArchive && !i->second.members.empty())
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Checking archive members of: " + i->second.file.string());

			for (std::set<ReleaseFile>::const_iterator m = i->second.members.begin(); m != i->second.members.end(); ++m)
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Checking for member file: " + m->file.string());

				if (!CheckLocalFile(targetPath, *m))
				{
					// A member is missing or out of date, mark the archive for download
					_downloadQueue.insert(*i);
				}
			}
		}
		else
		{
			TraceLog::Write(LOG_VERBOSE, "Checking for archive file: " + i->second.file.string() + "...");

			if (!CheckLocalFile(targetPath, i->second))
			{
				// A member is missing or out of date, mark the archive for download
				_downloadQueue.insert(*i);
			}
		}

		count++;
	}

	if (_fileProgressCallback != NULL)
	{
		_fileProgressCallback->OnFileOperationFinish();
	}

	if (NewUpdaterAvailable())
	{
		// Remove all download packages from the queue, except the one containing the updater
		RemoveAllPackagesExceptUpdater();
	}
}

bool Updater::CheckLocalFile(const fs::path& installPath, const ReleaseFile& releaseFile)
{
	boost::this_thread::interruption_point();

	fs::path localFile = installPath / releaseFile.file;

	TraceLog::Write(LOG_VERBOSE, " Checking for file " + releaseFile.file.string() + ": ");

	if (fs::exists(localFile))
	{
		// File exists, check ignore list
		if (_ignoreList.find(boost::algorithm::to_lower_copy(releaseFile.file.string())) != _ignoreList.end())
		{
			TraceLog::WriteLine(LOG_VERBOSE, "OK, file will not be updated. ");
			return true; // ignore this file
		}

		// Compare file size
		std::size_t fileSize = static_cast<std::size_t>(fs::file_size(localFile));

		if (fileSize != releaseFile.filesize)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "SIZE MISMATCH");
			return false;
		}

		// Size is matching, check CRC
		boost::uint32_t existingCrc = CRC::GetCrcForFile(localFile);

		if (existingCrc == releaseFile.crc)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "OK");
			return true;
		}
		else
		{
			TraceLog::WriteLine(LOG_VERBOSE, "CRC MISMATCH");
			return false;
		}
	}
	else
	{
		TraceLog::WriteLine(LOG_VERBOSE, "MISSING");
		return false;
	}
}

bool Updater::LocalFilesNeedUpdate()
{
	return !_downloadQueue.empty();
}

void Updater::PrepareUpdateStep()
{
	fs::path targetPath = GetTargetPath();

	// Create a download for each of the files
	for (ReleaseFileSet::iterator i = _downloadQueue.begin(); i != _downloadQueue.end(); ++i)
	{
		boost::this_thread::interruption_point();

		// Create a mirrored download
		DownloadPtr download(new MirrorDownload(_conn, _mirrors, i->second.file.string(), targetPath / i->second.file));

		// Check archives after download, pass crc and filesize to download
		if (File::IsArchive(i->second.file))
		{
			download->EnableValidPK4Check(true);
			download->EnableCrcCheck(true);
			download->EnableFilesizeCheck(true);

			download->SetRequiredCrc(i->second.crc);
			download->SetRequiredFilesize(i->second.filesize);
		}

		i->second.downloadId = _downloadManager->AddDownload(download);
	}
}

void Updater::PerformUpdateStep()
{
	// Wait until the download is done
	while (_downloadManager->HasPendingDownloads())
	{
		// For catching terminations
		boost::this_thread::interruption_point();

		_downloadManager->ProcessDownloads();

		if (_downloadManager->HasFailedDownloads())
		{
			throw FailureException("Could not download from any mirror.");
		}

		NotifyDownloadProgress();

		NotifyFullUpdateProgress();

		Util::Wait(100);
	}

	if (_downloadManager->HasFailedDownloads())
	{
		throw FailureException("Could not download from any mirror.");
	}

	if (_downloadProgressCallback)
	{
		_downloadProgressCallback->OnDownloadFinish();
	}

	// Check if any ZIP files have been downloaded, these need to be extracted
	for (ReleaseFileSet::iterator i = _downloadQueue.begin(); i != _downloadQueue.end(); ++i)
	{
		boost::this_thread::interruption_point();

		DownloadPtr download = _downloadManager->GetDownload(i->second.downloadId);

		if (download != NULL && download->GetStatus() == Download::SUCCESS)
		{
			if (File::IsZip(download->GetDestFilename()))
			{
				// Extract this ZIP archive after download
				ExtractAndRemoveZip(download->GetDestFilename());
			}
		}
	}
}

void Updater::NotifyFullUpdateProgress()
{
	if (_downloadProgressCallback == NULL)
	{
		return;
	}

	std::size_t totalDownloadSize = GetTotalDownloadSize();
	std::size_t totalBytesDownloaded = 0;

	for (ReleaseFileSet::iterator i = _downloadQueue.begin(); i != _downloadQueue.end(); ++i)
	{
		boost::this_thread::interruption_point();

		if (i->second.downloadId == -1)
		{
			continue;
		}

		DownloadPtr download = _downloadManager->GetDownload(i->second.downloadId);

		if (download == NULL) continue;

		if (download->GetStatus() == Download::SUCCESS)
		{
			totalBytesDownloaded += i->second.filesize;
		}
		else if (download->GetStatus() == Download::IN_PROGRESS)
		{
			totalBytesDownloaded += download->GetDownloadedBytes();
		}
	}

	OverallDownloadProgressInfo info;

	if (totalBytesDownloaded > totalDownloadSize)
	{
		totalBytesDownloaded = totalDownloadSize;
	}

	info.updateType = OverallDownloadProgressInfo::Full;
	info.totalDownloadSize = totalDownloadSize;
	info.bytesLeftToDownload = totalDownloadSize - totalBytesDownloaded;
	info.downloadedBytes = totalBytesDownloaded;
	info.progressFraction = static_cast<double>(totalBytesDownloaded) / totalDownloadSize;
	info.filesToDownload = _downloadQueue.size();
	
	_downloadProgressCallback->OnOverallProgress(info);
}

void Updater::NotifyDownloadProgress()
{
	int curDownloadId = _downloadManager->GetCurrentDownloadId();
	
	if (curDownloadId != -1 && _downloadProgressCallback)
	{
		DownloadPtr curDownload = _downloadManager->GetCurrentDownload();

		CurDownloadInfo info;

		info.file = curDownload->GetFilename();
		info.progressFraction = curDownload->GetProgressFraction();
		info.downloadSpeed = curDownload->GetDownloadSpeed();
		info.downloadedBytes = curDownload->GetDownloadedBytes();

		MirrorDownloadPtr mirrorDownload = boost::dynamic_pointer_cast<MirrorDownload>(curDownload);

		if (mirrorDownload != NULL)
		{
			info.mirrorDisplayName = mirrorDownload->GetCurrentMirrorName();
		}

		_downloadProgressCallback->OnDownloadProgress(info);
	}
}

void Updater::ExtractAndRemoveZip(const fs::path& zipFilePath)
{
	ZipFileReadPtr zipFile = Zip::OpenFileRead(zipFilePath);

	if (zipFile == NULL)
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Archive is not a valid ZIP.");
		return;
	}

	fs::path destPath = GetTargetPath();

	// tdm_update exists in its own PK4, so we can assume tdm_update and the TDM binaries will never be found in the same PK4.

	bool TDMbinaryPresent = false; // grayman - true if this zip contains "TheDarkMod.exe" or "thedarkmod.x86"

#ifdef WIN32
	const std::string TDM_BINARY_NAME("TheDarkMod.exe");
#else
	const std::string TDM_BINARY_NAME("thedarkmod.x86");
#endif

	try
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Extracting files from " + zipFilePath.string());

		// Check if the archive contains the updater binary
		if (zipFile->ContainsFile(_executable.string()))
		{
			// Set the flag for later use
			_updatingUpdater = true;
		}

		// Check if the archive contains the TDM binary
		if (zipFile->ContainsFile(TDM_BINARY_NAME))
		{
			// Set the flag for later use
			TDMbinaryPresent = true;
		}

		std::list<fs::path> extractedFiles;

		if (_updatingUpdater)
		{
			// Update all files, but save the updater binary; this will be handled separately
			std::set<std::string> hardIgnoreList;
			hardIgnoreList.insert(_executable.string());

			// Extract all but the updater
			// Ignore DoomConfig.cfg, etc. if already existing
			extractedFiles = zipFile->ExtractAllFilesTo(destPath, _ignoreList, hardIgnoreList); 

			// Extract the updater to a temporary filename
			fs::path tempUpdater = destPath / ("_" + _executable.string());

			zipFile->ExtractFileTo(_executable.string(), tempUpdater);

			// Set the executable bit on the temporary updater
			File::MarkAsExecutable(tempUpdater);

			// Prepare the update batch file
			PrepareUpdateBatchFile(tempUpdater);
		}
		else if (TDMbinaryPresent)
		{
			// Update all files, but save the TDM binary; this will be handled separately
			std::set<std::string> hardIgnoreList;
			hardIgnoreList.insert(TDM_BINARY_NAME);

			// Extract all but the TDM binary
			// Ignore DoomConfig.cfg, etc. if already existing
			extractedFiles = zipFile->ExtractAllFilesTo(destPath, _ignoreList, hardIgnoreList); 

			// Extract the TDM binary
			fs::path binaryFileName = destPath / TDM_BINARY_NAME;
			zipFile->ExtractFileTo(TDM_BINARY_NAME, binaryFileName);

			// Set the executable bit on the TDM binary
			File::MarkAsExecutable(binaryFileName);
		}
		else
		{
			// Regular archive (without updater or TDM binary), extract all files, ignore existing DoomConfig.cfg
			extractedFiles = zipFile->ExtractAllFilesTo(destPath, _ignoreList);
		}

		TraceLog::WriteLine(LOG_VERBOSE, "All files successfully extracted from " + zipFilePath.string());

#ifndef WIN32
		// In Linux or Mac, mark *.linux files as executable after extraction
		for (std::list<fs::path>::const_iterator i = extractedFiles.begin(); i != extractedFiles.end(); ++i)
		{
			std::string extension = boost::to_lower_copy(fs::extension(*i));

			if (extension == ".linux" || extension == ".macosx")
			{
				File::MarkAsExecutable(*i);
			}
		}
#endif
		
		// Close the zip file before removal
		zipFile.reset();

		// Remove the Zip
		File::Remove(zipFilePath);
	}
	catch (std::runtime_error& ex)
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Failed to extract files from " + zipFilePath.string() + ": " + ex.what());
	}
}

void Updater::PrepareUpdateBatchFile(const fs::path& temporaryUpdater)
{
	// Create a new batch file in the target location
	_updateBatchFile = GetTargetPath() / TDM_UPDATE_UPDATER_BATCH_FILE;

	TraceLog::WriteLine(LOG_VERBOSE, "Preparing TDM update batch file in " + _updateBatchFile.string());

	std::ofstream batch(_updateBatchFile.string().c_str());

	fs::path tempUpdater = temporaryUpdater.leaf();
	fs::path updater = _executable.leaf();

	// Append the current set of command line arguments to the new instance
	std::string arguments;

	for (std::vector<std::string>::const_iterator i = _options.GetRawCmdLineArgs().begin();
		 i != _options.GetRawCmdLineArgs().end(); ++i)
	{
		arguments += " " + *i;
	}

#ifdef WIN32
	batch << "@ping 127.0.0.1 -n 2 -w 1000 > nul" << std::endl; // # hack equivalent to Wait 2
	batch << "@copy " << tempUpdater.string() << " " << updater.string() << " >nul" << std::endl;
	batch << "@del " << tempUpdater.string() << std::endl;
	batch << "@echo TDM Updater executable has been updated." << std::endl;

	batch << "@echo Re-launching TDM Updater executable." << std::endl << std::endl;

	batch << "@start " << updater.string() << " " << arguments;
#else // POSIX
	// grayman - accomodate spaces in pathnames
	tempUpdater = GetTargetPath() / tempUpdater;
	updater = GetTargetPath() / updater;

	batch << "#!/bin/bash" << std::endl;
	batch << "echo \"Upgrading TDM Updater executable...\"" << std::endl;
	batch << "cd \"" << GetTargetPath().string() << "\"" << std::endl; 
	batch << "sleep 2s" << std::endl;
	batch << "mv -f \"" << tempUpdater.string() << "\" \"" << updater.string() << "\"" << std::endl;
	batch << "chmod +x \"" << updater.string() << "\"" << std::endl;
	batch << "echo \"TDM Updater executable has been updated.\"" << std::endl;
	batch << "echo \"Re-launching TDM Updater executable.\"" << std::endl;

	batch << "\"" << updater.string() << "\" " << arguments;
#endif

	batch.close();

#ifndef WIN32
	// Mark the shell script as executable in *nix
	File::MarkAsExecutable(_updateBatchFile);
#endif
}

void Updater::CleanupUpdateStep()
{
	for (ReleaseFileSet::iterator i = _downloadQueue.begin(); i != _downloadQueue.end(); ++i)
	{
		_downloadManager->RemoveDownload(i->second.downloadId);
	}

	_downloadQueue.clear();
}

void Updater::AssertMirrorsNotEmpty()
{
	if (_mirrors.empty())
	{
		throw FailureException("No mirror information, cannot continue.");
	}
}

std::size_t Updater::GetNumFilesToBeUpdated()
{
	return _downloadQueue.size();
}

std::size_t Updater::GetTotalDownloadSize()
{
	std::size_t totalSize = 0;

	for (ReleaseFileSet::iterator i = _downloadQueue.begin(); i != _downloadQueue.end(); ++i)
	{
		totalSize += i->second.filesize;
	}

	return totalSize;
}

std::size_t Updater::GetTotalBytesDownloaded()
{
	return _conn->GetBytesDownloaded();
}

void Updater::SetDownloadProgressCallback(const DownloadProgressPtr& callback)
{
	_downloadProgressCallback = callback;
}

void Updater::ClearDownloadProgressCallback()
{
	_downloadProgressCallback.reset();
}

void Updater::SetFileOperationProgressCallback(const FileOperationProgressPtr& callback)
{
	_fileProgressCallback = callback;
}

void Updater::ClearFileOperationProgressCallback()
{
	_fileProgressCallback.reset();
}

bool Updater::NewUpdaterAvailable()
{
	if (_options.IsSet("noselfupdate"))
	{
		return false; // no self-update overrides everything
	}

	TraceLog::WriteLine(LOG_VERBOSE, "Looking for executable " + _executable.string() + " in download queue.");
	
	// Is this the updater?
	for (ReleaseFileSet::const_iterator i = _downloadQueue.begin(); i != _downloadQueue.end(); ++i)
	{
		if (i->second.ContainsUpdater(_executable.string()))
		{
			TraceLog::WriteLine(LOG_VERBOSE, "The tdm_update binary needs to be updated.");
			return true;
		}
	}

	TraceLog::WriteLine(LOG_VERBOSE, "Didn't find executable name " + _executable.string() + " in download queue.");

	return false;
}

void Updater::RemoveAllPackagesExceptUpdater()
{
	TraceLog::WriteLine(LOG_VERBOSE, "Removing all packages, except the one containing the updater");

	for (ReleaseFileSet::iterator i = _downloadQueue.begin(); i != _downloadQueue.end(); /* in-loop */)
	{
		if (i->second.ContainsUpdater(_executable.string()))
		{
			// This package contains the updater, keep it
			++i;
		}
		else
		{
			// The inner loop didn't find the executable, remove that package
			_downloadQueue.erase(i++);
		}
	}
}

bool Updater::RestartRequired()
{
	return _updatingUpdater;
}

void Updater::RestartUpdater()
{
	TraceLog::WriteLine(LOG_VERBOSE, "Preparing restart...");

#ifdef WIN32
	if (!_updateBatchFile.empty())
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Update batch file pending, launching process.");
		
		// Spawn a new process

		// Create a tdmlauncher process, setting the working directory to the target directory
		STARTUPINFO siStartupInfo;
		PROCESS_INFORMATION piProcessInfo;

		memset(&siStartupInfo, 0, sizeof(siStartupInfo));
		memset(&piProcessInfo, 0, sizeof(piProcessInfo));

		siStartupInfo.cb = sizeof(siStartupInfo);

		fs::path parentPath = _updateBatchFile;
		parentPath.remove_leaf().remove_leaf();

		TraceLog::WriteLine(LOG_VERBOSE, "Starting batch file " + _updateBatchFile.string() + " in " + parentPath.string());

		BOOL success = CreateProcess(NULL, (LPSTR) _updateBatchFile.string().c_str(), NULL, NULL,  false, 0, NULL,
			parentPath.string().c_str(), &siStartupInfo, &piProcessInfo);

		if (!success)
		{
			LPVOID lpMsgBuf;

			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						  NULL,
						  GetLastError(),
						  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						  (LPTSTR) &lpMsgBuf,
						  0,
						  NULL);

			throw FailureException("Could not start new process: " + std::string((LPCTSTR)lpMsgBuf));
			
			LocalFree(lpMsgBuf);
		}
		else
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Process started");
		}
	}
#else

	if (!_updateBatchFile.empty())
	{
		TraceLog::WriteLine(LOG_STANDARD, "Relaunching tdm_update via shell script " + _updateBatchFile.string());

		// Perform the system command in a fork
		if (fork() == 0)
		{
			// Don't wait for the subprocess to finish
			system((_updateBatchFile.string() + " &").c_str());
			exit(EXIT_SUCCESS);
			return;
		}

		TraceLog::WriteLine(LOG_VERBOSE, "Process spawned.");

		// Done here too
		return;
	}
#endif
}

void Updater::PostUpdateCleanup()
{
	for (fs::directory_iterator i = fs::directory_iterator(GetTargetPath()); 
		i != fs::directory_iterator(); )
	{
		if (boost::algorithm::starts_with(i->path().string(), TMP_FILE_PREFIX))
		{
			File::Remove(*i++);
		}
		else
		{
			++i;
		}
	}
}

void Updater::CancelDownloads()
{
	_downloadManager->ClearDownloads();
}

} // namespace

} // namespace
