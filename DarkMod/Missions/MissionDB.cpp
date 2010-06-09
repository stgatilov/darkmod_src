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

#include "MissionDB.h"
#include "MissionInfoDecl.h"
#include "MissionManager.h"

CMissionDB::CMissionDB()
{}

void CMissionDB::Init()
{
	// Nothing for now
}

void CMissionDB::Save()
{
	// Trigger saving all the changed declarations
	// This will write the new .info file into the current mod's folder
	// If no mission is installed, this will go into darkmod/fms.

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Saving Mission database %s\r", cv_default_mission_info_file.GetString());

	// Save changed declarations
	for (MissionInfoMap::iterator i = _missionInfo.begin(); i != _missionInfo.end(); ++i)
	{
		i->second->Save();
	}

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Done saving mission info declarartions\r");

	idStr fs_game = cvarSystem->GetCVarString("fs_game");

	if (!fs_game.IsEmpty() && fs_game != "darkmod")
	{
		// We have a mod installed. Move the newly written file back to darkmod
		fs::path darkmodPath = g_Global.GetDarkmodPath();
		darkmodPath /= cv_default_mission_info_file.GetString();

		fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
		parentPath = parentPath.remove_leaf().remove_leaf();

		fs::path srcPath = parentPath / fs_game.c_str() / cv_default_mission_info_file.GetString();
		
		if (fs::exists(srcPath))
		{
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Copying %s back to darkmod/fms/\r", cv_default_mission_info_file.GetString());

			CMissionManager::DoRemoveFile(darkmodPath);
			CMissionManager::DoMoveFile(srcPath, darkmodPath);

			// Check if the mission/fms/ folder is empty
			srcPath.remove_leaf();

			if (fs::is_empty(srcPath))
			{
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Removing empty fms/ folder %s\r", srcPath.file_string().c_str());

				CMissionManager::DoRemoveFile(srcPath);
			}
		}
	}
}

const CMissionInfoPtr& CMissionDB::GetMissionInfo(const idStr& name)
{
	MissionInfoMap::iterator i = _missionInfo.find(name.c_str());
	
	if (i != _missionInfo.end())
	{
		return i->second;
	}

	// Get the mission info declaration (or create it if not found so far)
	CMissionInfoDecl* decl = CMissionInfoDecl::FindOrCreate(name);

	CMissionInfoPtr missionInfo(new CMissionInfo(name, decl));

	std::pair<MissionInfoMap::iterator, bool> result = _missionInfo.insert(
		MissionInfoMap::value_type(name.c_str(), missionInfo));

	// Load the data from the text files, if possible
	missionInfo->LoadMetaData();

	return result.first->second;
}
