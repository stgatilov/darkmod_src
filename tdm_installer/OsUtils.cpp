#include "OsUtils.h"
#include "StdFilesystem.h"
#include "Utils.h"
#include "StdString.h"
#include "LogUtils.h"

std::string OsUtils::_argv0;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void ThrowWinApiError() {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);
	std::string message = (LPCTSTR)lpMsgBuf;
	LocalFree(lpMsgBuf);
	g_logger->errorf("(WinAPI error) %s", message.c_str());
}
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


void OsUtils::InitArgs(const char *argv0) {
	g_logger->debugf("Save command line of current process: \"%s\"", argv0);
	_argv0 = argv0;
}
std::string OsUtils::GetExecutablePath() {
	ZipSyncAssert(!_argv0.empty());
	auto exePath = stdext::canonical(_argv0);
	return exePath.string();
}
std::string OsUtils::GetExecutableDir() {
	ZipSyncAssert(!_argv0.empty());
	auto exeDir = stdext::canonical(_argv0).parent_path();
	return exeDir.string();
}
std::string OsUtils::GetExecutableName() {
	ZipSyncAssert(!_argv0.empty());
	auto exeFn = stdext::path(_argv0).filename();
	return exeFn.string();
}


std::string OsUtils::GetCwd() {
	return stdext::current_path().string();
}

bool OsUtils::SetCwd(const std::string &newCwd) {
	g_logger->debugf("Setting CWD to \"%s\"", newCwd.c_str());
	if (!stdext::is_directory(newCwd)) {
		g_logger->warningf("Suggested CWD is not an existing directory");
		return false;
	}
	stdext::current_path(newCwd);
	auto cwd = stdext::current_path();
	if (cwd != newCwd) {
		g_logger->warningf("Failed to reset CWD");
		return false;
	}
	return true;
}

void OsUtils::MarkAsExecutable(const std::string &filePath) {
#ifndef _WIN32
	g_logger->debugf("Marking \"%s\" as executable", filePath.c_str());
	struct stat mask;
	stat(filePath.c_str(), &mask);
	mask.st_mode |= S_IXUSR|S_IXGRP|S_IXOTH;
	if (chmod(filePath.c_str(), mask.st_mode) == -1)
		g_logger->errorf("Could not mark file as executable: %s", filePath.c_str());
#endif
}


void OsUtils::ReplaceAndRestartExecutable(const std::string &targetPath, const std::string &temporaryPath, const std::vector<std::string> &cmdArgs) {
	std::string allArgs = stdext::join(cmdArgs, " ");
	std::string batchFilePath = targetPath + "_temp.cmd";
	g_logger->infof("Restarting executable \"%s\" from \"%s\" (arguments: %s)",
		targetPath.c_str(), temporaryPath.c_str(), allArgs.c_str()
	);

	{
		g_logger->infof("Creating updating batch/shell file \"%s\"", batchFilePath.c_str());
		ZipSync::StdioFileHolder batchFile(batchFilePath.c_str(), "wt");
#ifdef _WIN32
		fprintf(batchFile, "@ping 127.0.0.1 -n 6 -w 1000 > nul\n"); // # hack equivalent to Wait 5
		if (!temporaryPath.empty()) {
			fprintf(batchFile, "@copy %s %s >nul\n", temporaryPath.c_str(), targetPath.c_str());
			fprintf(batchFile, "@del %s\n", temporaryPath.c_str());
			fprintf(batchFile, "@echo Executable has been replaced.\n");
		}
		fprintf(batchFile, "@echo Re-launching executable.\n\n");
		fprintf(batchFile, "@start %s %s\n", targetPath.c_str(), allArgs.c_str());
#else //POSIX
		fprintf(batchFile, "#!/bin/bash\n");
		fprintf(batchFile, "sleep 5s\n");
		if (!temporaryPath.empty()) {
			fprintf(batchFile, "mv -f \"%s\" \"%s\"\n", temporaryPath.c_str(), targetPath.c_str());
			fprintf(batchFile, "chmod +x \"%s\"\n", targetPath.c_str());
			fprintf(batchFile, "echo \"Executable has been updated.\"\n");
		}
		fprintf(batchFile, "echo \"Re-launching executable.\"\n");
		fprintf(batchFile, "\"%s\" %s", targetPath.c_str(), allArgs.c_str());
#endif
	}

	//mark the shell script as executable in *nix
	MarkAsExecutable(batchFilePath);

	{
		g_logger->infof("Running updating script");
#ifdef _WIN32
		STARTUPINFO siStartupInfo = {0};
		PROCESS_INFORMATION piProcessInfo = {0};
		siStartupInfo.cb = sizeof(siStartupInfo);
		//start new process
		BOOL success = CreateProcess(
			NULL, (LPSTR)batchFilePath.c_str(), NULL, NULL, false, 0, NULL,
			NULL, &siStartupInfo, &piProcessInfo
		);
		if (!success)
			ThrowWinApiError();
#else //POSIX	
		//perform the system command in a fork
		int code = fork();
		if (code == -1)
			g_logger->errorf("Failed to fork process");
		if (code == 0) {
			//don't wait for the subprocess to finish
			system((batchFilePath + " &").c_str());
			exit(0);
		}
#endif
		g_logger->infof("Updating script started successfully");
	}

	//terminate this process
	exit(0);
}
