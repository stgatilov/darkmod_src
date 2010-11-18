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

namespace tdm
{

namespace updater
{

Updater::Updater(const UpdaterOptions& options, const fs::path& executable) :
	_options(options),
	_downloadManager(new DownloadManager),
	_executable(boost::algorithm::to_lower_copy(executable.file_string())), // convert that file to lower to be sure
	_updatingUpdater(false)
{
	// Set up internet connectivity
	_conn.reset(new HttpConnection);

	// Assign the proxy settings to the connection
	_options.CheckProxy(_conn);

	_ignoreList.insert("doomconfig.cfg");
	_ignoreList.insert("dmargs.txt");

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

	TraceLog::Write(LOG_VERBOSE, " Downloading mirror list from bloodgate.com...");

	fs::path mirrorPath = GetTargetPath() / TDM_MIRRORS_FILE;

	HttpRequestPtr request = _conn->CreateRequest(mirrorsUrl, mirrorPath.file_string());

	request->Perform();

	if (request->GetStatus() == HttpRequest::OK)
	{
		TraceLog::Write(LOG_VERBOSE, "done. ");

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
		TraceLog::Error("Cannot find downloaded version info file: " + (GetTargetPath() / TDM_VERSION_INFO_FILE).file_string());
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
		info.overallProgressFraction = fraction;

		_fileProgressCallback->OnFileOperationProgress(info);
	}
}

void Updater::DetermineLocalVersion()
{
	_localVersion.clear();

	TraceLog::WriteLine(LOG_VERBOSE, " Trying to determine installed TDM version...");

	for (ReleaseVersions::const_iterator v = _releaseVersions.begin(); v != _releaseVersions.end(); ++v)
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Trying to match against version: " + v->first);

		const ReleaseFileSet& set = v->second;

		// Will be true on first mismatching file
		bool mismatch = false;

		std::size_t count = 0;

		for (ReleaseFileSet::const_iterator f = set.begin(); f != set.end(); ++f)
		{
			NotifyFileProgress(f->second.file, CurFileInfo::Check, static_cast<double>(count) / set.size());
			
			fs::path candidate = GetTargetPath() / f->second.file;

			if (boost::algorithm::to_lower_copy(candidate.leaf()) == boost::algorithm::to_lower_copy(_executable.leaf()))
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Ignoring updater executable: %s.") % candidate.file_string()).str());
				continue;
			}

			if (!fs::exists(candidate))
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s is missing.") % candidate.file_string()).str());
				mismatch = true;
				break;
			}

			if (f->second.localChangesAllowed) 
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s exists, local changes are allowed, skipping.") % candidate.file_string()).str());
				continue;
			}

			if (fs::file_size(candidate) != f->second.filesize)
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching size, expected %d but found %d.")
					% candidate.file_string() % f->second.filesize % fs::file_size(candidate)).str());
				mismatch = true;
				break;
			}

			// Calculate the CRC of this file
			boost::uint32_t crc = CRC::GetCrcForFile(candidate);

			if (crc != f->second.crc)
			{
				TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching CRC, expected %x but found %x.")
					% candidate.file_string() % f->second.crc % crc).str());
				mismatch = true;
				break;
			}

			count++;
		}

		// All files passed the check?
		if (!mismatch) 
		{
			_localVersion = v->first;
			TraceLog::WriteLine(LOG_VERBOSE, " Local installation matches version: " + _localVersion);
			break;
		}
	}

	if (_fileProgressCallback != NULL)
	{
		_fileProgressCallback->OnFileOperationFinish();
	}

	if (_localVersion.empty())
	{
		TraceLog::WriteLine(LOG_VERBOSE, " Could not determine local version.");
	}
}

bool Updater::DifferentialUpdateAvailable()
{
	if (_localVersion.empty())
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Local version is not determined, no differential update can be applied.");
		return false;
	}

	// Check if an update package is available for this version
	for (UpdatePackageInfo::const_iterator it = _updatePackages.find(_localVersion);
		 it != _updatePackages.upper_bound(_localVersion) && it != _updatePackages.end();
		 ++it)
	{
		return true;
	}

	return false;
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

	UpdatePackageInfo::const_iterator it = _updatePackages.find(_localVersion);

	while (it != _updatePackages.end())
	{
		size += it->second.filesize;

		it = _updatePackages.find(it->second.toVersion);
	}

	return size;
}

DifferentialUpdateInfo Updater::GetDifferentialUpdateInfo()
{
	UpdatePackageInfo::const_iterator it = _updatePackages.find(_localVersion);

	DifferentialUpdateInfo info;

	if (it != _updatePackages.end())
	{
		info.fromVersion = it->second.fromVersion;
		info.toVersion = it->second.toVersion;
		info.filesize = it->second.filesize;
	}

	return info;
}

void Updater::DownloadDifferentialUpdate()
{
	TraceLog::WriteLine(LOG_VERBOSE, " Downloading differential update package for version " + _localVersion);

	// Use the first package available for the local version, even if multiple ones might be registered
	UpdatePackageInfo::const_iterator it = _updatePackages.find(_localVersion);

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
		// Download the file from one of our mirrors
		PerformSingleMirroredDownload(packageFilename.string());

		if (!VerifyUpdatePackageAt(it->second, packageTargetPath))
		{
			throw FailureException("Failed to download update package: " + packageTargetPath.file_string());
		}
	}
}

bool Updater::VerifyUpdatePackageAt(const UpdatePackage& info, const fs::path& package)
{
	if (!fs::exists(package))
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("VerifyUpdatePackageAt: File %s does not exist.") % package.file_string()).str());
		return false;
	}

	if (fs::file_size(package) != info.filesize)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching size, expected %d but found %d.")
			% package.file_string() % info.filesize % fs::file_size(package)).str());
		return false;
	}

	// Calculate the CRC of this file
	boost::uint32_t crc = CRC::GetCrcForFile(package);

	if (crc != info.crc)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s has mismatching CRC, expected %x but found %x.")
			% package.file_string() % info.crc % crc).str());
		return false;
	}

	TraceLog::WriteLine(LOG_VERBOSE, (boost::format("File %s is intact with checksum %x.")
			% package.file_string() % crc).str());
	return true; // all checks passed, file is ok
}

void Updater::PerformDifferentialUpdateStep()
{
	TraceLog::WriteLine(LOG_VERBOSE, " Applying differential update package for version " + _localVersion);

	// Use the first package available for the local version, even if multiple ones might be registered
	UpdatePackageInfo::iterator it = _updatePackages.find(_localVersion);

	if (it == _updatePackages.end())
	{
		throw FailureException("Cannot apply differential update - nothing found!");
	}

	fs::path packageFilename = it->second.filename;
	fs::path targetPath = GetTargetPath();

	fs::path packageTargetPath = targetPath / packageFilename;

	if (!VerifyUpdatePackageAt(it->second, packageTargetPath))
	{
		throw FailureException("Update package not found at the expected location: " + packageTargetPath.file_string());
	}

	UpdatePackage& info = it->second;

	ZipFileReadPtr package = Zip::OpenFileRead(packageTargetPath);

	if (package == NULL)
	{
		throw FailureException("Update package cannot be opened: " + packageTargetPath.file_string());
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

		package->ExtractFileTo(pk4->file.string(), targetPath / pk4->file.string());
	}

	// Perform in-depth PK4 changes
	for (UpdatePackage::Pk4DifferenceMap::const_iterator pk4Diff = info.pk4Differences.begin(); 
		 pk4Diff != info.pk4Differences.end(); ++pk4Diff)
	{
		TraceLog::Write(LOG_VERBOSE, (boost::format(" Changing PK4: %s...") % pk4Diff->first).str());

		fs::path targetPk4Path = targetPath / pk4Diff->first;
		
		// Double-check the PK4 checksum before doing the merge
		boost::uint32_t crc = CRC::GetCrcForFile(targetPk4Path);

		const UpdatePackage::PK4Difference& diff = pk4Diff->second;

		if (crc != diff.checksumBefore)
		{
			TraceLog::Error("  Cannot apply change, PK4 checksum is different.");
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

	// Close the ZIP file before removing it
	package.reset();

	if (_fileProgressCallback != NULL)
	{
		_fileProgressCallback->OnFileOperationFinish();
	}

	// Remove the update package after completion
	if (!_options.IsSet("keep-update-packages"))
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Removing package after differential update completion: %s") % packageTargetPath.file_string()).str());
		File::Remove(packageTargetPath);
	}
	else
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format(" Keeping package after differential update completion: %s") % packageTargetPath.file_string()).str());
	}
}

std::string Updater::GetDeterminedLocalVersion()
{
	return _localVersion;
}

void Updater::PerformSingleMirroredDownload(const std::string& remoteFile)
{
	AssertMirrorsNotEmpty();

	fs::path targetPath = GetTargetPath() / remoteFile;

	// Remove target path first
	File::Remove(targetPath);

	// Create a mirrored download
	DownloadPtr download(new MirrorDownload(_conn, _mirrors, remoteFile, targetPath));

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
	if (_options.IsSet("targetdir"))
	{
		return fs::path(_options.Get("targetdir"));
	}

	// Get the current path
	fs::path targetPath = fs::current_path();

	// If the current path is the actual engine path, switch folders to "darkmod"
	// We don't want to download the PK4s into the DOOM3.exe location
	if (Util::PathIsDoom3EnginePath(targetPath))
	{
		TraceLog::WriteLine(LOG_VERBOSE, "D3 engine found in current path, switching directories.");

		targetPath /= TDM_STANDARD_MOD_FOLDER;

		if (!fs::exists(targetPath))
		{
			TraceLog::WriteLine(LOG_VERBOSE, "D3/darkmod path not found, creating folder: " + targetPath.file_string());

			fs::create_directory(targetPath);
		}

		TraceLog::WriteLine(LOG_VERBOSE, " Changed working directory to darkmod/, continuing update process.");
	}

	return targetPath;
}

void Updater::CheckLocalFiles()
{
	_downloadQueue.clear();

	// Get the current path
	fs::path targetPath = GetTargetPath();

	TraceLog::WriteLine(LOG_VERBOSE, "Checking target folder: " + targetPath.file_string());

	std::size_t count = 0;

	for (ReleaseFileSet::const_iterator i = _latestRelease.begin(); i != _latestRelease.end(); ++i)
	{
		if (_fileProgressCallback != NULL)
		{
			CurFileInfo info;
			info.operation = CurFileInfo::Check;
			info.file = i->second.file;
			info.overallProgressFraction = static_cast<double>(count) / _latestRelease.size();

			_fileProgressCallback->OnFileOperationProgress(info);
		}

		if (i->second.isArchive && !i->second.members.empty())
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Checking archive members of: " + i->second.file.file_string());

			for (std::set<ReleaseFile>::const_iterator m = i->second.members.begin(); m != i->second.members.end(); ++m)
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Checking for member file: " + m->file.file_string());

				if (!CheckLocalFile(targetPath, *m))
				{
					// A member is missing or out of date, mark the archive for download
					_downloadQueue.insert(*i);
				}
			}
		}
		else
		{
			TraceLog::Write(LOG_VERBOSE, "Checking for archive file: " + i->second.file.file_string() + "...");

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

	TraceLog::Write(LOG_VERBOSE, " Checking for file " + releaseFile.file.file_string() + ": ");

	if (fs::exists(localFile))
	{
		// File exists, check ignore list
		if (_ignoreList.find(boost::algorithm::to_lower_copy(releaseFile.file.file_string())) != _ignoreList.end())
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

		// Check archives after download
		if (File::IsArchive(i->second.file))
		{
			download->EnableValidPK4Check(true);
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

	try
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Extracting files from " + zipFilePath.file_string());

		// Check if the archive contains the updater binary
		if (zipFile->ContainsFile(_executable.string()))
		{
			// Set the flag for later use
			_updatingUpdater = true;
		}

#ifdef WIN32
		if (zipFile->ContainsFile(_executable.string()))
		{
			// Update all files, but save the updater binary, this will be handled seperately
			std::set<std::string> hardIgnoreList;
			hardIgnoreList.insert(_executable.string());

			// Extract all but the updater
			// Ignore DoomConfig.cfg, etc. if already existing
			zipFile->ExtractAllFilesTo(destPath, _ignoreList, hardIgnoreList); 

			// Extract the updater to a temporary filename
			zipFile->ExtractFileTo(_executable.string(), destPath / ("_" + _executable.string()));

			// Prepare the update batch file
			PrepareUpdateBatchFile(destPath / ("_" + _executable.string()));
		}
		else
		{
			// Regular archive (without updater), extract all files, ignore existing DoomConfig.cfg
			zipFile->ExtractAllFilesTo(destPath, _ignoreList);
		}
#else	
		// Non-Win32 case: just extract all files, ignore existing DoomConfig.cfg
		zipFile->ExtractAllFilesTo(destPath, _ignoreList);
#endif

		TraceLog::WriteLine(LOG_VERBOSE, "All files successfully extracted from " + zipFilePath.file_string());
		
		// Close the zip file before removal
		zipFile.reset();

		// Remove the Zip
		File::Remove(zipFilePath);
	}
	catch (std::runtime_error& ex)
	{
		TraceLog::WriteLine(LOG_VERBOSE, "Failed to extract files from " + zipFilePath.file_string() + ": " + ex.what());
	}
}

#ifdef WIN32
void Updater::PrepareUpdateBatchFile(const fs::path& temporaryUpdater)
{
	// Create a new batch file in the target location
	_updateBatchFile = GetTargetPath() / TDM_UPDATE_UPDATER_BATCH_FILE;

	TraceLog::WriteLine(LOG_VERBOSE, "Preparing TDM update batch file in " + _updateBatchFile.file_string());

	std::ofstream batch(_updateBatchFile.file_string().c_str());

	fs::path tempUpdater = temporaryUpdater.leaf();
	fs::path updater = _executable.leaf();

	batch << "@ping 127.0.0.1 -n 2 -w 1000 > nul" << std::endl; // # hack equivalent to Wait 2
	batch << "@copy " << tempUpdater.file_string() << " " << updater.file_string() << " >nul" << std::endl;
	batch << "@del " << tempUpdater.file_string() << std::endl;
	batch << "@echo TDM Updater executable has been updated." << std::endl;

	batch << "@echo Re-launching TDM Updater executable." << std::endl << std::endl;

	// Append the current set of command line arguments to the new instance
	std::string arguments;

	for (std::vector<std::string>::const_iterator i = _options.GetRawCmdLineArgs().begin();
		 i != _options.GetRawCmdLineArgs().end(); ++i)
	{
		arguments += " " + *i;
	}

	batch << "@" << updater.file_string() << " " << arguments;
}
#endif

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

		TraceLog::WriteLine(LOG_VERBOSE, "Starting batch file " + _updateBatchFile.file_string() + " in " + parentPath.file_string());

		CreateProcess(NULL, (LPSTR) _updateBatchFile.file_string().c_str(), NULL, NULL,  false, 0, NULL,
			parentPath.file_string().c_str(), &siStartupInfo, &piProcessInfo);
	}
#else
	// TODO: Implement relaunch for *nix systems
	TraceLog::WriteLine(LOG_VERBOSE, "Please re-run tdm_update to complete the update of your TDM installation!");
#endif
}

void Updater::PostUpdateCleanup()
{
	for (fs::directory_iterator i = fs::directory_iterator(GetTargetPath()); 
		i != fs::directory_iterator(); )
	{
		if (boost::algorithm::starts_with(i->string(), TMP_FILE_PREFIX))
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
