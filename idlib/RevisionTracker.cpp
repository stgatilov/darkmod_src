/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "RevisionTracker.h"

#include <climits>
#include <vector>
#include <string>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

RevisionTracker::RevisionTracker() :
	_highestRevision(0),
	_lowestRevision(INT_MAX),
	_numFiles(0)
{}

int RevisionTracker::GetHighestRevision() const
{
	return _highestRevision;
}

int RevisionTracker::GetLowestRevision() const
{
	return _lowestRevision;
}

int RevisionTracker::GetNumFiles() const
{
	return _numFiles;
}

void RevisionTracker::AddRevision(int revision) 
{
	_numFiles++;

	if (_highestRevision < revision)
	{
		_highestRevision = revision;
	}

	if (_lowestRevision > revision)
	{
		_lowestRevision = revision;
	}
}

void RevisionTracker::ParseSVNIdString(const char* input)
{
	std::string revStr(input);
	std::vector<std::string> parts;

	// Split the incoming string into parts
	boost::algorithm::split(parts, revStr, boost::algorithm::is_any_of(" "));

	if (parts.size() > 1)
	{
		// The third token is the SVN revision, convert it to integer and pass it along
		Instance().AddRevision(atoi(parts[2].c_str()));
	}
}

// Accessor to the singleton
RevisionTracker& RevisionTracker::Instance()
{
	static RevisionTracker _instance;
	return _instance;
}
