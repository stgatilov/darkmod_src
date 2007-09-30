// dmlauncher.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <windows.h>

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	char arg1[100];
	char arg2[100];
	char arg3[100];
	char arg4[100];
	Sleep(1000);
	sscanf(lpCmdLine, "%s%s%s%s%s", &arg1, &arg2, &arg3, &arg4);
	intptr_t x = _spawnl(_P_NOWAIT, arg1, arg1, arg2, arg3, arg4, NULL);
	return 0;
}



