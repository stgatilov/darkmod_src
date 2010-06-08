/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _MISSION_MANAGER_H_
#define _MISSION_MANAGER_H_

#include "MissionInfo.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

class CMissionDB;
typedef boost::shared_ptr<CMissionDB> CMissionDBPtr;

namespace fs = boost::filesystem;

class CMissionManager
{
private:
	CMissionDBPtr _missionDB;

	// A plain list of available fs_game names
	idStringList _availableMissions;

	// A list of path => path associations for moving files around
	typedef std::list< std::pair<fs::path, fs::path> > MoveList;

public:
	CMissionManager();

	// This initialises the list of available missions
	void Init();

	// Returns the number of available missions
	int GetNumMissions();

	// Returns the mission info by index (or NULL if out of bounds)
	CMissionInfoPtr GetMissionInfo(int index);

	// returns the mission info by name (or NULL if no such mission)
	CMissionInfoPtr GetMissionInfo(const idStr& name);

private:
	void SearchForNewMissions();

	// Sub-routine of SearchForNewMissions() investigating the FM folder
	// using the given extension (including dot ".pk4", ".zip")
	MoveList SearchForNewMissions(const idStr& extension);

	// Returns the path to the "darkmod" base
	fs::path GetDarkmodPath();

	// Clears the mission list and searches for mods to install, then calls GenerateMissionList()
	void ReloadMissionList();

	// Finds all available missions
	void GenerateMissionList();

	// Sorts all missions by display name
	void SortMissionList();

	// Compare functor to sort mods by display name
	static int MissionSortCompare(const int* a, const int* b);

	// Convenience method which copies a file from <source> to <dest>
	// If <overwrite> is set to TRUE, any existing destination file will be removed beforehand
	// Note: CopyFile is already #define'd in a stupid WinBase.h header file, hence DoCopyFile.
	bool DoCopyFile(const fs::path& source, const fs::path& dest, bool overwrite = false);

	// Removes the given file, returns TRUE if this succeeded or if file wasn't there in the first place, FALSE on error
	bool DoRemoveFile(const fs::path& fileToRemove);

	// Moves the given file, from <fromPath> to <toPath>
	bool DoMoveFile(const fs::path& fromPath, const fs::path& toPath);
};
typedef boost::shared_ptr<CMissionManager> CMissionManagerPtr;

#endif /* _MISSION_MANAGER_H_ */
