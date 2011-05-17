/*************************************************************************
 *
 * PROJECT: The Dark Mod - Launcher
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 *************************************************************************/

#ifndef _LAUNCHER_H_
#define _LAUNCHER_H_

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

/**
 * greebo: Service class with platform-specific implementation
 * to launch the TDM game using the correct arguments.
 */
class Launcher
{
private:
	// The path to this launcher
	fs::path _dmLauncher;

	// The path to the darkmod game base
	fs::path _darkmodDir;

	// The path to the engine executable (DOOM3.exe / doom.x86)
	fs::path _engineExecutable;

	// The name of the current FM
	std::string _currentFM;

	// The arguments to pass to the engine
	std::vector<std::string> _arguments;

	std::size_t _additionalDelay;

public:
	// Pass the command line to this class
	Launcher(int argc, char* argv[]);

	// Launches the game, reading the settings and arguments from the DarkmodDir
	bool Launch();

private:
	// Reads the saved command line arguments
	void InitArguments();

	// Loads the name of the current FM
	void InitCurrentFM();

	// Reads the given text file and returns its contents
	std::string ReadFile(const fs::path& fileName);

	// Finds the engine executable (in Linux, this can be in several places), returns TRUE on success
	// The member _darkmodDir must be set already when calling this method
	bool FindExecutable();

	// Returns the path to Steam.exe if the engine executable we're using is running on Steam
	// The member _engineExecutable must be set already when calling this method
	fs::path FindSteamExecutable();

	// Tries to remove as many ../ and ./ from the given path as possible. Only works for absolute input paths
	static fs::path NormalisePath(const fs::path& p);

	// Adds an argument, automatically trims and sanitises the given string
	void AddArgument(const std::string& arg, bool insertAtFront = false);

	std::string GetArgumentString();
};

#endif /* _LAUNCHER_H_ */

