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

#include "ModInfo.h"
#include "ModInfoDecl.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

std::size_t CModInfo::GetModFolderSize()
{
	if (_modFolderSizeComputed)
	{
		return _modFolderSize;
	}

	_modFolderSizeComputed = true;
	_modFolderSize = 0;

	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	fs::path modPath = parentPath / modName.c_str();

	if (fs::exists(modPath))
	{
		// Iterate over all files in the mod folder
		for (fs::recursive_directory_iterator i(modPath); 
			i != fs::recursive_directory_iterator(); ++i)
		{
			if (fs::is_directory(i->path())) continue;

			_modFolderSize += fs::file_size(i->path());
		}
	}

	return _modFolderSize;
}

void CModInfo::ClearModFolderSize()
{
	_modFolderSize = 0;
	_modFolderSizeComputed = false;
}

idStr CModInfo::GetModFolderSizeString()
{
	float size = static_cast<float>(GetModFolderSize());

	// TODO: i18n
	idStr str;

	if (size < 1024)
	{
		str = va("%0.2f Bytes", size);
	}
	else if (size < 1024*1024)
	{
		str = va("%0.0f kB", size/1024.0f);
	}
	else if (size < 1024.0f*1024.0f*1024.0f)
	{
		str = va("%0.0f MB", size/(1024.0f*1024.0f));
	}
	else if (size < 1024.0f*1024.0f*1024.0f*1024.0f)
	{
		str = va("%0.2f GB", size/(1024.0f*1024.0f*1024.0f));
	}

	return str;
}

bool CModInfo::ModCompleted(int difficultyLevel)
{
	bool anyCompleted = false;

	for (int i = 0; i < DIFFICULTY_COUNT; ++i)
	{
		bool diffCompleted = GetKeyValue(va("mission_completed_%d", i)) == "1";

		if (difficultyLevel == i)
		{
			return diffCompleted;
		}

		// Accumulate the information
		anyCompleted |= diffCompleted;
	}

	return anyCompleted;
}

idStr CModInfo::GetModCompletedString()
{
	if (modName == "training_mission")
	{
		return common->GetLanguageDict()->GetString( "#str_02511" );	// Not completable
	}

	idStr diffStr;

	bool anyCompleted = false;

	for (int i = 0; i < DIFFICULTY_COUNT; ++i)
	{
		bool diffCompleted = GetKeyValue(va("mission_completed_%d", i)) == "1";

		if (diffCompleted)
		{
			diffStr += diffStr.Length() > 0 ? ", " : "";
			diffStr += gameLocal.m_DifficultyManager.GetDifficultyName(i);

			anyCompleted = true;
		}
	}

	if (anyCompleted)
	{
		return va( common->GetLanguageDict()->GetString( "#str_02513" ), diffStr.c_str());	// "Yes (%s)"
	}
	else
	{
		return common->GetLanguageDict()->GetString( "#str_02512" );	// Not yet
	}
}

idStr CModInfo::GetKeyValue(const char* key, const char* defaultStr)
{
	if (_decl == NULL) return defaultStr;

	return _decl->data.GetString(key, defaultStr);
}

void CModInfo::SetKeyValue(const char* key, const char* value)
{
	if (_decl == NULL) return;

	_declDirty = true;

	_decl->data.Set(key, value);
}

void CModInfo::RemoveKeyValue(const char* key)
{
	if (_decl == NULL) return;

	_declDirty = true;

	_decl->data.Delete(key);
}

void CModInfo::RemoveKeyValuesMatchingPrefix(const char* prefix)
{
	if (_decl == NULL) return;

	for (const idKeyValue* kv = _decl->data.MatchPrefix(prefix, NULL); 
		 kv != NULL; 
		 kv = _decl->data.MatchPrefix(prefix, NULL))
	{
		_decl->data.Delete(kv->GetKey());
	}
}

void CModInfo::SaveToFile(idFile* file)
{
	if (_decl == NULL) return;

	_decl->Update(modName);
	_decl->SaveToFile(file);
}

idStr CModInfo::GetModFolderPath()
{
	fs::path parentPath(fileSystem->RelativePathToOSPath("", "fs_savepath"));
	parentPath = parentPath.remove_leaf().remove_leaf();

	fs::path modPath = parentPath / modName.c_str();

	return modPath.file_string().c_str();
}

bool CModInfo::HasModNotes()
{
	// Check for the readme.txt file
	idStr notesFileName = cv_tdm_fm_path.GetString() + modName + "/" + cv_tdm_fm_notes_file.GetString();

	return fileSystem->ReadFile(notesFileName, NULL) != -1;
}

idStr CModInfo::GetModNotes()
{
	// Check for the readme.txt file
	idStr notesFileName = cv_tdm_fm_path.GetString() + modName + "/" + cv_tdm_fm_notes_file.GetString();

	char* buffer = NULL;

	if (fileSystem->ReadFile(notesFileName, reinterpret_cast<void**>(&buffer)) == -1)
	{
		// File not found
		return "";
	}

	idStr modNotes(buffer);
	fileSystem->FreeFile(buffer);

	return modNotes;
}

bool CModInfo::LoadMetaData()
{
	if (modName.IsEmpty()) 
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Cannot load mission information from darkmod.txt without mod name.\r");
		return false;
	}

	idStr fmPath = cv_tdm_fm_path.GetString() + modName + "/";

	// Check for the darkmod.txt file
	idStr descFileName = fmPath + cv_tdm_fm_desc_file.GetString();

	char* buffer = NULL;

	if (fileSystem->ReadFile(descFileName, reinterpret_cast<void**>(&buffer)) == -1)
	{
		// File not found
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Couldn't find darkmod.txt for mod %s.\r", modName.c_str());
		return false;
	}

	idStr modFileContent(buffer);
	fileSystem->FreeFile(buffer);

	if (modFileContent.IsEmpty())
	{
		// Failed to find info
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Empty darkmod.txt for mod %s.\r", modName.c_str());
		return false;
	}

	pathToFMPackage = fmPath;
	
	int titlePos = modFileContent.Find("Title:", false);
	int descPos = modFileContent.Find("Description:", false);
	int authorPos = modFileContent.Find("Author:", false);
	int versionPos = modFileContent.Find("Required TDM Version:", false);

	int len = modFileContent.Length();

	if (titlePos >= 0)
	{
		displayName = idStr(modFileContent, titlePos, (descPos != -1) ? descPos : len);
		displayName.StripLeadingOnce("Title:");
		displayName.StripLeading(" ");
		displayName.StripLeading("\t");
		displayName.StripTrailingWhitespace();

		// Remove articles from mission titles
		MoveArticlesToBack(displayName);
	}

	if (descPos >= 0)
	{
		description = idStr(modFileContent, descPos, (authorPos != -1) ? authorPos : len);
		description.StripLeadingOnce("Description:");
		description.StripLeading(" ");
		description.StripLeading("\t");
		description.StripTrailingWhitespace();
	}

	if (authorPos >= 0)
	{
		author = idStr(modFileContent, authorPos, (versionPos != -1) ? versionPos : len);
		author.StripLeadingOnce("Author:");
		author.StripLeading(" ");
		author.StripLeading("\t");
		author.StripTrailingWhitespace();
	}

	if (versionPos >= 0)
	{
		requiredVersionStr = idStr(modFileContent, versionPos, len);
		requiredVersionStr.StripLeadingOnce("Required TDM Version:");
		requiredVersionStr.StripLeading(" ");
		requiredVersionStr.StripLeading("\t");
		requiredVersionStr.StripTrailingWhitespace();

		// Parse version
		int dotPos = requiredVersionStr.Find('.');

		if (dotPos != -1)
		{
			requiredMajor = atoi(requiredVersionStr.Mid(0, dotPos));
			requiredMinor = atoi(requiredVersionStr.Mid(dotPos + 1, requiredVersionStr.Length() - dotPos));
		}
	}

	// Check for mod image
	if (fileSystem->ReadFile(pathToFMPackage + cv_tdm_fm_splashimage_file.GetString(), NULL) != -1)
	{
		idStr splashImageName = cv_tdm_fm_splashimage_file.GetString();
		splashImageName.StripFileExtension();

		image = pathToFMPackage + splashImageName;
	}

	return true;
}

void CModInfo::MoveArticlesToBack(idStr& title)
{
	if (title.StripLeadingOnce("The "))
	{
		title += ", The";
	}
	else if (title.StripLeadingOnce("A "))
	{
		title += ", A";
	}
	else if (title.StripLeadingOnce("An "))
	{
		title += ", An";
	}
}
