#include "Actions.h"
#include "StdFilesystem.h"
#include "StdString.h"
#include "LogUtils.h"
#include "OsUtils.h"
#include "CommandLine.h"

//TODO: move to better place?
static std::vector<std::string> CollectTdmZipPaths(const std::string &installDir) {
	std::vector<std::string> res;
	auto allPaths = stdext::recursive_directory_enumerate(installDir);
	for (const auto &entry : allPaths) {
		if (stdext::is_regular_file(entry)) {
            std::string absPath = entry.string();
			std::string relPath = ZipSync::PathAR::FromAbs(absPath, installDir).rel;
			bool managed = false;

			//common caregories:
			if (stdext::istarts_with(relPath, "tdm_") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. tdm_ai_base01.pk4
			if (stdext::istarts_with(relPath, "tdm_") && stdext::iends_with(relPath, ".zip"))
				managed = true;		//e.g. tdm_shared_stuff.zip
			if (stdext::istarts_with(relPath, "fms/tdm_") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/tdm_training_mission/tdm_training_mission.pk4

			//hardcoded prepackaged FMs:
			if (stdext::istarts_with(relPath, "fms/newjob/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/newjob/newjob.pk4
			if (stdext::istarts_with(relPath, "fms/stlucia/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/stlucia/stlucia.pk4
			if (stdext::istarts_with(relPath, "fms/saintlucia/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/saintlucia/saintlucia.pk4
			if (stdext::istarts_with(relPath, "fms/training_mission/") && stdext::iends_with(relPath, ".pk4"))
				managed = true;		//e.g. fms/training_mission/training_mission.pk4

			if (managed)
				res.push_back(absPath);
		}
	}
	return res;
}

void Actions::RestartWithInstallDir(const std::string &installDir) {
	g_logger->infof("Restarting TDM installer in directory: %s", installDir.c_str());

	if (!stdext::is_directory(installDir)) {
		g_logger->debugf("Creating missing directories for restart");
		if (!stdext::create_directories(installDir) || !stdext::is_directory(installDir))
			g_logger->errorf("Failed to create missing directories for restart");
	}

	std::string oldExePath = OsUtils::GetExecutablePath();
	std::string newExePath = (stdext::path(installDir) / OsUtils::GetExecutableName()).string();
	g_logger->debugf("Copying updater to new install directory: \"%s\" -> \"%s\"", oldExePath.c_str(), newExePath.c_str());
	if (stdext::is_regular_file(newExePath))
		stdext::remove(newExePath);
	stdext::copy_file(oldExePath, newExePath);

	OsUtils::ReplaceAndRestartExecutable(newExePath, "");
}

void Actions::ScanInstallDirectoryIfNecessary(bool force, ZipSync::ProgressIndicator *progress) {
	//from now on, write logs to a logfile in CWD
	delete g_logger;
	g_logger = new LoggerTdm();

	bool doScan;
	if (force) {
		g_logger->debugf("Do scanning because forced by user");
		doScan = true;
	}
	else {
		//TODO: detect when scanning is required
		g_logger->debugf("No scanning (TODO)");
		doScan = false;
	}
	if (!doScan)
		return;

	std::string root = OsUtils::GetCwd();
	//remote temporary files --- possibly remained from previous run
	ZipSync::DoClean(root);
	//find metainformation for all TDM stuff
	std::vector<std::string> managedZips = CollectTdmZipPaths(root);
	ZipSync::Manifest manifest = ZipSync::DoAnalyze(root, managedZips, true, 1, progress);
	ZipSync::WriteIniFile((root + "/manifest.iniz").c_str(), manifest.WriteToIni());
}
