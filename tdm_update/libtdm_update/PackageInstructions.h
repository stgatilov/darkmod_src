/***************************************************************************
 *
 * PROJECT: The Dark Mod - Updater
 * $Revision: 4379 $
 * $Date: 2010-12-22 15:49:40 +0100 (Mi, 22 Dez 2010) $
 * $Author: greebo $
 *
 ***************************************************************************/

#pragma once

#include <list>
#include <fstream>

#include "TraceLog.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace tdm
{

struct PackageInstruction
{
	enum Type
	{
		Exclude,	// EXCLUDE
		Include,	// INCLUDE
		Mission,	// FM
		Ignore,		// everything else
	};

	Type type;

	// The instruction value (regular expression or map file)
	std::string value;

	// The pre-compiled regex
	boost::regex regex;

	// Default constructor
	PackageInstruction() :
		type(Ignore)
	{}

	PackageInstruction(Type type_, const std::string& value_) :
		type(type_),
		value(value_),
		regex(value, boost::regex::perl)
	{}
};

/**
 * The package instruction file is usually "darkmod_maps.txt" in the devel/manifests folder.
 * It contains the INCLUDE and EXCLUDE statements defining which files
 * of the darkmod SVN repository should be packaged and which should be left out.
 *
 * Syntax:
 *
 * # Comment
 * INCLUDE <regexp>
 * EXCLUDE <regexp>
 * FM <mapfile> - ignored for now
 * ordinary lines (e.g. "training_mission.map" or "prefabs/ \.pfb\z") - ignored for now
 */
class PackageInstructions :
	public std::list<PackageInstruction>
{
public:
	// Returns true if the file given by the path (relative to repository root) is excluded
	bool IsExcluded(const std::string& relativePath) const
	{
		for (const_iterator i = begin(); i != end(); ++i)
		{
			if (i->type != PackageInstruction::Exclude) continue;

			// Found an exclusion instruction, check the regexp
			if (boost::regex_search(relativePath, i->regex))
			{
				TraceLog::WriteLine(LOG_VERBOSE, 
					(boost::format("[PackageInstructions]: Relative path %s excluded by regex %s") % relativePath % i->value).str());
				return true;
			}
		}

		return false; // not excluded
	}

	void LoadFromFile(const fs::path& file)
	{
		// Start parsing
		std::ifstream stream(file.file_string().c_str());

		if (!stream)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "[PackageInstructions]: Cannot open file " + file.file_string());
			return;
		}

		LoadFromStream(stream);
	}

	void LoadFromStream(std::istream& stream)
	{
		clear();

		std::string line;

		std::size_t includeStatements = 0;
		std::size_t excludeStatements = 0;
		std::size_t ignoredLines = 0;

		while (std::getline(stream, line, '\n'))
		{
			boost::algorithm::trim_if(line, boost::algorithm::is_any_of(" \t"));

			// Skip empty lines
			if (line.empty()) continue;

			// Skip line comments
			if (line[0] == '#') continue;

			if (boost::algorithm::starts_with(line, "INCLUDE"))
			{
				std::string value = line.substr(7);
				boost::algorithm::trim_if(value, boost::algorithm::is_any_of(" \t"));

				push_back(PackageInstruction(PackageInstruction::Include, value));

				includeStatements++;
			}
			else if (boost::algorithm::starts_with(line, "EXCLUDE"))
			{
				std::string value = line.substr(7);
				boost::algorithm::trim_if(value, boost::algorithm::is_any_of(" \t"));

				push_back(PackageInstruction(PackageInstruction::Exclude, value));

				excludeStatements++;
			}
			else
			{
				// Just ignore this line
				ignoredLines++;
			}
		}

		TraceLog::WriteLine(LOG_STANDARD, (boost::format("Parsed %d INCLUDEs, %d EXCLUDEs and ignored %d lines.") % 
			includeStatements % excludeStatements % ignoredLines).str());
	}

	void LoadFromString(const std::string& str)
	{
		std::istringstream inputStream(str);

		LoadFromStream(inputStream);
	}
};

} // namespace
