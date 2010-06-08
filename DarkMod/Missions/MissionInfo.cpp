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

#include "MissionInfo.h"
#include "MissionInfoDecl.h"

std::size_t CMissionInfo::GetModFolderSize()
{
	return 0;
}

idStr CMissionInfo::GetKeyValue(const char* key)
{
	assert(_decl != NULL);

	return _decl->data.GetString(key);
}

void CMissionInfo::SetKeyValue(const char* key, const char* value)
{
	assert(_decl != NULL);

	_declDirty = true;

	_decl->data.Set(key, value);
}

void CMissionInfo::Save()
{
	// Don't do unnecessary work
	//if (!_declDirty) return;

	// Generate new declaration body text
	_decl->Update(modName);
	_decl->ReplaceSourceFileText();
}

void CMissionInfo::LoadMetaData()
{
	if (modName.IsEmpty()) 
	{
		DM_LOG(LC_MAINMENU, LT_ERROR)LOGSTRING("Cannot load mission information from darkomd.txt without mod name.\r");
		return;
	}

	idStr fmPath = cv_tdm_fm_path.GetString() + modName + "/";

	// Check for the darkmod.txt file
	idStr descFileName = fmPath + cv_tdm_fm_desc_file.GetString();

	char* buffer = NULL;

	if (fileSystem->ReadFile(descFileName, reinterpret_cast<void**>(&buffer)) == -1)
	{
		// File not found
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Couldn't find darkomd.txt for mod %s.\r", modName.c_str());
		return;
	}

	idStr modFileContent(buffer);
	fileSystem->FreeFile(buffer);

	if (modFileContent.IsEmpty())
	{
		// Failed to find info
		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Empty darkomd.txt for mod %s.\r", modName.c_str());
		return;
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
}
