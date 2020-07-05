#pragma once

#include <string>

namespace ZipSync {
	class ProgressIndicator;
}

class Actions {
public:
	//called when user clicks "Restart" button with custom install dir
	static void RestartWithInstallDir(const std::string &installDir);

	//called when we are sure user won't change install dir any more
	static void StartLogFile();

	//read g_config from file in install dir
	//if download = true, then the file is downloaded from TDM server first
	static void ReadConfigFile(bool download);

	//called when user clicks "Next" button on settings page
	//generates manifest in the install directory
	static void ScanInstallDirectoryIfNecessary(bool force, ZipSync::ProgressIndicator *progress);

	struct VersionInfo {
		uint64_t currentSize = 0;
		uint64_t finalSize = 0;
		uint64_t addedSize = 0;
		uint64_t removedSize = 0;
		uint64_t downloadSize = 0;
	};
	//user wants to know stats about possible update to specified version
	//this action can trigger downloading manifests (note: they are cached in g_state)
	static VersionInfo RefreshVersionInfo(const std::string &version, bool bitwiseExact, ZipSync::ProgressIndicator *progress);

	//perform prepared update: download all data
	static void PerformInstallDownload(ZipSync::ProgressIndicator *progress);

	//perform prepared update: repack installation
	static void PerformInstallRepack(ZipSync::ProgressIndicator *progress);
};
