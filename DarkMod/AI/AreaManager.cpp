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

#include "AreaManager.h"

namespace ai
{

void AreaManager::Save(idSaveGame* savefile) const
{
	int size = _forbiddenAreas.size();
	savefile->WriteInt(size);
	for (ForbiddenAreasMap::const_iterator i = _forbiddenAreas.begin(); i != _forbiddenAreas.end(); i++)
	{
		savefile->WriteInt(i->first);
		savefile->WriteObject(i->second);
	}
}

void AreaManager::Restore(idRestoreGame* savefile)
{
	int size;
	savefile->ReadInt(size);
	_forbiddenAreas.clear();
	for (int i = 0; i < size; i++)
	{
		int areanum;
		savefile->ReadInt(areanum);
		idActor* actor;
		savefile->ReadObject( reinterpret_cast<idClass *&>( actor ) );
		_forbiddenAreas.insert(ForbiddenAreasMap::value_type(areanum, actor));
	}
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


void AreaManager::Clear()
{
	_forbiddenAreas.clear();
}


} // namespace ai
