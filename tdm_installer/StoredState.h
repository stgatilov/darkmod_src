#pragma once

#include <string>
#include <vector>
#include "Ini.h"

//stores size and timestamp of every managed zip file
//it is saved in TDM_INSTALLER_LASTSCAN_PATH in order to
//avoid rescanning all zips on every launch
class ScanState {
	struct ZipState {
		std::string name;
		uint64_t timestamp = 0;
		uint32_t size = 0;

		bool operator<(const ZipState &b) const;
		bool operator==(const ZipState &b) const;
	};

	std::vector<ZipState> _zips;

public:
	static ScanState ScanZipSet(const std::vector<std::string> &zipPaths, std::string rootDir);

	static ScanState ReadFromIni(const ZipSync::IniData &ini);
	ZipSync::IniData WriteToIni() const;

	bool NotChangedSince(const ScanState &cleanState) const;
};

//stores last installed version and set of "owned" files
//it is saved in TDM_INSTALLER_LASTINSTALL_PATH in order to
//  1) show "last installer version" in GUI
//  2) remove all installed files when updating to different version
class InstallState {
	std::string _version;
	std::vector<std::string> _ownedZips;
	std::vector<std::string> _ownedUnpacked;

public:
	InstallState();
	InstallState(const std::string &version, const std::vector<std::string> &ownedZips, const std::vector<std::string> &ownedUnpacked);

	const std::string &GetVersion() const { return _version; }
	const std::vector<std::string> &GetOwnedZips() const { return _ownedZips; }
	const std::vector<std::string> &GetOwnedUnpackedFiles() const { return _ownedUnpacked; }

	static InstallState ReadFromIni(const ZipSync::IniData &ini);
	ZipSync::IniData WriteToIni() const;
};
