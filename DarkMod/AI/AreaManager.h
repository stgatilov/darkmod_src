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

class idActor;

namespace ai
{

class AreaManager
{
private:
	// angua: Forbidden areas (e.g. areas with locked doors) are excluded from path finding 
	// for specific AI
	// ForbiddenAreasMap: multi´map of area number and the AI for which this area should be excluded
	typedef std::multimap<int, const idActor*> ForbiddenAreasMap;
	ForbiddenAreasMap _forbiddenAreas;

public:
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	void AddForbiddenArea(int areanum, const idActor* actor);
	bool AreaIsForbidden(int areanum, const idActor* actor) const;
	void RemoveForbiddenArea(int areanum, const idActor* actor);

	void Clear();
};

} // namespace ai

#endif /* __AREA_MANAGER_H__ */
