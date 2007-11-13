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
	char exe[200];
	char pk4ToDelete[200];
	char argFileName[200];
	char args[200];
	sscanf(lpCmdLine, "%s%s%s%", &exe, &argFileName, &pk4ToDelete);
	Sleep(2000);
	remove(pk4ToDelete);
	args[0] = 0;
	FILE* argFile = fopen(argFileName, "r");
	if (argFile) {
		// read command line args from file
		do {
			if (fgets(args, 200, argFile) == NULL) {
				break;
			}
		} while (args[0] == '#');
		fclose(argFile);
	} else {
		// default args
		strcpy(args, "+set fs_game darkmod");
	}

	intptr_t x = _spawnl(_P_NOWAIT, exe, exe, args, NULL);
	return 0;
}



