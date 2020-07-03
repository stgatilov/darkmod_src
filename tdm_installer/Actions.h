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
};
