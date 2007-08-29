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
	_escapeEntities(new EscapeEntityList),
	_highestEscapePointId(0)
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
					// Increase the unique escape point ID
					_highestEscapePointId++;

					// Fill the EscapePoint structure with the relevant information
					EscapePoint escPoint;

					escPoint.id = _highestEscapePointId;
					escPoint.aasId = gameLocal.GetAASId(aas);
					escPoint.areaNum = areaNum;
					escPoint.origin = escapeEnt->GetPhysics()->GetOrigin();
					escPoint.pathFlee = (*_escapeEntities)[i];

					// Pack the info structure to this list
					int newIndex = _aasEscapePoints[aas]->Append(escPoint);

					// Store the pointer to this escape point into the lookup table
					// Looks ugly, it basically does this: map[int] = EscapePoint*
					_aasEscapePointIndex[escPoint.id] = &( (*_aasEscapePoints[aas])[newIndex] );
				}
			}

			DM_LOG(LC_AI, LT_INFO).LogString("EscapePointManager: AAS initialized: %s\r", aas->GetSettings()->fileExtension.c_str());
		}
	}
}

EscapePoint* CEscapePointManager::GetEscapePoint(int id)
{
	// Check the id for validity
	assert(_aasEscapePointIndex.find(id) != _aasEscapePointIndex.end());
	return _aasEscapePointIndex[id];
}

EscapeGoal CEscapePointManager::GetEscapeGoal(const EscapeConditions& conditions)
{
	assert(aas != NULL);
	// Assert on a known AAS pointer
	assert(_aasEscapePoints.find(conditions.aas) != _aasEscapePoints.end());

	DM_LOG(LC_AI, LT_INFO).LogString("Calculating escape point info.\r");

	// Create a shortcut to the list
	EscapePointList& escapePoints = *_aasEscapePoints[conditions.aas];

	EscapeGoal goal;

	if (escapePoints.Num() == 0)
	{
		gameLocal.Warning("No escape point information available for the given aas type in map!\n");
		goal.escapePointId = -1;
		goal.distance = -1;
		return goal;
	}
	else if (escapePoints.Num() == 1) 
	{
		// Only one point available, return that one
		DM_LOG(LC_AI, LT_DEBUG).LogString("Only one escape point available, returning this one: %d.\r", escapePoints[0].id);

		goal.escapePointId = escapePoints[0].id;
		goal.distance = (conditions.self.GetEntity()->GetPhysics()->GetOrigin() - escapePoints[0].origin).LengthFast();
		return goal;
	}

	// The location of the fleeing entity
	idVec3 selfOrigin = conditions.self.GetEntity()->GetPhysics()->GetOrigin();

	// The index of the best point so far (= the first one, better than nothing)
	int bestPoint = 0;
	goal.distance = (selfOrigin - escapePoints[0].origin).LengthFast();

	// Start with the second point in the list
	for (int i = 1; i < escapePoints.Num(); i++)
	{
		// Evaluate the given escape point

		// Is this point nearer than the currently best candidate?
		float distance = (selfOrigin - escapePoints[i].origin).LengthFast();

		if (distance > goal.distance)
		{
			// Yes, this is a better flee point
			bestPoint = i;
			goal.escapePointId = escapePoints[i].id;
			goal.distance = distance;
		}
	}

	DM_LOG(LC_AI, LT_DEBUG).LogString(
		"Best escape point has ID %d at %f %f %f in area %d.\r", 
		goal.escapePointId, 
		escapePoints[bestPoint].origin.x, escapePoints[bestPoint].origin.y, escapePoints[bestPoint].origin.z, 
		escapePoints[bestPoint].areaNum
	);

	return goal;
}

CEscapePointManager* CEscapePointManager::Instance()
{
	static CEscapePointManager _manager;
	return &_manager;
}
