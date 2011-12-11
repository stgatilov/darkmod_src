/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#pragma once

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "TraceLog.h"

namespace fs = boost::filesystem;

namespace tdm
{

class File
{
public:
	// Returns the path relative to the given root
	static fs::path GetRelativePath(const fs::path& absolute, const fs::path& root)
	{
		std::string fullString = absolute.file_string();

		if (boost::algorithm::starts_with(fullString, root.file_string()))
		{
			boost::algorithm::replace_first(fullString, root.file_string(), "");

			if (fullString.length() > 0 && (fullString[0] == '/' || fullString[0] == '\\'))
			{
				fullString = fullString.substr(1);
			}

			return fs::path(fullString);
		}

		// Return input
		return absolute;
	}

	// True if the given file has ending PK4 or ZIP
	static bool IsArchive(const fs::path& file)
	{
		std::string extension = boost::to_lower_copy(fs::extension(file));

		return (extension == ".pk4" || extension == ".zip");
	}

	// True if the given file has ending ZIP
	static bool IsZip(const fs::path& file)
	{
		std::string extension = boost::to_lower_copy(fs::extension(file));

		return extension == ".zip";
	}

	// True if the given file has ending PK4
	static bool IsPK4(const fs::path& file)
	{
		std::string extension = boost::to_lower_copy(fs::extension(file));

		return extension == ".pk4";
	}

	// Predicate to check whether the given file is a compressed one and whether it makes sense to deflate it at all
	static bool IsCompressed(const fs::path& file)
	{
		std::string extension = boost::to_lower_copy(fs::extension(file));

		return extension == ".pk4" || extension == ".jpg" || extension == ".ogg" ||
			   extension == ".zip";
	}

	static bool Remove(const fs::path& fileToRemove)
	{
		try
		{
			if (fs::exists(fileToRemove))
			{
				fs::remove(fileToRemove);
				TraceLog::WriteLine(LOG_VERBOSE, "Removed file " + fileToRemove.file_string());
			}

			return true;
		}
		catch (fs::basic_filesystem_error<fs::path>& e)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Exception while removing file: " + std::string(e.what()));
			return false;
		}
	}

	static bool Move(const fs::path& fromPath, const fs::path& toPath)
	{
		try
		{
			fs::rename(fromPath, toPath);
			TraceLog::WriteLine(LOG_VERBOSE, "Moved " + fromPath.file_string() + " to " + toPath.file_string());

			return true;
		}
		catch (fs::basic_filesystem_error<fs::path>& e)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Exception while moving file: " + std::string(e.what()));

			return false;
		}
	}

	static bool Copy(const fs::path& fromPath, const fs::path& toPath)
	{
		try
		{
			fs::copy_file(fromPath, toPath);
			TraceLog::WriteLine(LOG_VERBOSE, "Copied " + fromPath.file_string() + " to " + toPath.file_string());

			return true;
		}
		catch (fs::basic_filesystem_error<fs::path>& e)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Exception while copying file: " + std::string(e.what()));

			return false;
		}
	}

	// Marks the given file as executable (for all groups) - does nothing in Win32
	static void MarkAsExecutable(const fs::path& path);
};

}
