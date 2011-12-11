/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once

#include <boost/format.hpp>
#include <boost/filesystem.hpp>

// Platform-specific Sleep(int msec) definition
#ifdef WIN32
	#include <windows.h>
#else
	// Linux doesn't know Sleep(), add a substitute def
	#include <unistd.h>
	#define Sleep(x) usleep(static_cast<int>(1000 * (x)))
#endif 

namespace fs = boost::filesystem;

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
			return (boost::format("%0.2f GB") % (static_cast<double>(size) / (1024*1024*1024))).str();
		}
		else if (size > 1024*1024)
		{
			return (boost::format("%0.1f MB") % (static_cast<double>(size) / (1024*1024))).str();
		}
		else if (size > 1024)
		{
			return (boost::format("%0.0f kB") % (static_cast<double>(size) / 1024)).str();
		}
		else
		{
			return (boost::format("%d bytes") % size).str();
		}
	}

	/**
	 * greebo: Checks if the current path is the one Doom3.exe is located in.
	 * This is used to determine whether this is a clean installation attempt
	 * and the user downloaded the tdm_update.exe into the wrong folder.
	 */
	static bool PathIsDoom3EnginePath(const fs::path& path)
	{

#if WIN32
		std::string d3ExecutableName = "DOOM3.exe";
#else 
		std::string d3ExecutableName = "base/gamex86.so";
#endif

		if (fs::exists(path / d3ExecutableName))
		{
			return true;
		}

		return false;
	}

	static void Wait(int millisecs)
	{
		Sleep(millisecs);
	}

	// Platform-dependent process check routine (searches for gamex86.dll/gamex86.so)
	static bool D3IsRunning();

	// Platform-dependent process check routine (searches for DarkRadiant)
	static bool DarkRadiantIsRunning();
};

} // namespace
