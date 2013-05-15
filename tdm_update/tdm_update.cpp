/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod Updater (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include <cstdlib>
#include <iostream>

#include "TraceLog.h"

#include "Updater/UpdaterOptions.h"
#include "Updater/Updater.h"

#include "LogWriters.h"
#include "Util.h"

#include <boost/bind.hpp>
#include <boost/format.hpp>

#include "ConsoleUpdater.h"

using namespace tdm;
using namespace updater;

int main(int argc, char* argv[])
{
	// Start logging
	RegisterLogWriters();

	TraceLog::WriteLine(LOG_STANDARD, 
		(boost::format("TDM Updater v%s (c) 2009-2013 by tels & greebo. Part of The Dark Mod (http://www.thedarkmod.com).") % LIBTDM_UPDATE_VERSION).str());
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
