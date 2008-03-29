/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
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
