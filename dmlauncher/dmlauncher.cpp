// dmlauncher.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <windows.h>

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
	char exe[100];
	char pk4ToDelete[100];
	char argFileName[100];
	char args[100];
	sscanf(lpCmdLine, "%s%s%s%", &exe, &argFileName, &pk4ToDelete);
	Sleep(2000);
	remove(pk4ToDelete);
	FILE* argFile = fopen(argFileName, "r");
	if (argFile) {
		// read command line args from file
		fgets(args, 100, argFile);
		fclose(argFile);
	} else {
		// default args
		strcpy(args, "+set fs_game darkmod");
	}

	intptr_t x = _spawnl(_P_NOWAIT, exe, exe, args, NULL);
	return 0;
}



