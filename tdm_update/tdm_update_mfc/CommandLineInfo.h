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

#include "stdafx.h"
#include "Updater/UpdaterOptions.h"

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
		else if (stdext::starts_with(arg, "-proxy="))
		{
			options.Set("proxy", arg.substr(7));
		}
		else if (stdext::starts_with(arg, "-targetdir="))
		{
			std::string value = arg.substr(11);

			if (stdext::starts_with(value, "\""))
			{
				value = value.substr(1);
			}

			if (stdext::ends_with(value, "\""))
			{
				value = value.substr(0, value.length() - 1);
			}

			options.Set("targetdir", value);
		}
	}
};

} // namespace

} // namespace
