/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-01-27 18:53:28 +0200 (Di, 27 Jan 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "DifficultyManager.h"

// Constructor
CDifficultyManager::CDifficultyManager() :
	_difficulty(0)
{}

void CDifficultyManager::SetDifficultyLevel(int difficulty)
{
	_difficulty = difficulty;
}

int CDifficultyManager::GetDifficultyLevel() const
{
	return _difficulty;
}

void CDifficultyManager::Save(idSaveGame* savefile)
{
	savefile->WriteInt(_difficulty);
}

void CDifficultyManager::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt(_difficulty);
}
