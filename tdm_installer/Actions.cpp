#include "Actions.h"
#include "StdFilesystem.h"
#include "StdString.h"
#include "LogUtils.h"
#include "OsUtils.h"
#include "CommandLine.h"
#include "ChecksummedZip.h"
#include "Downloader.h"
#include "Utils.h"
#include "ZipSync.h"
#include "Constants.h"
#include "State.h"

//=======================================================================================

static std::vector<std::string> CollectTdmZipPaths(const std::string &installDir) {
	std::vector<std::string> res;
	auto allPaths = stdext::recursive_directory_enumerate(installDir);
	for (const auto &entry : allPaths) {
		if (stdext::is_regular_file(entry)) {
			std::string absPath = entry.string();
			std::string relPath = ZipSync::PathAR::FromAbs(absPath, installDir).rel;
			bool managed = false;

			//common categories:
			if (stdext::istarts_with(relPath, "tdm_") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. tdm_ai_base01.pk4
			if (stdext::istarts_with(relPath, "tdm_") && stdext::iends_with(relPath, ".zip"))
				managed = true;		//e.g. tdm_shared_stuff.zip
			if (stdext::istarts_with(relPath, "fms/tdm_") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/tdm_training_mission/tdm_training_mission.pk4

			//hardcoded prepackaged FMs:
			if (stdext::istarts_with(relPath, "fms/newjob/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/newjob/newjob.pk4
			if (stdext::istarts_with(relPath, "fms/stlucia/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/stlucia/stlucia.pk4
			if (stdext::istarts_with(relPath, "fms/saintlucia/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/saintlucia/saintlucia.pk4
			if (stdext::istarts_with(relPath, "fms/training_mission/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/training_mission/training_mission.pk4

			if (managed)
				res.push_back(absPath);
		}
	}
	return res;
}

static std::vector<std::string> CollectTdmUnpackedFilesToDelete(const std::string &installDir) {
	static const char *TDM_EXECUTABLES[] = {
		//Windows executables
		"TheDarkMod.exe",
		"TheDarkModx64.exe",
		//Windows DLLs (2.06)
		"ExtLibs.dll",
		"ExtLibsx64.dll",
		//Linux executables
		"thedarkmod.x86",
		"thedarkmod.x64",
		//game DLLs (2.05 and before)
		"gamex86.dll",
		"gamex86.so"
	};
	//note: let's leave all the rest intact
	std::vector<std::string> res;
	auto allPaths = stdext::recursive_directory_enumerate(installDir);
	for (const auto &entry : allPaths) {
		if (stdext::is_regular_file(entry)) {
			std::string absPath = entry.string();
			std::string relPath = ZipSync::PathAR::FromAbs(absPath, installDir).rel;

			bool shouldDelete = false;
			for (const char *s : TDM_EXECUTABLES)
				if (relPath == s)
					shouldDelete = true;

			if (shouldDelete)
				res.push_back(absPath);
		}
	}
	return res;
}

static const char *ZIPS_TO_UNPACK[] = {"tdm_shared_stuff.zip"};
static int ZIPS_TO_UNPACK_NUM = sizeof(ZIPS_TO_UNPACK) / sizeof(ZIPS_TO_UNPACK[0]);

//=======================================================================================

void Actions::RestartWithInstallDir(const std::string &installDir) {
	g_logger->infof("Restarting TDM installer in directory: %s", installDir.c_str());

	if (!stdext::is_directory(installDir)) {
		g_logger->debugf("Creating missing directories for restart");
		if (!stdext::create_directories(installDir) || !stdext::is_directory(installDir))
			g_logger->errorf("Failed to create missing directories for restart");
	}

	std::string oldExePath = OsUtils::GetExecutablePath();
	std::string newExePath = (stdext::path(installDir) / OsUtils::GetExecutableName()).string();
	g_logger->debugf("Copying updater to new install directory: \"%s\" -> \"%s\"", oldExePath.c_str(), newExePath.c_str());
	if (stdext::is_regular_file(newExePath))
		stdext::remove(newExePath);
	stdext::copy_file(oldExePath, newExePath);

	OsUtils::ReplaceAndRestartExecutable(newExePath, "");
}

void Actions::StartLogFile() {
	//from now on, write logs to a logfile in CWD
	delete g_logger;
	g_logger = new LoggerTdm();
}

bool Actions::NeedsSelfUpdate(ZipSync::ProgressIndicator *progress) {
	std::string exeFilename = OsUtils::GetExecutableName();
	std::string exePath = OsUtils::GetExecutablePath();
	std::string exeTempPath = OsUtils::GetExecutablePath() + ".__temp__";
	std::string exeZipPath = OsUtils::GetExecutablePath() + ".__temp__.zip";
	std::string exeUrl = TDM_INSTALLER_EXECUTABLE_URL_PREFIX + OsUtils::GetExecutableName() + ".zip";

	ZipSync::HashDigest myHash;
	{
		g_logger->infof("Computing hash of myself at %s...", exePath.c_str());
		std::vector<uint8_t> selfExeData = ZipSync::ReadWholeFile(exePath);
		myHash = ZipSync::Hasher().Update(selfExeData.data(), selfExeData.size()).Finalize();
		g_logger->infof("My hash is %s", myHash.Hex().c_str());
	}

	//fast pass: download hash of updater
	g_logger->infof("Checking installer executable at %s...", exeUrl.c_str());
	ZipSync::Downloader downloaderPreliminary;
	if (progress)
		downloaderPreliminary.SetProgressCallback(progress->GetDownloaderCallback());
	ZipSync::HashDigest desiredHash = ZipSync::GetHashesOfRemoteChecksummedZips(downloaderPreliminary, {exeUrl})[0];
	g_logger->infof("Downloaded bytes: %lld", downloaderPreliminary.TotalBytesDownloaded());
	g_logger->infof("Hash of installer on server is %s", desiredHash.Hex().c_str());

	if (myHash == desiredHash) {
		g_logger->infof("Hashes match, update not needed");
		return false;
	}
	else {
		//second pass: download full manifests when necessary
		g_logger->infof("Downloading installer executable from %s...", exeUrl.c_str());
		ZipSync::Downloader downloaderFull;
		if (progress)
			downloaderFull.SetProgressCallback(progress->GetDownloaderCallback());
		std::vector<std::string> outPaths = {exeZipPath};
		ZipSync::DownloadChecksummedZips(downloaderFull, {exeUrl}, {desiredHash}, {}, outPaths);
		g_logger->infof("Downloaded bytes: %lld", downloaderFull.TotalBytesDownloaded());
		ZipSyncAssert(exeZipPath == outPaths[0]);

		g_logger->infof("Unpacking data from %s to temporary file %s", exeZipPath.c_str(), exeTempPath.c_str());
		std::vector<uint8_t> data = ZipSync::ReadChecksummedZip(exeZipPath.c_str(), exeFilename.c_str());
		{
			ZipSync::StdioFileHolder f(exeTempPath.c_str(), "wb");
			int wr = fwrite(data.data(), 1, data.size(), f);
			ZipSyncAssert(wr == data.size());
		}
		stdext::remove(exeZipPath);

		g_logger->infof("");
		return true;
	}
}
void Actions::DoSelfUpdate() {
	std::string exePath = OsUtils::GetExecutablePath();
	std::string exeTempPath = OsUtils::GetExecutablePath() + ".__temp__";
	//replace executable and rerun it
	g_logger->infof("Replacing and restarting myself...");
	OsUtils::ReplaceAndRestartExecutable(exePath, exeTempPath);
}


void Actions::ReadConfigFile(bool download, ZipSync::ProgressIndicator *progress) {
	g_state->_config.Clear();

	if (download) {
		g_logger->infof("Downloading config file from %s...", TDM_INSTALLER_CONFIG_URL);
		ZipSync::Downloader downloader;
		auto DataCallback = [](const void *data, int len) {
			ZipSync::StdioFileHolder f(TDM_INSTALLER_CONFIG_FILENAME, "wb");
			int res = fwrite(data, 1, len, f);
			ZipSyncAssert(res == len);
		};
		downloader.EnqueueDownload(ZipSync::DownloadSource(TDM_INSTALLER_CONFIG_URL), DataCallback);
		if (progress)
			downloader.SetProgressCallback(progress->GetDownloaderCallback());
		downloader.DownloadAll();
		g_logger->infof("Downloaded bytes: %lld", downloader.TotalBytesDownloaded());
	}

	g_logger->infof("Reading INI file %s", TDM_INSTALLER_CONFIG_FILENAME);
	//read the file (throws exception if not present)
	auto iniData = ZipSync::ReadIniFile(TDM_INSTALLER_CONFIG_FILENAME);
	//analyze and check it
	try {
		g_logger->infof("Loading installer config from it");
		g_state->_config.InitFromIni(iniData);
	}
	catch(...) {
		//make sure corrupted config does not remain after error
		g_state->_config.Clear();
		throw;
	}
	g_logger->infof("");
}

void Actions::ScanInstallDirectoryIfNecessary(bool force, ZipSync::ProgressIndicator *progress) {
	g_state->_localManifest.Clear();
	bool doScan = false;
	if (force) {
		g_logger->infof("Do scanning because forced by user");
		doScan = true;
	}
	else {
		try {
			g_logger->infof("Trying to read local %s", TDM_INSTALLER_LOCAL_MANIFEST);
			ZipSync::IniData ini = ZipSync::ReadIniFile(TDM_INSTALLER_LOCAL_MANIFEST);
			g_state->_localManifest.ReadFromIni(ini, OsUtils::GetCwd());
		}
		catch(const std::exception &) {
			g_logger->infof("Failed to read it, fallback to scanning");
			doScan = true;
			g_state->_localManifest.Clear();
		}
		g_logger->infof("Local manifest read successfully");
		//TODO: check validity?
	}
	if (!doScan) {
		g_logger->infof("");
		return;
	}

	std::string root = OsUtils::GetCwd();
	g_logger->infof("Cleaning temporary files");	//possibly remained from previous run
	ZipSync::DoClean(root);

	g_logger->infof("Collecting set of already existing TDM-owned zips");
	std::vector<std::string> managedZips = CollectTdmZipPaths(root);

	g_logger->infof("Installation currently contains of %d TDM-owned zips", managedZips.size());
	uint64_t totalSize = 0;
	for (const std::string &mzip : managedZips) {
		g_logger->infof("  %s", mzip.c_str());
		totalSize += ZipSync::SizeOfFile(mzip);
	}
	g_logger->infof("Total size of managed zips: %0.0lf MB", totalSize * 1e-6);

	g_logger->infof("Analysing the archives");
	ZipSync::Manifest manifest = ZipSync::DoAnalyze(root, managedZips, true, 1, progress);
	g_logger->infof("Saving results of analysis to manifest file");
	ZipSync::WriteIniFile((root + "/manifest.iniz").c_str(), manifest.WriteToIni());
	g_state->_localManifest = std::move(manifest);
	g_logger->infof("");
}

Actions::VersionInfo Actions::RefreshVersionInfo(const std::string &targetVersion, bool bitwiseExact, ZipSync::ProgressIndicator *progress) {
	g_logger->infof("Evaluating version %s", targetVersion.c_str());
	g_state->_updater.reset();

	std::vector<std::string> addProvidedVersions = g_state->_config.GetAdditionalProvidedVersions(targetVersion);
	std::string targetManifestUrl = g_state->_config.ChooseManifestUrl(targetVersion);
	std::vector<std::string> providManifestUrls;
	for (int i = 0; i < addProvidedVersions.size(); i++)
		providManifestUrls.push_back(g_state->_config.ChooseManifestUrl(addProvidedVersions[i]));

	g_logger->infof("Target manifest at %s", targetManifestUrl.c_str());
	g_logger->infof("Version %s needs files from %d other versions", targetVersion.c_str(), int(addProvidedVersions.size()));
	for (int i = 0; i < addProvidedVersions.size(); i++)
		g_logger->debugf("  %s at %s", addProvidedVersions[i].c_str(), providManifestUrls[i].c_str());

	//see which manifests were not loaded in this updater session
	std::vector<std::string> downloadedVersions;
	std::vector<std::string> downloadedManifestUrls;
	for (int i = -1; i < (int)addProvidedVersions.size(); i++) {
		std::string ver = (i < 0 ? targetVersion : addProvidedVersions[i]);
		std::string url = (i < 0 ? targetManifestUrl : providManifestUrls[i]);
		if (g_state->_loadedManifests.count(ver))
			continue;
		downloadedVersions.push_back(ver);
		downloadedManifestUrls.push_back(url);
	}

	g_logger->infof("Need to download %d manifests", (int)downloadedVersions.size());
	for (int i = 0; i < downloadedVersions.size(); i++)
		g_logger->infof("  %s at %s", downloadedVersions[i].c_str(), downloadedManifestUrls[i].c_str());


	if (int n = downloadedVersions.size()) {
		//inspect local cache of manifests
		std::string cacheDir = TDM_INSTALLER_ZIPSYNC_DIR "/" TDM_INSTALLER_MANICACHE_SUBDIR;
		stdext::create_directories(cacheDir);
		//detect existing manifests and names for new ones
		g_logger->infof("Looking into manifests cache");
		std::vector<std::string> cachedManiNames, newManiNames;
		for (int id = 0; id < 1000 || newManiNames.size() < n; id++) {
			std::string filename = cacheDir + "/" + std::to_string(id) + ".iniz";
			if (stdext::is_regular_file(filename))
				cachedManiNames.push_back(filename);
			else if (newManiNames.size() < n)
				newManiNames.push_back(filename);
		}
		g_logger->infof("Detected %d manifests in cache", (int)cachedManiNames.size());

		//fast pass: download hashes of all manifests
		g_logger->infof("Downloading hashes of remote manifests");
		std::vector<ZipSync::HashDigest> allHashes;
		ZipSync::Downloader downloaderPreliminary;
		if (progress)
			downloaderPreliminary.SetProgressCallback(progress->GetDownloaderCallback());
		allHashes = ZipSync::GetHashesOfRemoteChecksummedZips(downloaderPreliminary, downloadedManifestUrls);
		g_logger->infof("Downloaded bytes: %lld", downloaderPreliminary.TotalBytesDownloaded());

		g_logger->infof("Downloading actual remote manifests");
		for (int i = 0; i < n; i++)
			g_logger->infof("  (%s) %s -> %s", allHashes[i].Hex().c_str(), downloadedManifestUrls[i].c_str(), newManiNames[i].c_str());

		//second pass: download full manifests when necessary
		ZipSync::Downloader downloaderFull;
		if (progress)
			downloaderFull.SetProgressCallback(progress->GetDownloaderCallback());
		std::vector<int> matching = ZipSync::DownloadChecksummedZips(downloaderFull, downloadedManifestUrls, allHashes, cachedManiNames, newManiNames);
		g_logger->infof("Downloaded bytes: %lld", downloaderFull.TotalBytesDownloaded());

		g_logger->infof("Downloaded all manifests successfully");
		for (int i = 0; i < n; i++)
			g_logger->infof("  (%s) %s -> %s", allHashes[i].Hex().c_str(), downloadedManifestUrls[i].c_str(), newManiNames[i].c_str());

		g_logger->infof("Loading downloaded manifests");
		for (int i = 0; i < n; i++) {
			if (progress)
				progress->Update(double(i) / n, ZipSync::formatMessage("Loading manifest \"%s\"...", newManiNames[i].c_str()));
			ZipSync::IniData ini = ZipSync::ReadIniFile(newManiNames[i].c_str());
			ZipSync::Manifest mani;
			mani.ReadFromIni(std::move(ini), OsUtils::GetCwd());
			mani.ReRoot(ZipSync::GetDirPath(downloadedManifestUrls[i]));
			g_state->_loadedManifests[downloadedVersions[i]] = std::move(mani);
			if (progress)
				progress->Update(double(i+1) / n, ZipSync::formatMessage("Manifest \"%s\" loaded", newManiNames[i].c_str()));
		}
		if (progress)
			progress->Update(1.0, "Manifests loaded");
	}

	//look at locally present zips
	std::vector<std::string> ownedZips = CollectTdmZipPaths(OsUtils::GetCwd());
	g_logger->infof("There are %d TDM-owned zips locally", int(ownedZips.size()));
	for (int i = 0; i < ownedZips.size(); i++)
		g_logger->debugf("  %s", ownedZips[i].c_str());

	//gather full manifests for update
	ZipSync::Manifest targetMani = g_state->_loadedManifests[targetVersion];
	ZipSync::Manifest providMani = targetMani.Filter([](const ZipSync::FileMetainfo &mf) -> bool {
		return mf.location != ZipSync::FileLocation::Nowhere;
	});
	providMani.AppendManifest(g_state->_localManifest);
	for (const std::string &ver : addProvidedVersions) {
		const ZipSync::Manifest &mani = g_state->_loadedManifests[ver];
		ZipSync::Manifest added = mani.Filter([](const ZipSync::FileMetainfo &mf) -> bool {
			return mf.location != ZipSync::FileLocation::Nowhere;
		});
		providMani.AppendManifest(added);
	}
	g_logger->infof("Collected full manifest: target (%d files) and provided (%d files)", (int)targetMani.size(), (int)providMani.size());

	//develop update plan
	g_logger->infof("Developing update plan");
	std::unique_ptr<ZipSync::UpdateProcess> updater(new ZipSync::UpdateProcess());
	updater->Init(targetMani, providMani, OsUtils::GetCwd());
	for (const std::string &path : ownedZips)
		updater->AddManagedZip(path);
	auto updateType = (bitwiseExact ? ZipSync::UpdateType::SameCompressed : ZipSync::UpdateType::SameContents);
	updater->DevelopPlan(updateType);

	//compute stats
	Actions::VersionInfo info;
	for (int i = 0; i < ownedZips.size(); i++)
		info.currentSize += ZipSync::SizeOfFile(ownedZips[i]);
	for (int i = 0; i < updater->MatchCount(); i++) {
		auto m = updater->GetMatch(i);
		uint32_t sz = m.target->props.compressedSize;
		info.finalSize += sz;
		if (m.provided->location == ZipSync::FileLocation::RemoteHttp)
			info.downloadSize += sz;
		if (m.provided->location != ZipSync::FileLocation::Inplace)
			info.addedSize += sz;
	}
	uint64_t remainsSize = info.finalSize - info.addedSize;
	info.removedSize = info.currentSize - remainsSize;
	g_logger->infof("Update statistics:");
	g_logger->infof("  current: %llu", info.currentSize);
	g_logger->infof("  final: %llu", info.finalSize);
	g_logger->infof("  download: %llu", info.downloadSize);
	g_logger->infof("  added: %llu", info.addedSize);
	g_logger->infof("  removed: %llu", info.removedSize);

	//save updater --- perhaps it would be used to actually perform the update
	g_state->_updater = std::move(updater);
	g_logger->infof("");
	return info;
}

void Actions::PerformInstallDownload(ZipSync::ProgressIndicator *progress) {
	g_logger->infof("Starting installation: download");
	ZipSync::UpdateProcess *updater = g_state->_updater.get();
	ZipSyncAssert(updater);

	auto callback = (progress ? progress->GetDownloaderCallback() : ZipSync::GlobalProgressCallback());
	uint64_t totalBytesDownloaded = updater->DownloadRemoteFiles(callback);
	g_logger->infof("Downloaded bytes: %lld", totalBytesDownloaded);

	g_logger->infof("");
}

void Actions::PerformInstallRepack(ZipSync::ProgressIndicator *progress) {
	g_logger->infof("Starting installation: repack");
	ZipSync::UpdateProcess *updater = g_state->_updater.get();
	ZipSyncAssert(updater);

	updater->RepackZips(progress->GetDownloaderCallback());
	g_logger->infof("Repacking finished");

	g_logger->infof("");
}

static void UnpackZip(unzFile zf) {
	SAFE_CALL(unzGoToFirstFile(zf));
	while (1) {
		char currFilename[4096];
		SAFE_CALL(unzGetCurrentFileInfo(zf, NULL, currFilename, sizeof(currFilename), NULL, 0, NULL, 0));
		ZipSync::StdioFileHolder outfile(currFilename, "wb");

		SAFE_CALL(unzOpenCurrentFile(zf));
		char buffer[64<<10];
		while (1) {
			int read = unzReadCurrentFile(zf, buffer, sizeof(buffer));
			ZipSyncAssert(read >= 0);
			if (read == 0)
				break;
			int wr = fwrite(buffer, 1, read, outfile);
			ZipSyncAssert(read == wr);
		}
		SAFE_CALL(unzCloseCurrentFile(zf));

		int res = unzGoToNextFile(zf);
		if (res == UNZ_END_OF_LIST_OF_FILE)
			break;   //finished
		SAFE_CALL(res);
	}
}
void Actions::PerformInstallFinalize(ZipSync::ProgressIndicator *progress) {
	ZipSync::UpdateProcess *updater = g_state->_updater.get();
	ZipSyncAssert(updater);

	if (progress)
		progress->Update(0.0, "Saving resulting manifest...");
	ZipSync::Manifest mani = updater->GetProvidedManifest().Filter([](const ZipSync::FileMetainfo &mf) -> bool {
		return mf.location == ZipSync::FileLocation::Inplace;
	});
	ZipSync::WriteIniFile(TDM_INSTALLER_LOCAL_MANIFEST, mani.WriteToIni());
	if (progress)
		progress->Update(0.5, "Manifest saved");

	if (progress)
		progress->Update(0.5, "Cleaning temporary zips...");
	ZipSync::DoClean(OsUtils::GetCwd());
	if (progress)
		progress->Update(0.6, "Cleaning finished");

	if (progress)
		progress->Update(0.6, "Deleting old files...");
	std::vector<std::string> delFiles = CollectTdmUnpackedFilesToDelete(OsUtils::GetCwd());
	for (const std::string &fn : delFiles)
		stdext::remove(fn);
	if (progress)
		progress->Update(0.7, "Deleting finished");

	for (int i = 0; i < ZIPS_TO_UNPACK_NUM; i++) {
		const char *fn = ZIPS_TO_UNPACK[i];
		ZipSync::UnzFileHolder zf(unzOpen(fn));
		if (!zf)
			continue;
		if (progress)
			progress->Update(0.7 + 0.3 * (i+0)/ZIPS_TO_UNPACK_NUM, formatMessage("Unpacking %s...", fn).c_str());
		UnpackZip(zf);
		if (progress)
			progress->Update(1.0 + 0.3 * (i+1)/ZIPS_TO_UNPACK_NUM, "Unpacking finished");
	}

	g_logger->infof("");
}
