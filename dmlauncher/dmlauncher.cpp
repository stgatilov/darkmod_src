/*
Darkmod Launcher (Windows only). Launches doom3. Builds command-line args
from currentfm.txt and dmargs.txt.
If the command line that invokes this executable is not blank, then
pause 2 seconds before spawing doom3.
*/

#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <windows.h>
#include "boost/filesystem.hpp"

namespace fs = boost::filesystem;

char * readFile(fs::path fileName)
{
	FILE* file = fopen(fileName.file_string().c_str(), "r");
	char * buf = NULL;
	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);
		long len = ftell(file);
		fseek(file, 0, SEEK_SET);
		buf = (char *)malloc(len+1);
		fread(buf, len, 1, file);
		buf[len] = 0;
		fclose(file);
	}
	return buf;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	char exename[1001];
	char buffer[1001];
	char doomArgs[1001];
	char extraArgs[1001];

	// Get the path for this executable
	GetModuleFileName(hInstance, exename, 1000);

	// pause 2 seconds
	Sleep(2000);

	// path to this exe
	fs::path dmlauncher(exename);

	// path to the darkmod directory
	fs::path darkmodDir = dmlauncher.remove_leaf();

	// optional file that contains custom doom3 command line args
	fs::path argFileName(darkmodDir / "dmargs.txt");

	if (strlen(lpCmdLine)) {
		fs::path optionalArgsFileName(darkmodDir / lpCmdLine);
		if (exists(optionalArgsFileName))
		{
			if (exists(argFileName))
			{
				remove(argFileName);
			}
			copy_file(optionalArgsFileName, argFileName);
		}
	}

	// file that contains name of the current FM directory
	fs::path currentFMName(darkmodDir / "currentfm.txt");

	// path to doom3.exe
	fs::path doom3dir(darkmodDir / "..");
	fs::path doom3exe(darkmodDir / ".." / "doom3.exe");

	// get the current FM
	char * current = readFile(currentFMName);

	doomArgs[0] = 0;
	extraArgs[0] = 0;
	FILE* argFile = fopen(argFileName.file_string().c_str(), "r");
	if (argFile) {
		// read command line args from file
		do {
			if (fgets(buffer, 1000, argFile) == NULL) {
				break;
			}
			if (buffer[0] != '#') {
				strcpy(extraArgs, buffer);
			}
		} while (buffer[0] == '#');
		fclose(argFile);
	}

	// build command line to doom3
	strcpy(doomArgs, "+set fs_game_base darkmod ");
	if (current != NULL) {
		strcat(doomArgs, "+set fs_game ");
		strcat(doomArgs, current);
		strcat(doomArgs, " ");
	}
	strcat(doomArgs, extraArgs);
	::SetCurrentDirectory(doom3dir.file_string().c_str());
	_spawnl(_P_NOWAIT, doom3exe.file_string().c_str(), doom3exe.file_string().c_str(), doomArgs, NULL);
	return 0;
}



