/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __AREA_MANAGER_H__
#define __AREA_MANAGER_H__

#include <map>
#include <set>

class idAI;

namespace ai
{

class AreaManager
{
private:
	// angua: Forbidden areas (e.g. areas with locked doors) are excluded from path finding 
	// for specific AI
	// ForbiddenAreasMap: multimap of area number and the AI for which this area should be excluded
	typedef std::multimap<int, const idAI*> ForbiddenAreasMap;
	ForbiddenAreasMap _forbiddenAreas;

	// angua: AiAreasMap: gives a set of areas for each AI (for faster lookup)
	typedef std::set<int> AreaSet;
	typedef std::map<const idAI*, AreaSet> AiAreasMap;
	AiAreasMap _aiAreas;

public:
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	void AddForbiddenArea(int areanum, const idAI* ai);
	bool AreaIsForbidden(int areanum, const idAI* ai) const;
	void RemoveForbiddenArea(int areanum, const idAI* ai);

	void DisableForbiddenAreas(const idAI* ai);
	void EnableForbiddenAreas(const idAI* ai);

	void Clear();
};

} // namespace ai

#endif /* __AREA_MANAGER_H__ */
