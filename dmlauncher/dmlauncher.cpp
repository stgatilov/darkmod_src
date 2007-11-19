// dmlauncher.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <windows.h>
#include "boost/filesystem.hpp"
/*
Darkmod Launcher. Launches doom3 from command line arguments; used when
swapping missions. Also deletes the .pk4 file from the "old" mission.
Args:
 - doom3 exe to launch
 - name of file that contains command line arguments to doom3 exe
 - pk4 file to delete (optional)
*/
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	namespace fs = boost::filesystem;
	char exe[200];
	char argFileName[200];
	char darkModPathName[200];
	char modDirPathName[200];
	char buffer[200];
	sscanf(lpCmdLine, "%s%s%s%s", &exe, &argFileName, &darkModPathName, &modDirPathName);
	Sleep(2000);

	// Path to the darkmod directory
	fs::path darkmodPath(darkModPathName);

	// Path to the new mod directory
	fs::path modDirPath(modDirPathName);

	// Name of the file that contains the list of pk4s for the current mod
	fs::path fmpk4list(darkmodPath / "fmpk4s.txt");

	// Read the list of pk4s in the current mod, delete them
	FILE* fmpk4file = fopen(fmpk4list.file_string().c_str(), "r");
	if (fmpk4file) {
		// read pk4s to delete from file
		while (fgets(buffer, 200, fmpk4file) != NULL) {
			size_t nLen = strlen(buffer);
			if (buffer[nLen-1] == '\n' ) {
				// strip newline
				buffer[--nLen] = 0;
			}
			fs::path pk4ToDelete(darkmodPath / buffer);
			fs::remove(pk4ToDelete);
		}
		fclose(fmpk4file);
	}

	// Copy the pk4s from the new mod
	fs::directory_iterator end_iter;
	fmpk4file = fopen(fmpk4list.file_string().c_str(), "w+");
	for (fs::directory_iterator dir_itr(modDirPath); dir_itr != end_iter; ++dir_itr)
	{
		if (!fs::is_directory(dir_itr->status()))
		{
			size_t pos = strlen(dir_itr->path().leaf().c_str()) - 4;
			int comp = strcmp(&(dir_itr->path().leaf().c_str()[pos]), ".pk4");
			if (comp == 0)
			{
				fs::path dest(darkmodPath / dir_itr->path().leaf());
				fs::copy_file(dir_itr->path(), dest);
				fputs(dir_itr->path().leaf().c_str(), fmpk4file);
				fputs("\n", fmpk4file);
			}
		}
	}
	fclose(fmpk4file);

	buffer[0] = 0;
	FILE* argFile = fopen(argFileName, "r");
	if (argFile) {
		// read command line args from file
		do {
			if (fgets(buffer, 200, argFile) == NULL) {
				break;
			}
		} while (buffer[0] == '#');
		fclose(argFile);
	} else {
		// default args
		strcpy(buffer, "+set fs_game darkmod");
	}

	intptr_t x = _spawnl(_P_NOWAIT, exe, exe, buffer, NULL);
	return 0;
}



