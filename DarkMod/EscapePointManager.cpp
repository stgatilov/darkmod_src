/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 866 $
 * $Date: 2007-03-23 22:25:02 +0100 (Fr, 23 Mar 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/
#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: EscapePointManager.cpp 870 2007-03-27 14:21:59Z greebo $", init_version);

#include "EscapePointManager.h"

CEscapePointManager::CEscapePointManager()
{}

CEscapePointManager::~CEscapePointManager()
{}

void CEscapePointManager::Clear()
{
	_escapePoints.Clear();
}

void CEscapePointManager::Save( idSaveGame *savefile ) const
{
	// TODO
}

void CEscapePointManager::Restore( idRestoreGame *savefile )
{
	// TODO
}

void CEscapePointManager::AddEscapePoint(tdmPathFlee* escapePoint)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Adding escape point: %s\r", escapePoint->name.c_str());

	idEntityPtr<tdmPathFlee> pathFlee;
	pathFlee = escapePoint;
	_escapePoints.Append(pathFlee);
}

void CEscapePointManager::RemoveEscapePoint(tdmPathFlee* escapePoint)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Removing escape point: %s\r", escapePoint->name.c_str());
	for (int i = 0; i < _escapePoints.Num(); i++)
	{
		if (_escapePoints[i].GetEntity() == escapePoint) 
		{
			_escapePoints.RemoveIndex(i);
			return;
		}
	}

	// Not found
	DM_LOG(LC_AI, LT_ERROR).LogString("Failed to remove escape point: %s\r", escapePoint->name.c_str());
}

void CEscapePointManager::InitAAS()
{
	for (int i = 0; i < gameLocal.NumAAS(); i++)
	{
		idAAS* aas = gameLocal.GetAAS(i);

		if (aas != NULL) {
			gameLocal.Printf("EscapePointManager: AAS initialized: %s\n", aas->GetSettings()->fileExtension.c_str());
		}
	}
}

EscapeGoal CEscapePointManager::GetEscapePoint(const EscapeConditions& conditions)
{
	EscapeGoal goal;

	if (_escapePoints.Num() == 0) {
		gameLocal.Warning("No escape point information available in map!\n");
		goal.escapePoint = NULL;
		return goal;
	}

	DM_LOG(LC_AI, LT_INFO).LogString("Calculating escape point info.\n");

	goal.escapePoint = _escapePoints[0];

	return goal;
}

CEscapePointManager* CEscapePointManager::Instance()
{
	static CEscapePointManager _manager;
	return &_manager;
}
