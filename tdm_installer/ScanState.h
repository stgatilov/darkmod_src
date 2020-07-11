#pragma once

#include <string>
#include <vector>
#include "Ini.h"

//stores size and timestamp of every managed zip file
//it is saved in TDM_INSTALLER_LASTSCAN_PATH in order to:
//  1) avoid rescanning all zips on every launch
//  2) show "last installer version" in GUI
class ScanState {
	struct ZipState {
		std::string name;
		uint64_t timestamp = 0;
		uint32_t size = 0;

		bool operator<(const ZipState &b) const;
		bool operator==(const ZipState &b) const;
	};

	std::string _version;
	std::vector<ZipState> _zips;

public:
	static ScanState ScanZipSet(const std::vector<std::string> &zipPaths, std::string rootDir, std::string version = "");

	static ScanState ReadFromIni(const ZipSync::IniData &ini);
	ZipSync::IniData WriteToIni() const;

	bool NotChangedSince(const ScanState &cleanState) const;

	const std::string &GetVersion() const { return _version; }
};
