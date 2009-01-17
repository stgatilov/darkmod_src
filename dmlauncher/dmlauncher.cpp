/*
Darkmod Launcher. Launches doom3. Builds command-line args
from currentfm.txt and dmargs.txt.
If the command line that invokes this executable is not blank, then
pause 2 seconds before spawing doom3.
*/

#include "Launcher.h"

#include "boost/filesystem.hpp"

int main(int argc, char* argv[])
{
	// path to this exe
	boost::filesystem::path dmlauncher(argv[0]);

	// path to the darkmod directory
	boost::filesystem::path darkmodDir = dmlauncher.remove_leaf();

	// Instantiate a new Launcher class
	Launcher launcher(darkmodDir);
	launcher.Launch();

	return 0;
}
