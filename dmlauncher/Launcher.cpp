#include "Launcher.h"

#include <iostream>
#include <vector>
#include <cstring>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>

const std::string CURRENT_FM_FILE = "currentfm.txt";
const std::string ARGS_FILE = "dmargs.txt";
const std::string GAME_BASE_NAME = "darkmod";

#ifdef WIN32
	#include <windows.h>
	#include <process.h>

	#define ENGINE_EXECUTABLE "DOOM3.exe"
#else 
	// Linux
	#include <unistd.h>

	#define ENGINE_EXECUTABLE "doom.x86"
#endif

Launcher::Launcher(int argc, char* argv[])
{
	// path to this exe
	boost::filesystem::path dmlauncher(argv[0]);

	// path to the darkmod directory
	_darkmodDir = dmlauncher.remove_leaf();

	fs::path argFileName(_darkmodDir / ARGS_FILE);

	for (int i = 1; i < argc; ++i)
	{
		fs::path optionalArgsFileName(_darkmodDir / argv[i]);

		if (fs::exists(optionalArgsFileName))
		{
			if (fs::exists(argFileName))
			{
				fs::remove(argFileName);
			}

			fs::copy_file(optionalArgsFileName, argFileName);
		}
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
}

void Launcher::InitCurrentFM()
{
	// file that contains name of the current FM directory
	fs::path currentFMFileName(_darkmodDir / CURRENT_FM_FILE);

	// get the current FM
	_currentFM = ReadFile(currentFMFileName);
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

	Sleep(2000);

	// path to doom3.exe
	fs::path doom3dir(_darkmodDir / "..");
	fs::path doom3exe(_darkmodDir / ".." / ENGINE_EXECUTABLE);

	::SetCurrentDirectory(doom3dir.file_string().c_str());
	_spawnl(_P_NOWAIT, doom3exe.file_string().c_str(), doom3exe.file_string().c_str(), _arguments.c_str(), NULL);

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

	// Wait 2 seconds
	usleep(2000000);

	// path to doom3.exe
	fs::path doom3dir("/usr/local/games/doom3/");
	fs::path doom3app(doom3dir / ENGINE_EXECUTABLE);

	// Start a new process
	pid_t child_pid = fork();
	
	if (child_pid == 0)
	{
		// Add the doom3 app path as first argument
		_arguments = doom3app.string() + " " + _arguments;

		// Split the argument string into parts
		std::vector<std::string> parts;
		boost::algorithm::split(parts, _arguments, boost::algorithm::is_any_of(" "));

		// Instantiate the char* array needed for execvp (need one more for the trailing NULL)
		char* argv[parts.size() + 1];

		for (std::size_t i = 0; i < parts.size(); ++i)
		{
			argv[i] = new char[parts[i].length() + 1];
			strcpy(argv[i], parts[i].c_str());
		}

		// The last argument points to NULL
		argv[parts.size()] = NULL;

		int result = execvp(doom3app.string().c_str(), argv);

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

