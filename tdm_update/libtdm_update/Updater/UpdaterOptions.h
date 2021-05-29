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

#pragma once

#include <regex>
#include "../StdString.h"
#include "../ProgramOptions.h"

namespace tdm
{

namespace updater
{

class UpdaterOptions :
	public ProgramOptions
{
public:
	UpdaterOptions()
	{
		SetupDescription();
	}

	// Construct options from command line arguments
	UpdaterOptions(int argc, char* argv[])
	{
		SetupDescription();
		ParseFromCommandLine(argc, argv);

		for (int i = 1; i < argc; ++i)
		{
			_cmdLineArgs.push_back(argv[i]);
		}
	}

	void CheckProxy(const HttpConnectionPtr& conn) const
	{
		std::string proxyStr = Get("proxy");

		if (proxyStr.empty()) 
		{
			TraceLog::WriteLine(LOG_VERBOSE, "No proxy configured.");
			return; // nothing to do
		}

		if (stdext::ends_with(proxyStr, "/"))
		{
			proxyStr = proxyStr.substr(proxyStr.length() - 1);
		}

		// Get the proxy info out of this string
		std::regex expr("^(https?)://(([^@]+)|([^@]+)@([^@]+))$",
						  std::regex::ECMAScript|std::regex::icase);

		std::smatch matches;
		
		if (std::regex_match(proxyStr, matches, expr))
		{
			if (matches[3].matched)
			{
				// Non-authenticated proxy
				TraceLog::WriteLine(LOG_VERBOSE, "Using proxy: " + matches[3].str());
				conn->SetProxyHost(matches[3]);
			}
			else if (matches[5].matched)
			{
				// Proxy with authentication
				TraceLog::WriteLine(LOG_VERBOSE, "Using proxy with authentication: " + matches[5].str());
				conn->SetProxyHost(matches[5]);
				
				// Split the username and password
				std::vector<std::string> parts;
				std::string userPassStr = matches[4].str();
				stdext::split(parts, userPassStr, ":");

				if (parts.size() == 2)
				{
					conn->SetProxyUsername(parts[0]);
					conn->SetProxyPassword(parts[1]);
				}
				else
				{
					TraceLog::WriteLine(LOG_VERBOSE, "Discarding proxy authentication information, wrong format.");
				}
			}
			else
			{
				TraceLog::WriteLine(LOG_VERBOSE, "No proxy host found, ignoring proxy setting.");
			}
		}
		else
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Proxy string doesn't match the required pattern.");
		}
	}

private:
	// Implement base class method
	void SetupDescription()
	{
		_desc.push_back(Option::Str("proxy", "Use a proxy to connect to the internet, example \n--proxy=http://user:pass@proxy:port\n"));
		_desc.push_back(Option::Str("targetdir", "The folder which should be updated.\n--targetdir=c:\\games\\tdm\\darkmod\n"));
		_desc.push_back(Option::Flag("help", "Display this help page"));
		_desc.push_back(Option::Flag("keep-mirrors", "Don't download updated mirrors list from the server, use local one."));
		_desc.push_back(Option::Flag("keep-update-packages", "Don't delete downloaded update packages after applying them."));
		_desc.push_back(Option::Flag("noselfupdate", "Don't perform any special 'update the updater' routines."));
		_desc.push_back(Option::Flag("dry-run", "Don't do any updates, just perform checks."));
	}
};

}

}
