/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _MISSION_INFO_H_
#define _MISSION_INFO_H_

#include "../../idlib/precompiled.h"
#include <boost/shared_ptr.hpp>

class CMissionInfo
{
private:
	// The cached size of the fs_game folder, in KB
	std::size_t _modFolderSize;
	bool		_modFolderSizeComputed;

public:
	idStr modName;			// The mod name (fs_game)
	idStr displayName;		// The display name of the mission
	idStr pathToFMPackage;	// path to PK4 in fms/ folder
	idStr description;		// description text
	idStr author;			// author(s)
	idStr image;			// splash image

	// Required TDM version
	idStr requiredVersionStr;
	int requiredMajor;
	int requiredMinor;

	enum MissionPlayStatus
	{
		NOT_PLAYED_YET,
		IN_PROGRESS,
		COMPLETED,
	};

	idList<MissionPlayStatus> difficultiesPlayed;

	CMissionInfo() :
		_modFolderSize(0),
		_modFolderSizeComputed(false),
		requiredMajor(TDM_VERSION_MAJOR),
		requiredMinor(TDM_VERSION_MINOR)
	{
		difficultiesPlayed.SetNum(DIFFICULTY_COUNT);

		for (int i = 0; i < DIFFICULTY_COUNT; ++i)
		{
			difficultiesPlayed[i] = NOT_PLAYED_YET;
		}
	}

	// Returns the size requirements of the fs_game folder
	// Returns 0 if the mod has not been installed yet
	std::size_t GetModFolderSize();
};
typedef boost::shared_ptr<CMissionInfo> CMissionInfoPtr;

#endif /* _MISSION_INFO_H_ */
