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

#ifndef _MISSION_DB_H_
#define _MISSION_DB_H_

#include <map>
#include <boost/shared_ptr.hpp>
#include "ModInfo.h"

/**
 * greebo: The mission database class holds the list of available TDM missions
 * and the corresponding data like play statistics, size info, etc.
 */
class CMissionDB
{
private:
	// Named mission info structures (fs_game => info)
	typedef std::map<std::string, CModInfoPtr> MissionInfoMap;
	MissionInfoMap _missionInfo;

public:
	CMissionDB();

	void Init();

	// Saves changed data to disk
	void Save();

	// Returns the mission info structure for this fs_game
	// Always returns non-NULL, if the name is not existing, 
	// a new structure will be created
	const CModInfoPtr& GetModInfo(const idStr& name);

	// Checks whether there is a record for the given mod name
	bool ModInfoExists(const idStr& name);

private:
	void ReloadMissionInfoFile();
};
typedef boost::shared_ptr<CMissionDB> CMissionDBPtr;

#endif /* _MISSION_DB_H_ */
