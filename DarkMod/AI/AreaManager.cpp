/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: AreaManager.cpp 1435 2007-10-16 16:53:28Z angua $", init_version);

#include "AreaManager.h"

namespace ai
{

void AreaManager::Save(idSaveGame* savefile) const
{
	//TODO
}

void AreaManager::Restore(idSaveGame* savefile)
{
	//TODO
}

void AreaManager::AddForbiddenArea(int areanum, const idActor* actor)
{
	if (!AreaIsForbidden(areanum, actor))
	{
		_forbiddenAreas.insert(ForbiddenAreasMap::value_type(areanum, actor));
	}
}

bool AreaManager::AreaIsForbidden(int areanum, const idActor* actor) const
{
	typedef ForbiddenAreasMap::const_iterator Iterator;
	std::pair<Iterator, Iterator> range = _forbiddenAreas.equal_range(areanum);

	for (Iterator found = range.first; found != range.second; found++) 
	{
		if (found->second == actor)
		{
			return true;
		}
	}
	return false;
}

void AreaManager::RemoveForbiddenArea(int areanum, const idActor* actor)
{
	typedef ForbiddenAreasMap::iterator Iterator;
	std::pair<Iterator, Iterator> range = _forbiddenAreas.equal_range(areanum);

	for (Iterator found = range.first; found != range.second; found++) 
	{
		if (found->second == actor)
		{
			_forbiddenAreas.erase(found);
			return;
		}
	}
}



} // namespace ai
