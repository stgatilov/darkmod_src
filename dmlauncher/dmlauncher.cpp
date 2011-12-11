/*************************************************************************
 *
 * PROJECT: The Dark Mod - Launcher
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 *************************************************************************/

/*
Darkmod Launcher. Launches doom3. Builds command-line args
from currentfm.txt and dmargs.txt.
*/
#include "Launcher.h"

int main(int argc, char* argv[])
{
	// Instantiate a new Launcher class
	Launcher launcher(argc, argv);

	return launcher.Launch() ? EXIT_SUCCESS : EXIT_FAILURE;
}
