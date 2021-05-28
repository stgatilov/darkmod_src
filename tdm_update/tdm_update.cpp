/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include <cstdlib>
#include <iostream>

#include "TraceLog.h"

#include "Updater/UpdaterOptions.h"
#include "Updater/Updater.h"

#include "LogWriters.h"
#include "Util.h"

#include "StdFormat.h"

#include "ConsoleUpdater.h"

using namespace tdm;
using namespace updater;

int main(int argc, char* argv[])
{
	// stgatilov #5495: use tdm_installer
	(std::cerr
		<< "TDM Updater has reached end-of-life.\n"
		<< "It cannot install the latest version of TheDarkMod!\n"
		<< "Please visit website www.thedarkmod.com and download tdm_installer instead.\n"
		<< "\n"
		<< "Enter \"ignore\" if you want to continue anyway: "
	);
	std::string enteredStr;
	std::cin >> enteredStr;
	if (enteredStr != "ignore")
		return EXIT_FAILURE;

	// Start logging
	try {
		RegisterLogWriters();
	} 
	catch (FileOpenException &) {
		// most common cause of failure is: having updater in admin-owned directory where normal user cannot create files
		// other reasons are: file marked as readonly, file opened by running process
		(std::cerr
			<< "TDM Updater Error:\n"
			<< "Unable to open log file and start updater.\n"
			<< "\n"
			<< "Please ensure that:\n"
			<< "1. You can create files in the current directory without admin rights (better avoid Program Files).\n"
			<< "2. The current directory is not set to 'Read-only'. Neither is tdm_update.log, if it exists.\n"
			<< "3. Another instance of tdm_update.exe is not running.\n"
		);
        return EXIT_FAILURE;
	}

	TraceLog::WriteLine(LOG_STANDARD, 
        stdext::format("TDM Updater v%s/%d (c) 2009-2017 by tels & greebo. Part of The Dark Mod (http://www.thedarkmod.com).", LIBTDM_UPDATE_VERSION, (sizeof(void*) * 8)));
	TraceLog::WriteLine(LOG_STANDARD, "");

	ConsoleUpdater updater(argc, argv);

	updater.Run();

	int exitCode = EXIT_FAILURE;

	switch (updater.GetOutcome())
	{
		case ConsoleUpdater::None:
			// should not happen?
			break;
		case ConsoleUpdater::Failed:
			exitCode = EXIT_FAILURE;
			break;
		case ConsoleUpdater::Ok:
			exitCode = EXIT_SUCCESS;
			break;
		case ConsoleUpdater::OkNeedRestart:
			exitCode = EXIT_SUCCESS;
			break;
	};

	return exitCode;
}
