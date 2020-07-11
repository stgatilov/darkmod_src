#include "ScanState.h"
#include "StdFilesystem.h"
#include "StdString.h"
#include "Path.h"
#include <algorithm>
#include "LogUtils.h"

bool ScanState::ZipState::operator<(const ZipState &b) const {
	return name < b.name;
}
bool ScanState::ZipState::operator==(const ZipState &b) const {
	return name == b.name && timestamp == b.timestamp && size == b.size;
}

ScanState ScanState::ScanZipSet(const std::vector<std::string> &zipPaths, std::string rootDir, std::string version) {
	ScanState res;
	res._version = version;
	for (const std::string &absPath : zipPaths) {
		std::string relPath = ZipSync::PathAR::FromAbs(absPath, rootDir).rel;
		ZipState zs;
		zs.name = relPath;
		zs.size = stdext::file_size(absPath);
		zs.timestamp = stdext::last_write_time(absPath);
		res._zips.push_back(zs);
	}
	std::sort(res._zips.begin(), res._zips.end());
	return res;
}

ScanState ScanState::ReadFromIni(const ZipSync::IniData &ini) {
	ScanState res;

	for (const auto &pNS : ini) {
		std::string secname = pNS.first;
		ZipSync::IniSect sec = pNS.second;

		if (secname == "Version") {
			ZipSyncAssert(sec[0].first == "version");
			res._version = sec[0].second;
		}
		else if (stdext::starts_with(secname, "Zip ")) {
			ZipState zs;
			zs.name = secname.substr(4);
			ZipSyncAssert(sec[0].first == "modTime");
			long long unsigned temp;
			int k = sscanf(sec[0].second.c_str(), "%llu", &temp);
			ZipSyncAssert(k == 1);
			zs.timestamp = temp;
			ZipSyncAssert(sec[1].first == "size");
			k = sscanf(sec[1].second.c_str(), "%u", &zs.size);
			ZipSyncAssert(k == 1);
			res._zips.push_back(zs);
		}
		else {
			ZipSyncAssert(false);
		}
	}

	return res;
}

ZipSync::IniData ScanState::WriteToIni() const {
	ZipSync::IniData ini;

	ZipSync::IniSect sec = {std::make_pair("version", _version)};
	ini.emplace_back("Version", sec);

	for (const ZipState &zs : _zips) {
		std::string secname = "Zip " + zs.name;
		ZipSync::IniSect sec;
		sec.emplace_back("modTime", std::to_string(zs.timestamp));
		sec.emplace_back("size", std::to_string(zs.size));
		ini.emplace_back(secname, sec);
	}

	return ini;
}

bool ScanState::NotChangedSince(const ScanState &cleanState) const {
	//note: both must already be sorted
	return _zips == cleanState._zips;
}