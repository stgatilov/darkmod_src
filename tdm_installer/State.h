#pragma once

#include "InstallerConfig.h"
#include "StoredState.h"
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
	//this is read from TDM_INSTALLER_LASTINSTALL_PATH, includes:
	//  version --- for display only
	//  owned set of files --- to be removed during update
	InstallState _lastInstall;
	//set of versions for which manifest has already been loaded
	std::map<std::string, ZipSync::Manifest> _loadedManifests;
	//which version was last evaluated with "refresh" button
	//if custom url was specified, then it is appended to based version (with " & " separator)
	std::string _versionRefreshed;
	//the update which is going to be made (or is made right now)
	//if present, then it is prepared to update to _versionRefreshed
	//action RefreshVersionInfo stores it here if plan is successfully developed
	std::unique_ptr<ZipSync::UpdateProcess> _updater;
	//name of the renamed darkmod.cfg file
	//useful if user decides to restore it
	std::string _oldConfigFilename;
	//name of preferred mirror (or empty if auto)
	std::string _preferredMirror;

	void Reset();
	~State();
};

//global state of the updater
extern State *g_state;
