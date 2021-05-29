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

#include "StdFormat.h"
#include "StdFilesystem.h"

#ifdef WIN32
	#define _WIN32_WINNT _WIN32_WINNT_VISTA
	#include <windows.h>
#else
	// Linux doesn't know Sleep(), add a substitute def
	#include <unistd.h>
	#define Sleep(x) usleep(static_cast<int>(1000 * (x)))
#endif 

namespace fs = stdext;

namespace tdm
{

/**
 * Generic helper classes
 */
class Util
{
public:
	// Formats the given number in bytes/kB/MB/GB
	static std::string GetHumanReadableBytes(std::size_t size)
	{
		if (size > 1024*1024*1024)
		{
			return stdext::format("%0.2f GB", (static_cast<double>(size) / (1024*1024*1024)));
		}
		else if (size > 1024*1024)
		{
			return stdext::format("%0.1f MB", (static_cast<double>(size) / (1024*1024)));
		}
		else if (size > 1024)
		{
			return stdext::format("%0.0f kB", (static_cast<double>(size) / 1024));
		}
		else
		{
			return stdext::format("%d bytes", size);
		}
	}

	/**
	 * greebo: Checks if the current path is the one TheDarkMod.exe is located in.
	 * This is used to determine whether this is a clean installation attempt
	 * and the user downloaded the tdm_update.exe into the wrong folder.

	 * grayman - check for The Dark Mod, not D3
	 * grayman - no longer necessary for 2.00.
	 */
/*
	static bool PathIsTDMEnginePath(const fs::path& path)
	{

#if WIN32
		std::string tdmExecutableName = "TheDarkMod.exe";
#else 
		std::string tdmExecutableName = "thedarkmod.x86";
#endif

		if (fs::exists(path / tdmExecutableName))
		{
			return true;
		}

		return false;
	}
	*/

	static void Wait(int millisecs)
	{
		Sleep(millisecs);
	}

	// Platform-dependent process check routine (grayman - searches for TheDarkMod executable among active processes)
	static bool TDMIsRunning();

	// Platform-dependent process check routine (searches for the DarkRadiant executable among active processes)
	static bool DarkRadiantIsRunning();

	// Is current process run "under admin"?
	// Returns false on Non-Windows platforms
	static bool HasElevatedPrivilegesWindows();
};

} // namespace
