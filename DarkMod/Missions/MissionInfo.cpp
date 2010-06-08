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
	if (!_declDirty) return;

	// Generate new declaration body text
	_decl->SetText("\"Test\" \"Body\"");
	_decl->ReplaceSourceFileText();
}
