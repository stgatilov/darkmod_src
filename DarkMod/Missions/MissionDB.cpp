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
	// Load mission database file
	ReloadMissionInfoFile();
}

void CMissionDB::ReloadMissionInfoFile()
{
	// Clear mission info structure first
	_missionInfo.clear();

	// Check if the mission info file exists
	if (fileSystem->ReadFile(cv_default_mission_info_file.GetString(), NULL) == -1)
	{
		// File doesn't exist, skip this step
		gameLocal.Printf("Parsed 0 mission declarations, no mission database file present.\n");

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("TDM Mission Info file not existing, skipping parse.\r");
		return;
	}

	idLexer src(cv_default_mission_info_file.GetString());
	src.SetFlags(DECL_LEXER_FLAGS & ~LEXFL_NOSTRINGESCAPECHARS );
	
	idToken token;

	while (true)
	{
		// If there's an EOF, this is an error.
		if (!src.ReadToken(&token))
		{
			break;
		}

		if (token.type == TT_NAME && idStr::Cmp(token.c_str(), CMissionInfoDecl::TYPE_NAME) == 0)
		{
			if (!src.ReadToken(&token))
			{
				src.Warning("Missing name on info declaration in %s line %d", src.GetFileName(), src.GetLineNum());
				break;
			}

			// Found a tdm_missioninfo declaration
			CMissionInfoDeclPtr decl(new CMissionInfoDecl);

			std::pair<MissionInfoMap::iterator, bool> result = _missionInfo.insert(
				MissionInfoMap::value_type(token.c_str(), CMissionInfoPtr(new CMissionInfo(token.c_str(), decl))));

			if (!result.second)
			{
				src.Warning("Duplicate mission info declaration found: %s", token.c_str());

				// Attempt to skip over this decl
				src.SkipUntilString("}");
				continue;
			}

			// Load the data from the text files, if possible
			result.first->second->LoadMetaData();

			// Let the declaration parse the persistent info
			decl->Parse(src);
		}
		else
		{
			src.Warning("Unknown token encountered in mission info file: %s", token.c_str());
			break;
		}
	}

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Parsed %d mission declarations.\r", static_cast<int>(_missionInfo.size()));
	gameLocal.Printf("Parsed %d mission declarations.\n", static_cast<int>(_missionInfo.size()));
}

void CMissionDB::Save()
{
	// Create the mission database file
	idStr infoFilePath = g_Global.GetDarkmodPath().c_str();
	infoFilePath += "/";
	infoFilePath += cv_default_mission_info_file.GetString();

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Saving Mission database to %s\r", infoFilePath.c_str());

	idFile* outFile = fileSystem->OpenExplicitFileWrite(infoFilePath);

	if (outFile == NULL)
	{
		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Could not open mission database for writing: %s\r", infoFilePath.c_str());
		return;
	}

	// Save changed declarations
	for (MissionInfoMap::iterator i = _missionInfo.begin(); i != _missionInfo.end(); ++i)
	{
		i->second->SaveToFile(outFile);
	}

	fileSystem->CloseFile(outFile);

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Done saving mission info declarartions\r");

	/*idStr fs_game = cvarSystem->GetCVarString("fs_game");

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
			}CMissionInfoDeclPtr(new CMissionInfoDecl)
		}
	}*/
}

const CMissionInfoPtr& CMissionDB::GetMissionInfo(const idStr& name)
{
	MissionInfoMap::iterator i = _missionInfo.find(name.c_str());
	
	if (i != _missionInfo.end())
	{
		return i->second;
	}

	// Create a new one
	CMissionInfoDeclPtr decl(new CMissionInfoDecl);
	CMissionInfoPtr missionInfo(new CMissionInfo(name, decl));

	std::pair<MissionInfoMap::iterator, bool> result = _missionInfo.insert(
		MissionInfoMap::value_type(name.c_str(), missionInfo));

	// Load the data from the text files, if possible
	missionInfo->LoadMetaData();

	return result.first->second;
}

bool CMissionDB::MissionInfoExists(const idStr& name)
{
	return _missionInfo.find(name.c_str()) != _missionInfo.end();
}
