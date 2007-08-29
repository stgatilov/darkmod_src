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

CEscapePointManager::CEscapePointManager() :
	_escapeEntities(new EscapeEntityList)
{}

CEscapePointManager::~CEscapePointManager()
{}

void CEscapePointManager::Clear()
{
	_escapeEntities->Clear();
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
	_escapeEntities->Append(pathFlee);
}

void CEscapePointManager::RemoveEscapePoint(tdmPathFlee* escapePoint)
{
	DM_LOG(LC_AI, LT_INFO).LogString("Removing escape point: %s\r", escapePoint->name.c_str());
	for (int i = 0; i < _escapeEntities->Num(); i++)
	{
		if ((*_escapeEntities)[i].GetEntity() == escapePoint) 
		{
			_escapeEntities->RemoveIndex(i);
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
			DM_LOG(LC_AI, LT_INFO).LogString("EscapePointManager: Initialising AAS: %s\r", aas->GetSettings()->fileExtension.c_str());

			// Allocate a new list for this AAS type
			_aasEscapePoints[aas] = EscapePointListPtr(new EscapePointList);

			// Now go through our master list and retrieve the area numbers 
			// for each tdmPathFlee entity
			for (int i = 0; i < _escapeEntities->Num(); i++)
			{
				tdmPathFlee* escapeEnt = (*_escapeEntities)[i].GetEntity();
				int areaNum = aas->PointAreaNum(escapeEnt->GetPhysics()->GetOrigin());

				DM_LOG(LC_AI, LT_INFO).LogString("Flee entity %s is in area number %d\r", escapeEnt->name.c_str(), areaNum);
				if (areaNum != -1)
				{
					// Fill the EscapePoint structure with the relevant information
					EscapePoint escPoint;

					escPoint.aasId = gameLocal.GetAASId(aas);
					escPoint.areaNum = areaNum;
					escPoint.origin = escapeEnt->GetPhysics()->GetOrigin();
					escPoint.pathFlee = (*_escapeEntities)[i];

					// Pack the info structure to this list
					_aasEscapePoints[aas]->Append(escPoint);
				}
			}

			DM_LOG(LC_AI, LT_INFO).LogString("EscapePointManager: AAS initialized: %s\r", aas->GetSettings()->fileExtension.c_str());
		}
	}
}

EscapeGoal CEscapePointManager::GetEscapePoint(const EscapeConditions& conditions)
{
	assert(aas != NULL);
	// Assert on a known AAS pointer
	assert(_aasEscapePoints.find(conditions.aas) != _aasEscapePoints.end());

	DM_LOG(LC_AI, LT_INFO).LogString("Calculating escape point info.\n");

	// Create a shortcut to the list
	EscapePointList& escapePoints = *_aasEscapePoints[conditions.aas];

	EscapeGoal goal;

	if (escapePoints.Num() == 0) {
		gameLocal.Warning("No escape point information available for the given aas type in map!\n");
		//goal.escapePoint = NULL;
		return goal;
	}

	for (int i = 0; i < escapePoints.Num(); i++)
	{
		
	}

	//goal.escapePoint = escapePoints[0];

	return goal;
}

CEscapePointManager* CEscapePointManager::Instance()
{
	static CEscapePointManager _manager;
	return &_manager;
}
