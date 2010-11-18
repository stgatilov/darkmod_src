#pragma once

#include "stdafx.h"
#include "Updater/UpdaterOptions.h"
#include <boost/algorithm/string/predicate.hpp>

namespace tdm
{

namespace updater
{

class CommandLineInfo : 
	public CCommandLineInfo
{
public:
	UpdaterOptions options;

	CommandLineInfo()
	{}

	void ParseParam(const TCHAR* param, BOOL bFlag, BOOL bLast)
	{
		// greebo: ParseCommandLine() removes one dash from the front of the string
		std::wstring warg = param;
		std::string arg;
		arg.assign(warg.begin(), warg.end()); 

		if (arg == "-keep-mirrors")
		{
			options.Set("keep-mirrors");
		}
		else if (arg == "-keep-update-packages")
		{
			options.Set("keep-update-packages");
		}
		else if (arg == "-noselfupdate")
		{
			options.Set("noselfupdate");
		}
		else if (boost::algorithm::starts_with(arg, "-proxy="))
		{
			options.Set("proxy", arg.substr(7));
		}
		else if (boost::algorithm::starts_with(arg, "-targetdir="))
		{
			std::string value = arg.substr(11);

			if (boost::algorithm::starts_with(value, "\""))
			{
				value = value.substr(1);
			}

			if (boost::algorithm::ends_with(value, "\""))
			{
				value = value.substr(0, value.length() - 1);
			}

			options.Set("targetdir", value);
		}
	}
};

} // namespace

} // namespace
