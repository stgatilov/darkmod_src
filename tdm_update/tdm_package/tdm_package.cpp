#include <cstdlib>
#include <iostream>

#include "TraceLog.h"
#include "ExceptionSafeThread.h"

#include "LogWriters.h"
#include "Packager/Packager.h"
#include "Packager/PackagerOptions.h"
#include "Util.h"

#include <boost/bind.hpp>
#include <boost/format.hpp>

#include <map>

namespace tdm
{

} // namespace

using namespace tdm;
using namespace packager;

int main(int argc, char* argv[])
{
	// Start logging
	RegisterLogWriters();

	TraceLog::WriteLine(LOG_STANDARD, "TDM Packager v0.01 (c) 2010 by greebo. Part of The Dark Mod (http://www.thedarkmod.com).");
	TraceLog::WriteLine(LOG_STANDARD, "");

	// Parse the command line
	PackagerOptions options(argc, argv);

	if (options.Empty() || options.IsSet("help"))
	{
		options.PrintHelp();
		return EXIT_SUCCESS;
	}

	try
	{
		if (options.IsSet("create-update-package"))
		{
			if (options.Get("basedir").empty() || options.Get("headdir").empty() ||
				options.Get("baseversion").empty() || options.Get("headversion").empty() ||
				options.Get("outputdir").empty())
			{
				options.PrintHelp();
				return EXIT_SUCCESS;
			}

			Packager packager(options);

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			// Analyse base
			packager.GatherBaseSet();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			// Analyse head
			packager.GatherHeadSet();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			// Calculate the set difference of the two packages
			packager.CalculateSetDifference();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			// Pack the changed files into an update PK4
			packager.CreateUpdatePackage();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			TraceLog::WriteLine(LOG_STANDARD, "Done.");
		}
		else if (options.IsSet("update-version-info-file"))
		{
			if (options.Get("basedir").empty() || options.Get("baseversion").empty() ||
				options.Get("outputdir").empty())
			{
				options.PrintHelp();
				return EXIT_SUCCESS;
			}

			Packager packager(options);

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			// Analyse base
			packager.GatherBaseSet();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			// Pack the changed files into an update PK4
			packager.CreateVersionInformation();

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			TraceLog::WriteLine(LOG_STANDARD, "Done.");
		}
		else if (options.IsSet("register-update-package"))
		{
			if (options.Get("package").empty() || options.Get("outputdir").empty())
			{
				options.PrintHelp();
				return EXIT_SUCCESS;
			}

			Packager packager(options);

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			packager.RegisterUpdatePackage(options.Get("package"));

			TraceLog::WriteLine(LOG_STANDARD, "---------------------------------------------------------");

			TraceLog::WriteLine(LOG_STANDARD, "Done.");
		}
	}
	catch (std::runtime_error& ex)
	{
		TraceLog::Error(ex.what());
		return EXIT_FAILURE;
	}

	
	return EXIT_SUCCESS;
}
