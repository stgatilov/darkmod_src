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

#include "../IniFile.h"
#include "../TraceLog.h"

#include <map>
#include "../StdString.h"
#include <string>

namespace tdm
{

// Information about a TDM mirror
struct Mirror
{
	// Display name, e.g. "KeepOfMetalAndGold"
	std::string displayName;

	// URL prefix, e.g. "http://www.keepofmetalandgold.com/files/tdm/"
	std::string	url;

	// Probability [0..1]
	float weight;

	Mirror(const std::string& displayName_,
		   const std::string& url_,
		   float weight_) :
		displayName(displayName_),
		url(url_),
		weight(weight_)
	{
		// grayman #3208 - remove all spaces in the url
		auto end_iter = std::remove(url.begin(), url.end(), ' ');
		url.erase(end_iter, url.end());

		// url should terminate with "/"
		if (!stdext::ends_with(url, "/"))
		{
			url += "/";
		}
	}
};

/**
 * The list of mirrors available for download.
 * Each mirror has a certain "weight" to accommodate
 * the various download limits imposed on the people
 * who are generously offering to host our files.
 */
class MirrorList :
	public std::vector<Mirror>,
	public IniFile::SectionVisitor
{
public:
	MirrorList()
	{}

	// Construct this mirror list from the given ini file
	MirrorList(const IniFile& iniFile)
	{
		iniFile.ForeachSection(*this);

		NormaliseWeights();
	}

	void VisitSection(const IniFile& iniFile, const std::string& sectionName)
	{
		if (!stdext::istarts_with(sectionName, "Mirror "))
		{
			return; // ignore non-Mirror sections
		}

		try
		{
			push_back(Mirror(
				sectionName.substr(7), // displayname
				iniFile.GetValue(sectionName, "url"),
                std::stof(iniFile.GetValue(sectionName, "weight"))
			));
		}
		catch (std::invalid_argument&)
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Invalid weight on mirror " + sectionName);
		}
	}

private:
	void NormaliseWeights()
	{
		// Calculate weight sum
		float sum = 0;

		for (const_iterator i = begin(); i != end(); ++i)
		{
			sum += i->weight;
		}

		// Normalise weights
		if (sum > 0)
		{
			for (iterator i = begin(); i != end(); ++i)
			{
				i->weight /= sum;
			}
		}
		else
		{
			TraceLog::WriteLine(LOG_VERBOSE, "Invalid weights, total sum <= 0");
		}
	}
};

} // namespace
