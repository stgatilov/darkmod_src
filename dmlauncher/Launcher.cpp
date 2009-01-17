#include "Launcher.h"

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

	#define ENGINE_EXECUTABLE "doom3"
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
		long len = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* buf = reinterpret_cast<char*>(malloc(len+1));

		fread(buf, len, 1, file);
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
	fs::path doom3dir(_darkmodDir / "..");
	fs::path doom3exe(_darkmodDir / ".." / ENGINE_EXECUTABLE);

	return true;
}

#endif
