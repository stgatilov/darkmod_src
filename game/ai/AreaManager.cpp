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
	for (ForbiddenAreasMap::const_iterator i = _forbiddenAreas.begin(); i != _forbiddenAreas.end(); ++i)
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
		idAI* ai;
		savefile->ReadObject( reinterpret_cast<idClass *&>( ai ) );
		AddForbiddenArea(areanum, ai);
	}
}

void AreaManager::AddForbiddenArea(int areanum, const idAI* ai)
{
	if (!AreaIsForbidden(areanum, ai))
	{
		_forbiddenAreas.insert(ForbiddenAreasMap::value_type(areanum, ai));

		AiAreasMap::iterator found = _aiAreas.find(ai);
		if (found != _aiAreas.end())
		{
			found->second.insert(AreaSet::value_type(areanum));
		}
		else
		{
			AreaSet set;
			// set.insert(areanum);
			std::pair<AiAreasMap::iterator, bool> result = _aiAreas.insert(AiAreasMap::value_type(ai, AreaSet()) );
			result.first->second.insert(areanum);
		}
	}
}

bool AreaManager::AreaIsForbidden(int areanum, const idAI* ai) const
{
	typedef ForbiddenAreasMap::const_iterator Iterator;
	std::pair<Iterator, Iterator> range = _forbiddenAreas.equal_range(areanum);

	for (Iterator found = range.first; found != range.second; ++found) 
	{
		if (found->second == ai)
		{
			return true;
		}
	}
	return false;
}

void AreaManager::RemoveForbiddenArea(int areanum, const idAI* ai)
{
	typedef ForbiddenAreasMap::iterator Iterator;
	std::pair<Iterator, Iterator> range = _forbiddenAreas.equal_range(areanum);

	for (Iterator found = range.first; found != range.second; ++found) 
	{
		if (found->second == ai)
		{
			_forbiddenAreas.erase(found);
			break;
		}
	}

	AiAreasMap::iterator foundAI = _aiAreas.find(ai);
	if (foundAI != _aiAreas.end())
	{
		AreaSet::iterator foundArea = foundAI->second.find(areanum);
		if (foundArea != foundAI->second.end())
		{
			foundAI->second.erase(foundArea);
		}
	}
}

void AreaManager::DisableForbiddenAreas(const idAI* ai)
{
	AiAreasMap::iterator foundAI = _aiAreas.find(ai);
	if (foundAI != _aiAreas.end())
	{
		idAAS* aas = ai->GetAAS();
		for (AreaSet::iterator i = foundAI->second.begin(); i != foundAI->second.end(); ++i)
		{
			aas->DisableArea(*i);
		}
	}
}

void AreaManager::EnableForbiddenAreas(const idAI* ai)
{
	AiAreasMap::iterator foundAI = _aiAreas.find(ai);
	if (foundAI != _aiAreas.end())
	{
		idAAS* aas = ai->GetAAS();
		for (AreaSet::iterator i = foundAI->second.begin(); i != foundAI->second.end(); ++i)
		{
			aas->EnableArea(*i);
		}
	}
}

void AreaManager::Clear()
{
	_forbiddenAreas.clear();
	_aiAreas.clear();
}


} // namespace ai
