/*
Darkmod Launcher. Launches doom3. Builds command-line args
from currentfm.txt and dmargs.txt.
If the command line that invokes this executable is not blank, then
pause 2 seconds before spawing doom3.
*/

#include "Launcher.h"

int main(int argc, char* argv[])
{
	// Instantiate a new Launcher class
	Launcher launcher(argc, argv);

	return launcher.Launch() ? EXIT_SUCCESS : EXIT_FAILURE;
}
