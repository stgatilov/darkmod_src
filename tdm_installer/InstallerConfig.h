#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include "Ini.h"

//describes information read from file TDM_INSTALLER_CONFIG_FILENAME
class InstallerConfig {
	struct MirrorSet {
		std::vector<std::string> _urls;
		std::string _name;
		ZipSync::IniSect _ini;
	};
	struct Version {
		std::vector<std::string> _manifestUrls;
		std::vector<std::string> _depends;
		std::vector<std::string> _folderPath;
		std::set<std::string> _providedVersions;		//transitive closure by _depends (itself excluded)
		std::string _name;
		ZipSync::IniSect _ini;
	};
	std::map<std::string, MirrorSet> _mirrorSets;
	std::map<std::string, Version> _versions;
	std::string _defaultVersion;

public:
	void Clear();

	//creates InstallerConfig object by reading specified ini file
	void InitFromIni(const ZipSync::IniData &iniData);

	//returns sequence of all known versions
	std::vector<std::string> GetAllVersions() const;

	//returns GUI tree pathname for specified version
	//consists of sequence of path elements, excluding the version element at the end
	std::vector<std::string> GetFolderPath(const std::string &version) const;

	//default version is installed if user has not chosen any specific version
	std::string GetDefaultVersion() const;

	//returns url to manifest file of the version
	//if there are several possibilities, then random one is chosen
	std::string ChooseManifestUrl(const std::string &version) const;

	//returns a set of additional provided URLs that the specified version needs
	//it tales every version in _providedVersions, and chooses its manifest by ChooseManifestUrl
	std::vector<std::string> ChooseAdditionalProvidedManifests(const std::string &version) const;
};

//global instance of config
extern InstallerConfig *g_config;
