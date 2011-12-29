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

#include "Launcher.h"

#include <limits.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/lexical_cast.hpp>

#include "D3ProcessChecker.h"
#include "TraceLog.h"

const std::string CURRENT_FM_FILE = "currentfm.txt";
const std::string ARGS_FILE = "dmargs.txt";
const std::string GAME_BASE_NAME = "darkmod";

const std::string STEAM_ARGS = "-applaunch 9050";

#ifdef WIN32
	#include <windows.h>
	#include <process.h>

	#define ENGINE_EXECUTABLE "DOOM3.exe"
	#define MODULE_NAME "gamex86.dll"
	#define STEAM_PATH_NAME "steamapps"
	#define STEAM_EXE_PATH "../steam.exe"

// The ShellAPI.h header is polluting our namespace
#ifdef FindExecutable  
#undef FindExecutable
#endif

#elif __linux__
	// Linux
	#include <unistd.h>
	#include <errno.h>

	#define ENGINE_EXECUTABLE "doom.x86"
#elif MACOS_X
	#include <unistd.h>
	#include <errno.h>
	#include <mach-o/dyld.h>

	#define ENGINE_EXECUTABLE "Doom 3"

// Platform-specific method to expand the ~ in a path with a full path
// adjusted from http://developer.apple.com/library/mac/#qa/qa2007/qa1549.html

#include <glob.h>

std::string GetExpandedTildePath(const char* path)
{
	assert(path != NULL); // don't accept bogus input

	std::string result = path;

	// Check if the path starts with a tilde 
	if (!result.empty() > 0 && result[0] == '~')
	{
		// Got a tilde at the front, replace that with the home folder using glob()
		glob_t globbuf;

		if (glob("~", GLOB_TILDE, NULL, &globbuf) == 0)
		{
			char** v = globbuf.gl_pathv; // list of matched pathnames
	        
			// Replace the tilde
			result = result.substr(1);
			result = v[0] + result;

			globfree(&globbuf);
		}
	}

	return result;
}

#else
	#error 'Unsupported platform.'
#endif

Launcher::Launcher(int argc, char* argv[]) :
	_additionalDelay(0)
{
#ifdef WIN32
	// path to this exe
	_dmLauncher = argv[0];

	if (!_dmLauncher.has_branch_path())
	{
		TraceLog::WriteLine("No path defined in argv[0], will prepend current directory.");
		_dmLauncher = boost::filesystem::initial_path() / _dmLauncher;
	}

#elif defined(__linux__)
	char exepath[PATH_MAX] = {0};
	std::size_t bytesRead = readlink("/proc/self/exe", exepath, sizeof(exepath));

	_dmLauncher = exepath;
#elif defined (MACOS_X)
	char exepath[4096];
	uint32_t size = sizeof(exepath);
	
	if (_NSGetExecutablePath(exepath, &size) != 0)
	{
		TraceLog::WriteLine("Cannot read executable path, buffer too small.");
	}
	
	_dmLauncher = exepath;
#else
#error Unsupported Platform
#endif
	
	TraceLog::WriteLine("Path to tdmlauncher is " + _dmLauncher.file_string());

	// path to the darkmod directory
	_darkmodDir = _dmLauncher.remove_leaf();

	TraceLog::WriteLine("Darkmod directory is " + _darkmodDir.file_string());

	// Try to remove ../ and ./ references from darkmoddir
	_darkmodDir = NormalisePath(_darkmodDir);

	TraceLog::WriteLine("Darkmod directory after normalisation is " + _darkmodDir.file_string());

	// Find the engine executable
	if (!FindExecutable())
	{
		TraceLog::WriteLine("Could not find engine executable at default locations, will be searching command line arguments for a path to doom3 executable.");
	}

	_engineExecutable = NormalisePath(_engineExecutable);

	TraceLog::WriteLine("Engine path after normalisation is " + _engineExecutable.file_string());
	
	fs::path argFileName(_darkmodDir / ARGS_FILE);

	// Inspect the arguments
	for (int i = 1; i < argc; ++i)
	{
		fs::path possibleExecutable = argv[i];

		if (possibleExecutable.string().find(ENGINE_EXECUTABLE) != std::string::npos)
		{
			// We've found an argument which might fit for an executable, check if it exists
			if (fs::exists(possibleExecutable) && !fs::is_directory(possibleExecutable))
			{
				// Got it, use this as engine executable
				TraceLog::WriteLine("Reading engine executable from command line arguments: " + possibleExecutable.file_string());

				_engineExecutable = possibleExecutable;
				continue; // don't use this as argument file
			}
		}

		// Check if this is an optional dmargs file
		fs::path optionalArgsFileName(_darkmodDir / argv[i]);

		// Tels: fix #2803: check that both source and destination are regular files
		if (fs::exists(optionalArgsFileName) && fs::is_regular_file(optionalArgsFileName) && fs::is_regular_file(argFileName))
		{
			if (fs::exists(argFileName))
			{
				TraceLog::WriteLine("Removing default args file: " + argFileName.file_string());
				fs::remove(argFileName);
			}

			TraceLog::WriteLine("Using custom args file: " + optionalArgsFileName.file_string());

			fs::copy_file(optionalArgsFileName, argFileName);
		}

		std::string delayStr = argv[i];
		if (delayStr.find("--delay=") != std::string::npos)
		{
			TraceLog::WriteLine("Additional delay before starting D3: " + delayStr.substr(9) + " milliseconds.");

			try
			{
				_additionalDelay = boost::lexical_cast<std::size_t>(delayStr.substr(9));
			}
			catch (boost::bad_lexical_cast&)
			{
				TraceLog::WriteLine("Invalid delay string, reset delay to 0: " + delayStr.substr(9));
				_additionalDelay = 0;
			}

			// Avoid ridicolous delays longer than one minute
			if (_additionalDelay > 60000)
			{
				TraceLog::WriteLine("Capping delay to 60 seconds.");
				_additionalDelay = 60000;
			}
		}
	}
}

// Detect Steam version of Doom 3 and find the associated exe file
fs::path Launcher::FindSteamExecutable()
{
#ifdef WIN32
	fs::path partialPath;
	for (fs::path::iterator itr = _engineExecutable.begin(); itr != _engineExecutable.end(); ++itr)
	{
		partialPath /= *itr;
		if (boost::algorithm::iequals(*itr, STEAM_PATH_NAME))
		{
			return partialPath / STEAM_EXE_PATH;
		}
	}
#endif
	return fs::path();
}

void Launcher::InitArguments()
{
	if (!_currentFM.empty())
	{
		// greebo: issue #2904 suggests the argument order is important, put fs_game first
		AddArgument("+set");
		AddArgument("fs_game");
		AddArgument(_currentFM);

		AddArgument("+set");
		AddArgument("fs_game_base");
		AddArgument(GAME_BASE_NAME);
	}
	else
	{
		// No current FM, pass darkmod as fs_game instead of fs_game_base
		AddArgument("+set");
		AddArgument("fs_game");
		AddArgument(GAME_BASE_NAME);
	}

	// optional file that contains custom doom3 command line args
	fs::path argFileName(_darkmodDir / ARGS_FILE);
	
	FILE* argFile = fopen(argFileName.file_string().c_str(), "r");

	if (argFile != NULL)
	{
		char buffer[3001];

		// read command line args from file
		do
		{
			// Try to read a few bytes
			if (fgets(buffer, 3000, argFile) == NULL)
			{
				break;
			}

			// Ignore all lines starting with #
			if (buffer[0] != '#')
			{
				std::vector<std::string> parts;
				boost::algorithm::split(parts, buffer, boost::algorithm::is_any_of(" \t"));
				
				for (std::size_t i = 0; i < parts.size(); ++i)
				{
					AddArgument(parts[i]);
				}
			}
		}
		while (buffer[0] == '#');

		fclose(argFile);
	}

	// Check for a leading "+" sign
	if (!_arguments.empty() && !boost::algorithm::starts_with(_arguments[0], "+"))
	{
		TraceLog::WriteLine("Adding a '+' sign to the front of raw argument string: " + _arguments[0]);
		_arguments[0] = "+" + _arguments[0];
	}
}

void Launcher::InitCurrentFM()
{
	// file that contains name of the current FM directory
	fs::path currentFMFileName(_darkmodDir / CURRENT_FM_FILE);

	if (!fs::exists(currentFMFileName))
	{
		TraceLog::WriteLine("Could not find 'currentfm.txt' file in " + currentFMFileName.string());
	}

	// get the current FM
	_currentFM = ReadFile(currentFMFileName);

	TraceLog::WriteLine("Current FM is: " + _currentFM);

	// Check if the FM folder actually exists
	fs::path modFolder = _darkmodDir;
	modFolder = modFolder.remove_leaf().remove_leaf() / _currentFM;

	if (!fs::exists(modFolder))
	{
		TraceLog::WriteLine("Cannot find Mod folder for FM: " + _currentFM);
		TraceLog::WriteLine("Current FM reset to empty again.");
		_currentFM.clear();
	}
}

std::string Launcher::ReadFile(const fs::path& fileName)
{
	std::string returnValue;

	FILE* file = fopen(fileName.file_string().c_str(), "r");

	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);
		std::size_t len = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* buf = reinterpret_cast<char*>(malloc(len+1));

		std::size_t bytesRead = fread(buf, 1, len, file);

		if (bytesRead != len)
		{
			std::cerr << "Warning: bytes read mismatches file length?" << std::endl;
		}		

		buf[len] = 0;

		returnValue = buf;
		free(buf);

		fclose(file);
	}

	return returnValue;
}

fs::path Launcher::NormalisePath(const fs::path& p)
{
	if (!p.has_root_path() || !p.is_complete()) 
	{
		return p;
	}

	fs::path result;

	for (fs::path::iterator it = p.begin(); it != p.end(); ++it)
	{
		if (*it == "..")
		{
			// /a/b/.. is not necessarily /a if b is a symbolic link
			if (fs::is_symlink(result))
			{
				result /= *it;
			}
			// /a/b/../.. is not /a/b/.. under most circumstances
			// We can end up with ..s in our result because of symbolic links
			else if (result.filename() == "..")
			{
				result /= *it;
			}
			// Otherwise it should be safe to resolve the parent
			else
			{
				result = result.parent_path();
			}
		}
		else if (*it == ".")
		{
			// Ignore
		}
		else
		{
			// Just cat other path entries
			result /= *it;
		}
	}

	return result;
}


bool Launcher::FindExecutable()
{
	// Try to look in the parent directory of the darkmod folder (this definitely applies to Win32)
	_engineExecutable = _darkmodDir;
	_engineExecutable = _engineExecutable.remove_leaf().remove_leaf() / ENGINE_EXECUTABLE;

	TraceLog::WriteLine("Trying default value for engine executable: " + _engineExecutable.file_string());
	
	if (fs::exists(_engineExecutable))
	{
		// Found engine executable
		TraceLog::WriteLine("Found engine executable in " + _engineExecutable.file_string());
		return true;
	}

	// No executable found, start guessing...
	
#ifdef __linux__

	// Default value not found, in Linux, the engine is usually in /usr/local/games/doom3
	_engineExecutable = "/usr/local/games/doom3/";
	_engineExecutable /= ENGINE_EXECUTABLE;
	
	TraceLog::WriteLine("Trying default Linux location for engine executable " + _engineExecutable.file_string());
	
	if (fs::exists(_engineExecutable))
	{
		// Found engine executable
		TraceLog::WriteLine("Found engine executable in " + _engineExecutable.file_string());
		return true;
	}

#elif MACOS_X

	// In OSX, try to find the executable relative to our parent directory
	_engineExecutable = _darkmodDir;
	_engineExecutable /= "../Doom 3.app/Contents/MacOS";
	_engineExecutable /= ENGINE_EXECUTABLE;

	TraceLog::WriteLine("Trying to find engine executable in " + _engineExecutable.file_string());

	if (fs::exists(_engineExecutable))
	{
		// Found engine executable
		TraceLog::WriteLine("Found engine executable in " + _engineExecutable.file_string());
		return true;
	}

	// Still not found, try finding the engine below the user's Desktop folder
	_engineExecutable = GetExpandedTildePath("~/Desktop/Doom 3/Doom3.app/Contents/MacOS/Doom 3");

	TraceLog::WriteLine("Trying to find engine executable in " + _engineExecutable.file_string());

	if (fs::exists(_engineExecutable))
	{
		// Found engine executable
		TraceLog::WriteLine("Found engine executable in " + _engineExecutable.file_string());
		return true;
	}
	
#endif
	
	// not found!
	return false;
}

#ifdef WIN32

// Windows implementation
bool Launcher::Launch()
{
	// Get the name of the current FM
	InitCurrentFM();

	// Initialise the arguments
	InitArguments();

	// Check for a D3 process (max. 10 seconds)
	int timeout = 10000;
	while (D3ProcessChecker::D3IsRunning(ENGINE_EXECUTABLE, MODULE_NAME) && timeout >= 0)
	{
		TraceLog::WriteLine("Doom 3 is still running, waiting one second...");
		Sleep(1000);
		timeout -= 1000;
	}

	if (_additionalDelay > 0)
	{
		TraceLog::WriteLine("Sleeping for another " + boost::lexical_cast<std::string>(_additionalDelay) + " milliseconds.");
		Sleep(static_cast<DWORD>(_additionalDelay));
	}

	// path to doom3.exe
	fs::path doom3exe = _engineExecutable;
	fs::path doom3dir = doom3exe;
	doom3dir = doom3dir.remove_leaf().remove_leaf();

	// Check if we're running steam
	fs::path steamExe(FindSteamExecutable());

	if (!steamExe.empty())
	{
		TraceLog::WriteLine("Detected Steam version of D3; Steam executable found at " + steamExe.file_string());

		doom3exe = steamExe;

		std::vector<std::string> parts;
		boost::algorithm::split(parts, STEAM_ARGS, boost::algorithm::is_any_of(" \t"));

		// Pre-pend the steam argument list
		for (std::vector<std::string>::reverse_iterator i = parts.rbegin(); i != parts.rend(); ++i)
		{
			AddArgument(*i, true);
		}
	}

	// Debug output the argument string
	TraceLog::WriteLine("Using the following argument vector:");

	for (std::size_t i = 0; i < _arguments.size(); ++i)
	{
		TraceLog::WriteLine("#" + boost::lexical_cast<std::string>(i) + ": " + _arguments[i]);
	}

	// Assemble the argument string
	std::string argStr = GetArgumentString();

	TraceLog::WriteLine("Starting process " + doom3exe.file_string() + " " + argStr);
	
	::SetCurrentDirectory(doom3dir.file_string().c_str());

	if (_spawnl(_P_NOWAIT, doom3exe.file_string().c_str(), doom3exe.file_string().c_str(), argStr.c_str(), NULL) == -1)
	{
		TraceLog::WriteLine(std::string("Error spawning the D3 process: ") + strerror(errno));
	}

	return true;
}

#else

// Linux doesn't know Sleep(), add a substitute def
#include <unistd.h>
#define Sleep(x) usleep(static_cast<int>(1000 * (x)))

// Linux implementation
bool Launcher::Launch()
{
	// Get the name of the current FM
	InitCurrentFM();

	// Initialise the arguments
	InitArguments();

	// Check for a D3 process (max. 10 seconds)
	int timeout = 10000;
	
	// Don't pass a module name in Linux/MacOSX, this is not necessary
	while (D3ProcessChecker::D3IsRunning(ENGINE_EXECUTABLE, "") && timeout >= 0)
	{
		TraceLog::WriteLine("Doom 3 is still running, waiting one second...");
		Sleep(1000);
		timeout -= 1000;
	}

	if (_additionalDelay > 0)
	{
		TraceLog::WriteLine("Sleeping for another " + boost::lexical_cast<std::string>(_additionalDelay) + " milliseconds.");
		Sleep(static_cast<int>(_additionalDelay));
	}

	// path to doom3.exe
	fs::path doom3app(_engineExecutable);
	fs::path doom3dir = doom3app;
	doom3dir = doom3dir.remove_leaf().remove_leaf();

	// Start a new process
	pid_t child_pid = fork();
	
	if (child_pid == 0)
	{
		// Add the doom3 app path as first argument
		AddArgument(doom3app.file_string(), true);

		TraceLog::WriteLine("Using the following argument vector:");

		for (std::size_t i = 0; i < _arguments.size(); ++i)
		{
			TraceLog::WriteLine("#" + boost::lexical_cast<std::string>(i) + ": " + _arguments[i]);
		}

		TraceLog::WriteLine("Trying to launch " + _engineExecutable.file_string() + " " + GetArgumentString());

		// Instantiate the char* array needed for execvp (need one more for the trailing NULL)
		char* argv[_arguments.size() + 1];

		for (std::size_t i = 0; i < _arguments.size(); ++i)
		{
			argv[i] = new char[_arguments[i].length() + 1];
			strcpy(argv[i], _arguments[i].c_str());
		}

		// The last argument points to NULL
		argv[_arguments.size()] = NULL;

		int result = execvp(doom3app.file_string().c_str(), argv);

		if (result == -1)
		{
			std::cerr << "Error while launching D3 executable: " << strerror(errno) << std::endl;
		}

		// Free the char* array again
		for (std::size_t i = 0; i < _arguments.size(); ++i)
		{
			delete[] argv[i];
		}

		exit(0);
	}

	return true;
}

#endif

void Launcher::AddArgument(const std::string& arg, bool insertAtFront)
{
	std::string argument = arg;

	// Replace all tabs and newlines with spaces
	boost::algorithm::replace_all(argument, "\t", " ");
	boost::algorithm::replace_all(argument, "\n", " ");

	// Trim head and tail
	boost::algorithm::trim(argument);
	
	// Check if the string needs sanitising
	if (argument.find(' ') != std::string::npos)
	{
		// Escape any double-quoates in the argument itself before surrounding it
		boost::algorithm::replace_all(argument, "\"", "\\\"");

		// surroung argument with double quotes
		argument = "\"" + argument + "\"";
	}

	// Add the argument
	_arguments.insert(insertAtFront ? _arguments.begin() : _arguments.end(), argument);
}

std::string Launcher::GetArgumentString()
{
	std::string argStr;

	// The strings are already sanitised, just concatenate everything into one string
	for (std::vector<std::string>::const_iterator i = _arguments.begin(); i != _arguments.end(); ++i)
	{
		argStr += !argStr.empty() ? " " : "";
		argStr += *i;
	}

	return argStr;
}
