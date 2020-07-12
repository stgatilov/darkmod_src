#pragma once

#include <string>
#include <vector>

class OsUtils {
	static std::string _argv0;
public:

	//pass information about command line
	//must be called into order to use ResetCwd and GetExecutableName
	static void InitArgs(const char *argv0);

	//returns absolute path to current executable
	//it is extracted from argv[0], set by InitArgs
	static std::string GetExecutablePath();
	//returns directory where current executable is located
	//it is extracted from argv[0], set by InitArgs
	static std::string GetExecutableDir();
	//returns filename of the current executable
	//it is extracted from argv[0], set by InitArgs
	static std::string GetExecutableName();

	//change working directory to the location of the launched executable
	//location is taken from argv[0], set by InitArgs
	static bool SetCwd(const std::string &newCwd);
	//returns current working directory
	static std::string GetCwd();

	//chmod file to be executable (NOP on Windows)
	static void MarkAsExecutable(const std::string &filePath);

	//used for self-update of the installer
	//runs batch/shell script which:
	//  1) deletes targetPath file
	//  2) renames temporaryPath to targetPath
	//  3) starts targetPath as executable
	//note: if temporaryPath is empty, then steps 1 and 2 are omitted
	static void ReplaceAndRestartExecutable(const std::string &targetPath, const std::string &temporaryPath, const std::vector<std::string> &cmdArgs = {});
	//returns name of batch file (for deleting it)
	static std::string GetBatchForReplaceAndRestartExecutable(const std::string &targetPath);

	//return number of bytes available at path
	static uint64_t GetAvailableDiskSpace(const std::string &path);
};
