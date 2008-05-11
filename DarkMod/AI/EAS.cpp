/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-11 10:15:28 +0200 (Su, 11 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: EAS.cpp 1435 2008-05-11 10:15:28Z greebo $", init_version);

#include "EAS.h"

tdmEAS::tdmEAS(idAASLocal* aas) :
	_aas(aas),
	_numClusterInfos(0),
	_clusterInfo(NULL)
{}

void tdmEAS::Clear()
{
	_elevators.Clear();
	ClearClusterInfoStructures();
}

void tdmEAS::AddElevator(CMultiStateMover* mover)
{
	_elevators.Alloc() = mover;

	/*const idList<MoverPositionInfo>& infoEnts = mover->GetPositionInfoList();

	for (int i = 0; i < infoEnts.Num(); i++)
	{
		idEntity* positionEnt = infoEnts[i].positionEnt.GetEntity();

		int areaNum = PointAreaNum(positionEnt->GetPhysics()->GetOrigin());

		if (areaNum == 0) continue;

		// Add a reachability connecting this floor to all other floors
		for (int j = 0; j < infoEnts.Num(); j++)
		{
			if (i == j) continue; // don't add reachability to self

			const idVec3& otherOrg = infoEnts[j].positionEnt.GetEntity()->GetPhysics()->GetOrigin();
			int otherAreaNum = PointAreaNum(otherOrg);
			if (otherAreaNum == 0) continue;

		}
	}*/
}

void tdmEAS::Compile()
{
	if (_aas == NULL)
	{
		gameLocal.Error("Cannot Compile EAS, no AAS available.");
	}

	// First, allocate the memory for the cluster info structures
	SetupClusterInfoStructures();

	// Then, traverse the registered elevators and assign their "stations" or floors to the clusters
	AssignElevatorsToClusters();
}

void tdmEAS::SetupClusterInfoStructures() 
{
	if (_clusterInfo != NULL)
	{
		// Cluster info must be cleared beforehand
		ClearClusterInfoStructures();
	}

	_numClusterInfos = _aas->file->GetNumClusters();

	// Allocate the memory for the pointers
	_clusterInfo = (ClusterInfo**) Mem_ClearedAlloc(_numClusterInfos * sizeof(ClusterInfo*));
	
	for (int i = 0; i < _numClusterInfos; i++)
	{
		const aasCluster_t& cluster = _aas->file->GetCluster(i);
		
		_clusterInfo[i] = (ClusterInfo*) Mem_ClearedAlloc(sizeof(ClusterInfo));
		_clusterInfo[i]->clusterNum = i;
	}
}

void tdmEAS::ClearClusterInfoStructures()
{
	if (_clusterInfo != NULL)
	{
		for (int i = 0; i < _numClusterInfos; i++)
		{
			Mem_Free(_clusterInfo[i]);
		}

		Mem_Free(_clusterInfo);
	}

	_clusterInfo = NULL;
}

void tdmEAS::AssignElevatorsToClusters()
{
	for (int i = 0; i < _elevators.Num(); i++)
	{
		CMultiStateMover* elevator = _elevators[i].GetEntity();

		const idList<MoverPositionInfo>& positionList = elevator->GetPositionInfoList();

		for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
		{
			CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();
			idVec3 origin = positionEnt->GetPhysics()->GetOrigin();
						
			int areaNum = _aas->PointAreaNum(origin);

			// If areaNum could not be determined, try again at a slightly higher position
			if (areaNum == 0) areaNum = _aas->PointAreaNum(origin + idVec3(0,0,16));
			if (areaNum == 0) areaNum = _aas->PointAreaNum(origin + idVec3(0,0,32));
			if (areaNum == 0) areaNum = _aas->PointAreaNum(origin + idVec3(0,0,48));

			if (areaNum == 0)
			{
				gameLocal.Warning("[%s]: Cannot assign multistatemover position to AAS area:  %s", _aas->name.c_str(), positionEnt->name.c_str());
				continue;
			}

			const aasArea_t& area = _aas->file->GetArea(areaNum);
			_clusterInfo[area.cluster]->numElevators++;
		}
	}
}

void tdmEAS::Save(idSaveGame* savefile) const
{
	// TODO
}

void tdmEAS::Restore(idRestoreGame* savefile)
{
	// TODO
}
