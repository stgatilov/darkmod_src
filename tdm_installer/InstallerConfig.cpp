#include "InstallerConfig.h"
#include <random>
#include <functional>
#include <set>
#include <time.h>
#include "LogUtils.h"
#include "StdString.h"
#include "Path.h"
#include "Constants.h"

void InstallerConfig::Clear() {
	_mirrorSets.clear();
	_versions.clear();
	_defaultVersion.clear();
}

void InstallerConfig::InitFromIni(const ZipSync::IniData &iniData) {
	Clear();

	//pass 1: read everything to our structures
	for (const auto &pNS : iniData) {
		const std::string &secHeader = pNS.first;
		const ZipSync::IniSect &secData = pNS.second;

		//split into section class and name
		int pos = (int)secHeader.find(' ');
		ZipSyncAssertF(pos > 0, "No space in INI section header: \"%s\"", secHeader.c_str());
		std::string secClass = secHeader.substr(0, pos);
		std::string secName = secHeader.substr(pos+1);

		if (secClass == "MirrorSet") {
			ZipSyncAssertF(_mirrorSets.count(secName) == 0, "MirrorSet %s: described in INI twice", secName.c_str());
			std::map<std::string, WeightedUrl> nameToMirror;
			for (const auto &pKV : secData) {
				const std::string &key = pKV.first;
				const std::string &value = pKV.second;
				if (stdext::starts_with(key, "url_"))
					nameToMirror[key.substr(4)]._url = value;
				else if (stdext::starts_with(key, "weight_"))
					nameToMirror[key.substr(7)]._weight = stod(value);
				else {
					ZipSyncAssertF(false, "MirrorSet %s: unexpected key \"%s\"", secName.c_str(), key.c_str());
				}
			}
			MirrorSet mirset;
			mirset._name = secName;
			mirset._ini = secData;
			for (const auto &pNM : nameToMirror) {
				const std::string &key = pNM.first;
				const WeightedUrl &mirror = pNM.second;
				ZipSyncAssertF(!mirror._url.empty(), "MirrorSet %s: url_%s not set or empty", secName.c_str(), key.c_str());
				ZipSyncAssertF(mirror._weight >= 0.0, "MirrorSet %s: weight_%s < 0 or not set", secName.c_str(), key.c_str());
				mirset._urls.push_back(mirror);
			}
			_mirrorSets[mirset._name] = std::move(mirset);
		}
		else if (secClass == "Version") {
			ZipSyncAssertF(_versions.count(secName) == 0, "Version %s: described in INI twice", secName.c_str());
			Version ver;
			ver._name = secName;
			ver._ini = secData;
			for (const auto &pKV : secData) {
				const std::string &key = pKV.first;
				const std::string &value = pKV.second;
				if (key == "folder") {
					ZipSyncAssertF(ver._folderPath.empty(), "Version %s: folder described twice", ver._name.c_str());
					stdext::split(ver._folderPath, value, "/");
				}
				else if (key == "default") {
					ZipSyncAssertF(_defaultVersion.empty(), "Two versions marked as default");
					_defaultVersion = ver._name;
				}
				else if (key == "manifestUrl" || stdext::starts_with(key, "manifestUrl_")) {
					WeightedUrl addWurl;
					addWurl._url = value;
					addWurl._weight = 1.0;
					ver._manifestUrls.push_back(addWurl);
				}
				else if (key == "depends" || stdext::starts_with(key, "depends_")) {
					ver._depends.push_back(value);
				}
				else {
					ZipSyncAssertF(false, "Version %s: unexpected key \"%s\"", ver._name.c_str(), key.c_str());
				}
			}
			ZipSyncAssertF(!ver._folderPath.empty(), "Version %s: folder not specified", ver._name.c_str());
			ZipSyncAssertF(!ver._manifestUrls.empty(), "Version %s: no manifestUrl-s", ver._name.c_str());
			_versions[ver._name] = std::move(ver);
		}
		else {
			ZipSyncAssertF(false, "Unknown INI section class \"%s\"", secClass.c_str());
		}
	}
	ZipSyncAssertF(!_defaultVersion.empty(), "No default version specified in INI");

	//pass 2: resolve manifestUrls and verify depends
	for (auto &pNV : _versions) {
		Version &ver = pNV.second;

		//process mirror sets, replace them with mirror URLs
		std::vector<WeightedUrl> newUrls;
		for (const WeightedUrl &wurl : ver._manifestUrls) {
			const std::string &url = wurl._url;
			//syntax: ${MS:MIRROR_SET_NAME}  --- allowed only at beginning
			if (stdext::starts_with(url, "${MS:")) {
				int pos = (int)url.find('}');
				ZipSyncAssertF(pos > 0, "Missing closing brace");
				std::string mirsetName = url.substr(5, pos-5);
				std::string tail = url.substr(pos + 1);
				ZipSyncAssertF(_mirrorSets.count(mirsetName), "Version %s: unknown MirrorSet %s", ver._name.c_str(), mirsetName.c_str());
				const auto &replacements = _mirrorSets.at(mirsetName)._urls;
				for (const WeightedUrl &repl : replacements) {
					WeightedUrl addWurl;
					addWurl._url = repl._url + tail;
					addWurl._weight = repl._weight * wurl._weight;
					newUrls.push_back(addWurl);
				}
			}
			else {
				newUrls.push_back(wurl);
			}
		}
		for (const WeightedUrl &wurl : newUrls) {
			ZipSyncAssertF(ZipSync::PathAR::IsHttp(wurl._url), "Version %s: manifest URL is not recognized as HTTP", ver._name.c_str());
		}
		ver._manifestUrls = std::move(newUrls);

		int trustedCnt = 0;
		for (const WeightedUrl &wurl : ver._manifestUrls) {
			if (IsUrlTrusted(wurl._url))
				trustedCnt++;
		}
		ZipSyncAssertF(trustedCnt > 0, "Version %s: has no manifest URL at trusted location", ver._name.c_str());

		for (const std::string &dep : ver._depends) {
			ZipSyncAssertF(_versions.count(dep), "Version %s: depends on missing Version %s", ver._name.c_str(), dep.c_str());
		}
	}

	//pass 3: compute provided manifests for every version, check for dependency cycles
	for (auto &pNV : _versions) {
		const std::string &name = pNV.first;
		Version &ver = pNV.second;

		std::vector<std::string> providedVersions;
		std::set<std::string> inRecursion;
		std::function<void(const std::string &)> TraverseDependencies = [&](const std::string &version) -> void {
			ZipSyncAssertF(inRecursion.count(version) == 0, "Version %s: at dependency cycle", version.c_str());
			inRecursion.insert(version);

			if (std::count(providedVersions.begin(), providedVersions.end(), version))
				return;
			providedVersions.push_back(version);

			const auto &depends = _versions.at(version)._depends;
			for (const std::string &dep : depends) {
				TraverseDependencies(dep);
			}

			inRecursion.erase(version);
		};
		TraverseDependencies(name);

		ver._providedVersions = std::move(providedVersions);
	}
}

std::vector<std::string> InstallerConfig::GetAllVersions() const {
	std::vector<std::string> res;
	for (const auto &pNV : _versions)
		res.push_back(pNV.first);
	return res;
}

std::vector<std::string> InstallerConfig::GetFolderPath(const std::string &version) const {
	return _versions.at(version)._folderPath;
}

std::string InstallerConfig::GetDefaultVersion() const {
	ZipSyncAssertF(!_defaultVersion.empty(), "Cannot return default version: %s was not read", TDM_INSTALLER_CONFIG_FILENAME);
	return _defaultVersion;
}

bool InstallerConfig::IsUrlTrusted(const std::string &url) const {
	return stdext::starts_with(url, TDM_INSTALLER_TRUSTED_URL_PREFIX);
}

std::string InstallerConfig::ChooseManifestUrl(const std::string &version, bool trusted) const {
	static std::mt19937 MirrorChoosingRandom((int)time(0) ^ clock());	//RNG here!!!

	if (ZipSync::PathAR::IsHttp(version))
		return version;	//this is custom manifest, not a version

	const Version &ver = _versions.at(version);
	std::vector<WeightedUrl> candidates;
	if (trusted) {
		for (const WeightedUrl &wurl : ver._manifestUrls)
			if (IsUrlTrusted(wurl._url))
				candidates.push_back(wurl);
	}
	else {
		candidates = ver._manifestUrls;
	}

	double sum = 0.0;
	for (const WeightedUrl &wurl : candidates)
		sum += wurl._weight;
	sum = std::max(sum, 1e-6);
	ZipSyncAssertF(candidates.size(), "No candidates to choose url from (version %s)", version.c_str());

	double param = (MirrorChoosingRandom() % 1000000) * 1e-6 * sum;
	const WeightedUrl *chosen = &candidates[0];
	for (const WeightedUrl &wurl : candidates) {
		param -= wurl._weight;
		if (param < 0.0) {
			chosen = &wurl;
			break;
		}
	}
	return chosen->_url;
}

std::vector<std::string> InstallerConfig::GetProvidedVersions(const std::string &version) const {
	const Version &ver = _versions.at(version);
	return ver._providedVersions;
}

void InstallerConfig::RemoveFailingUrl(const std::string &badUrl) {
	int cntRemoved = 0;

	for (auto &pNV : _versions) {
		const std::string &ver = pNV.first;
		Version &version = pNV.second;

		std::vector<WeightedUrl> remainUrls;
		for (WeightedUrl maniUrl : version._manifestUrls) {
			if (maniUrl._url == badUrl) {
				g_logger->warningf("Removed manifest URL %s from version %s", maniUrl._url.c_str(), ver.c_str());
				cntRemoved++;
			}
			else
				remainUrls.push_back(maniUrl);
		}
		version._manifestUrls = std::move(remainUrls);
	}

	ZipSyncAssertF(cntRemoved > 0, "Cannot handle nonworking URL %s", badUrl.c_str());
}
