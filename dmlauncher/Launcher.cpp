#include "Launcher.h"

#include <limits.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "D3ProcessChecker.h"
#include "TraceLog.h"

const std::string CURRENT_FM_FILE = "currentfm.txt";
const std::string ARGS_FILE = "dmargs.txt";
const std::string GAME_BASE_NAME = "darkmod";

#ifdef WIN32
	#include <windows.h>
	#include <process.h>

	#define ENGINE_EXECUTABLE "DOOM3.exe"
	#define MODULE_NAME "gamex86.dll"
#else 
	// Linux
	#include <unistd.h>
	#include <errno.h>

	#define ENGINE_EXECUTABLE "doom.x86"
#endif

Launcher::Launcher(int argc, char* argv[]) :
	_pauseBeforeStart(true)
{
#ifdef WIN32
	// path to this exe
	boost::filesystem::path dmlauncher(argv[0]);
#else
	char exepath[PATH_MAX] = {0};
	std::size_t bytesRead = readlink("/proc/self/exe", exepath, sizeof(exepath));

	boost::filesystem::path dmlauncher(exepath);
#endif
	
	TraceLog::WriteLine("Path to tdmlauncher is " + dmlauncher.file_string());

	// path to the darkmod directory
	_darkmodDir = dmlauncher.remove_leaf();

	TraceLog::WriteLine("Darkmod directory is " + _darkmodDir.file_string());

	// Default value
	_engineExecutable = _darkmodDir;
	_engineExecutable = _engineExecutable.remove_leaf().remove_leaf() / ENGINE_EXECUTABLE;

	TraceLog::WriteLine("Default value for engine executable is " + _engineExecutable.file_string());

	fs::path argFileName(_darkmodDir / ARGS_FILE);

	// Number of arguments to ignore (one is this executable itself, ignore it)
	int numIgnoreArgs = 1;

	// Inspect the arguments
	for (int i = 1; i < argc; ++i)
	{
		fs::path possibleExecutable = argv[i];

		if (possibleExecutable.string().find(ENGINE_EXECUTABLE) != std::string::npos)
		{
			// We've found an argument which might fit for an executable, check if it exists
			if (fs::exists(possibleExecutable))
			{
				// Got it, use this as engine executable
				TraceLog::WriteLine("Reading engine executable from command line arguments: " + possibleExecutable.file_string());

				_engineExecutable = possibleExecutable;
				numIgnoreArgs++;
				continue; // don't use this as argument file
			}
		}

		// Check if this is an optional dmargs file
		fs::path optionalArgsFileName(_darkmodDir / argv[i]);

		if (fs::exists(optionalArgsFileName))
		{
			if (fs::exists(argFileName))
			{
				TraceLog::WriteLine("Removing default args file: " + argFileName.file_string());
				fs::remove(argFileName);
			}

			TraceLog::WriteLine("Using custom args file: " + optionalArgsFileName.file_string());

			fs::copy_file(optionalArgsFileName, argFileName);

			numIgnoreArgs++;
		}
	}

	if (argc - numIgnoreArgs == 0)
	{
		_pauseBeforeStart = false; // don't wait if no arguments supplied
	}
}

void Launcher::InitArguments()
{
	_arguments = " +set fs_game_base " + GAME_BASE_NAME + " ";

	if (!_currentFM.empty())
	{
		_arguments.append(" +set fs_game " + _currentFM + " ");
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
				_arguments.append(" ");
				_arguments.append(buffer);
			}
		}
		while (buffer[0] == '#');

		fclose(argFile);
	}

	boost::algorithm::trim(_arguments);

	// Check for a leading "+" sign
	if (!_arguments.empty() && _arguments[0] != '+')
	{
		TraceLog::WriteLine("Adding a '+' sign to the front of raw argument string: " + _arguments);
		_arguments = "+" + _arguments;
	}

	TraceLog::WriteLine("Full argument string is: " + _arguments);
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

	// path to doom3.exe
	fs::path doom3exe = _engineExecutable;
	fs::path doom3dir = doom3exe;
	doom3dir = doom3dir.remove_leaf().remove_leaf();

	TraceLog::WriteLine("Starting process " + doom3exe.file_string() + " " + _arguments);
	
	::SetCurrentDirectory(doom3dir.file_string().c_str());
	if (_spawnl(_P_NOWAIT, doom3exe.file_string().c_str(), doom3exe.file_string().c_str(), _arguments.c_str(), NULL) == -1)
	{
		TraceLog::WriteLine(std::string("Error when spawning D3 process: ") + strerror(errno));
	}

	return true;
}

#else

// Linux implementation
bool Launcher::Launch()
{
	// Get the name of the current FM
	InitCurrentFM();

	// Initialise the arguments
	InitArguments();

	if (_pauseBeforeStart)
	{
		// Wait 2 seconds
		usleep(2000000);
	}

	std::cout << "Trying to launch " << _engineExecutable.file_string() << " " << _arguments.c_str() << std::endl;

	// path to doom3.exe
	fs::path doom3app(_engineExecutable);
	fs::path doom3dir = doom3app;
	doom3dir = doom3dir.remove_leaf().remove_leaf();

	// Start a new process
	pid_t child_pid = fork();
	
	if (child_pid == 0)
	{
		// Add the doom3 app path as first argument
		_arguments = doom3app.file_string() + " " + _arguments;

		// Remove any double or triple whitespace
		boost::algorithm::replace_all(_arguments, "   ", " ");
		boost::algorithm::replace_all(_arguments, "  ", " ");

		// Split the argument string into parts
		std::vector<std::string> parts;
		boost::algorithm::split(parts, _arguments, boost::algorithm::is_any_of(" "));

		// Instantiate the char* array needed for execvp (need one more for the trailing NULL)
		char* argv[parts.size() + 1];

		for (std::size_t i = 0; i < parts.size(); ++i)
		{
			// greebo: Sanitise the strings by trimming any whitespace from them
			boost::algorithm::trim(parts[i]);

			argv[i] = new char[parts[i].length() + 1];
			strcpy(argv[i], parts[i].c_str());
		}

		// The last argument points to NULL
		argv[parts.size()] = NULL;

		int result = execvp(doom3app.file_string().c_str(), argv);

		if (result == -1)
		{
			std::cerr << "Error while launching D3 executable: " << strerror(errno) << std::endl;
		}

		// Free the char* array again
		for (std::size_t i = 0; i < parts.size(); ++i)
		{
			delete[] argv[i];
		}

		exit(0);
	}

	return true;
}

#endif

