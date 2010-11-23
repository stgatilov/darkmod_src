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

class CMissionInfoDecl;
typedef boost::shared_ptr<CMissionInfoDecl> CMissionInfoDeclPtr;

class CMissionInfo
{
private:
	// The "internal" mission info declaration, 
	// holding the persistent information about a mission (completion status, etc.)
	CMissionInfoDeclPtr _decl;

	// TRUE if the underlying declaration has been altered and needs saving
	bool _declDirty;

	// The cached size of the fs_game folder, in KB
	std::size_t _modFolderSize;
	bool		_modFolderSizeComputed;

public:
	// Public Properties - these aren't stored in the mission info declaration
	// but are constructed from the text files found in the fms/mission/ folders.

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

	CMissionInfo(const idStr& modName_, const CMissionInfoDeclPtr& detailsDecl) :
		_decl(detailsDecl),
		_declDirty(false),
		_modFolderSize(0),
		_modFolderSizeComputed(false),
		modName(modName_),
		requiredMajor(TDM_VERSION_MAJOR),
		requiredMinor(TDM_VERSION_MINOR)
	{}

	// Returns the size requirements of the fs_game folder
	// Returns 0 if the mod has not been installed yet
	std::size_t GetMissionFolderSize();

	// Returns the full OS path to the mod folder
	idStr GetMissionFolderPath();

	void ClearMissionFolderSize();

	// Fast check whether the readme.txt file exists
	bool HasMissionNotes();

	// Retrieves the readme.txt contents (is never cached, always read live from disk)
	idStr GetMissionNotes();

	// Returns true if this mission has been completed
	// Pass the difficulty level to check for a specific difficulty, or -1 (default) to check
	// whether the mission has been completed on any difficulty level.
	bool MissionCompleted(int difficultyLevel = -1);

	// Get the assembled mission completion string with difficulty information
	idStr GetMissionCompletedString();

	// Returns a human-readable format string (i.e. 1.33 GB)
	idStr	GetMissionFolderSizeString();

	// Returns a specific key value from the mission info declaration's dictionary
	idStr	GetKeyValue(const char* key, const char* defaultStr ="");

	// Saves a key into the internal declaration dictionary
	void	SetKeyValue(const char* key, const char* value);

	// Removes a certain keyvalue
	void	RemoveKeyValue(const char* key);

	// Removes key values matching the given prefix
	void	RemoveKeyValuesMatchingPrefix(const char* prefix);

	// Will save any persistent info to the given file
	void	SaveToFile(idFile* file);

	// Load stuff from darkmod.txt
	void	LoadMetaData();

	// Moves articles from the front of the string to its back "The Alchemist" => "Alchemist, The"
	static void MoveArticlesToBack(idStr& title);
};
typedef boost::shared_ptr<CMissionInfo> CMissionInfoPtr;

#endif /* _MISSION_INFO_H_ */
