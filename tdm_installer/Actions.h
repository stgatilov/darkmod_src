#pragma once

#include <string>

namespace ZipSync {
	class ProgressIndicator;
}

class Actions {
public:
	//called when user clicks "Restart" button with custom install dir
	static void RestartWithInstallDir(const std::string &installDir);

	//called when user clicks "Next" button on settings page
	//generates manifest in the install directory
	static void ScanInstallDirectoryIfNecessary(bool force, ZipSync::ProgressIndicator *progress);
};
