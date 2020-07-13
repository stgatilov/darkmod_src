#pragma once

#include <string>
#include <vector>

namespace ZipSync {
	class ProgressIndicator;
}

class Actions {
public:
	//called when user clicks "Restart" button with custom install dir
	static void RestartWithInstallDir(const std::string &installDir);

	//checks that we can write files at installation directory, and for free space
	//errors are thrown as exceptions, but warnings are returned as array of strings
	static std::vector<std::string> CheckSpaceAndPermissions(const std::string &installDir);

	//called when we are sure user won't change install dir any more
	static void StartLogFile();

	//check if central server offers different executable
	static bool NeedsSelfUpdate(ZipSync::ProgressIndicator *progress);
	//update and restart installer
	//must be called immediately after NeedsSelfUpdate returns true
	static void DoSelfUpdate();

	//read g_config from file in install dir
	//if download = true, then the file is downloaded from TDM server first
	static void ReadConfigFile(bool download, ZipSync::ProgressIndicator *progress);

	//called when user clicks "Next" button on settings page
	//generates manifest in the install directory
	static void ScanInstallDirectoryIfNecessary(bool force, ZipSync::ProgressIndicator *progress);

	struct VersionInfo {
		uint64_t currentSize = 0;
		uint64_t finalSize = 0;
		uint64_t addedSize = 0;
		uint64_t removedSize = 0;
		uint64_t downloadSize = 0;
		uint64_t missingSize = 0;		//only possible with custom manifest
	};
	//user wants to know stats about possible update to specified version
	//this action can trigger downloading manifests (note: they are cached in g_state)
	//if customManifestUrl is nonempty, then it overrides target manifest location
	static VersionInfo RefreshVersionInfo(const std::string &version, const std::string &customManifestUrl, bool bitwiseExact, ZipSync::ProgressIndicator *progress);

	//perform prepared update: download all data
	static void PerformInstallDownload(ZipSync::ProgressIndicator *progress);

	//perform prepared update: repack installation
	static void PerformInstallRepack(ZipSync::ProgressIndicator *progress);

	//finalize update (cleanup, manifest, unpacking, etc.)
	static void PerformInstallFinalize(ZipSync::ProgressIndicator *progress);
};
