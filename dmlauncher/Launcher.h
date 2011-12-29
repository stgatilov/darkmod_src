/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

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

