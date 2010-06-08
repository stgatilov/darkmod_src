/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id$", init_version);

#include "MissionDB.h"

namespace fs = boost::filesystem;

CMissionDB::CMissionDB()
{}

void CMissionDB::Init()
{
	// TODO
}

const CMissionInfoPtr& CMissionDB::GetMissionInfo(const idStr& name)
{
	MissionInfoMap::iterator i = _missionInfo.find(name.c_str());
	
	if (i == _missionInfo.end())
	{
		std::pair<MissionInfoMap::iterator, bool> result = _missionInfo.insert(
			MissionInfoMap::value_type(name.c_str(), CMissionInfoPtr(new CMissionInfo))
		);

		i = result.first;
	}

	return i->second;
}

void CMissionDB::LoadFromFile(const fs::path& file)
{
	// TODO
}
