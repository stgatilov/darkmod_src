/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

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
		(boost::format("TDM Updater v%s (c) 2009-2011 by tels & greebo. Part of The Dark Mod (http://www.thedarkmod.com).") % LIBTDM_UPDATE_VERSION).str());
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
