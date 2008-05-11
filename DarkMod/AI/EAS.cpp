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
	_clusterInfo(NULL),
	_numElevatorStations(0),
	_elevatorStations(NULL)
{}

void tdmEAS::Clear()
{
	_elevators.Clear();
	ClearClusterInfoStructures();
	ClearElevatorStationStructures();
}

void tdmEAS::AddElevator(CMultiStateMover* mover)
{
	_elevators.Alloc() = mover;
}

void tdmEAS::Compile()
{
	if (_aas == NULL)
	{
		gameLocal.Error("Cannot Compile EAS, no AAS available.");
	}

	// First, allocate the memory for the cluster info structures
	SetupClusterInfoStructures();
	SetupElevatorStationStructures();

	// Then, traverse the registered elevators and assign their "stations" or floors to the clusters
	AssignElevatorsToClusters();

	// Now setup the connection information between clusters
	SetupClusterRouting();
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

void tdmEAS::SetupElevatorStationStructures()
{
	if (_elevatorStations != NULL)
	{
		ClearElevatorStationStructures();
	}

	_numElevatorStations = 0;
	for (int i = 0; i < _elevators.Num(); i++)
	{
		CMultiStateMover* elevator = _elevators[i].GetEntity();
		const idList<MoverPositionInfo>& positionList = elevator->GetPositionInfoList();
		_numElevatorStations += positionList.Num();
	}

	// Allocate memory for each elevator station pointer
	_elevatorStations = (ElevatorStationInfo**) Mem_ClearedAlloc(_numElevatorStations*sizeof(ElevatorStationInfo*));
}

void tdmEAS::ClearElevatorStationStructures()
{
	if (_elevatorStations != NULL)
	{
		for (int i = 0; i < _numElevatorStations; i++)
		{
			Mem_Free(_elevatorStations[i]);
		}

		Mem_Free(_elevatorStations);
	}

	_elevatorStations = NULL;
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
	int stationIndex = 0;
	for (int i = 0; i < _elevators.Num(); i++)
	{
		CMultiStateMover* elevator = _elevators[i].GetEntity();

		const idList<MoverPositionInfo>& positionList = elevator->GetPositionInfoList();

		for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
		{
			CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();
			idVec3 origin = positionEnt->GetPhysics()->GetOrigin();
						
			int areaNum = _aas->file->PointReachableAreaNum(origin, _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);

			// If areaNum could not be determined, try again at a slightly higher position
			if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(origin + idVec3(0,0,16), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);
			if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(origin + idVec3(0,0,32), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);
			if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(origin + idVec3(0,0,48), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);

			if (areaNum == 0)
			{
				gameLocal.Warning("[%s]: Cannot assign multistatemover position to AAS area:  %s", _aas->name.c_str(), positionEnt->name.c_str());
				continue;
			}

			const aasArea_t& area = _aas->file->GetArea(areaNum);
			_clusterInfo[area.cluster]->numElevatorStations++;

			// Allocate a new ElevatorStationStructure for this station
			_elevatorStations[stationIndex] = (ElevatorStationInfo*) Mem_Alloc(sizeof(ElevatorStationInfo));
			
			_elevatorStations[stationIndex]->elevator = elevator;
			_elevatorStations[stationIndex]->elevatorPosition = positionEnt;
			_elevatorStations[stationIndex]->areaNum = areaNum;
			_elevatorStations[stationIndex]->clusterNum = area.cluster;
			stationIndex++;
		}
	}
}

void tdmEAS::SetupClusterRouting()
{
	for (int cluster = 0; cluster < _numClusterInfos; cluster++)
	{
		// Find an area within that cluster
		int areaNum = _aas->GetAreaInCluster(cluster);

		if (areaNum <= 0)
		{
			gameLocal.Warning("No areas in cluster %d?\n", cluster);
			continue;
		}

		// For each cluster, try to setup a route to all elevator stations
		for (int e = 0; e < _numElevatorStations; e++)
		{
			/*idBounds areaBounds = _aas->GetAreaBounds(areaNum);
			idVec3 areaCenter = _aas->AreaCenter(areaNum);

			gameRenderWorld->DrawText(va("%d", areaNum), areaCenter, 0.2f, colorRed, idAngles(0,0,0).ToMat3(), 1, 50000);
			gameRenderWorld->DebugBox(colorRed, idBox(areaBounds), 50000);

			areaBounds = _aas->GetAreaBounds(_elevatorStations[e]->areaNum);
			idVec3 areaCenter2 = _aas->AreaCenter(_elevatorStations[e]->areaNum);

			gameRenderWorld->DrawText(va("%d", _elevatorStations[e]->areaNum), areaCenter2, 0.2f, colorBlue, idAngles(0,0,0).ToMat3(), 1, 50000);
			gameRenderWorld->DebugBox(colorBlue, idBox(areaBounds), 50000);*/

			idReachability* reach;
			int travelTime = 0;
			bool routeFound = _aas->RouteToGoalArea(areaNum, _aas->AreaCenter(areaNum), 
				_elevatorStations[e]->areaNum, TFL_WALK|TFL_AIR, travelTime, &reach, NULL);

			//gameRenderWorld->DebugArrow(routeFound ? colorGreen : colorRed, areaCenter, areaCenter2, 1, 50000);
			
			if (routeFound) 
			{
				gameLocal.Printf("Cluster %d can reach elevator station %s\n", cluster, _elevatorStations[e]->elevatorPosition.GetEntity()->name.c_str());
			}
			else 
			{
				gameLocal.Printf("Cluster %d can NOT reach elevator station %s\n", cluster, _elevatorStations[e]->elevatorPosition.GetEntity()->name.c_str());
			}
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
