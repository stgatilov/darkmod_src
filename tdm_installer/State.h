#pragma once

#include "InstallerConfig.h"
#include <map>
#include "Manifest.h"

namespace ZipSync {
	class UpdateProcess;
};

struct State {
	//contents of TDM_INSTALLER_CONFIG_FILENAME
	InstallerConfig _config;
	//describes local state of the installation dir
	ZipSync::Manifest _localManifest;
	//this is read from TDM_INSTALLER_LASTSCAN_PATH (for display only)
	std::string _lastInstalledVersion;
	//set of versions for which manifest has already been loaded
	std::map<std::string, ZipSync::Manifest> _loadedManifests;
	//which version was last evaluated with "refresh" button
	std::string _versionRefreshed;
	//the update which is going to be made (or is made right now)
	//if present, then it is prepared to update to _versionRefreshed
	//action RefreshVersionInfo stores it here if plan is successfully developed
	std::unique_ptr<ZipSync::UpdateProcess> _updater;

	void Reset();
	~State();
};

//global state of the updater
extern State *g_state;
