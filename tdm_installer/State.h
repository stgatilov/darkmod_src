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
	//set of versions for which manifest has already been loaded
	std::map<std::string, ZipSync::Manifest> _loadedManifests;
	//the update which is going to be made (or is made right now)
	//action RefreshVersionInfo stores it here if plan is successfully developed
	std::unique_ptr<ZipSync::UpdateProcess> _updater;

	~State();
};

//global state of the updater
extern State *g_state;
