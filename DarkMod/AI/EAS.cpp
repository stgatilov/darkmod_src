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
	_elevatorStations(NULL),
	_routingIterations(0)
{}

void tdmEAS::Clear()
{
	_elevators.Clear();
	_clusterInfo.clear();;
	_elevatorStations.clear();;
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
	// Clear the vector and allocate a new one (one structure ptr for each cluster)
	_clusterInfo.clear();
	_clusterInfo.resize(_aas->file->GetNumClusters());

	for (std::size_t i = 0; i < _clusterInfo.size(); i++)
	{
		const aasCluster_t& cluster = _aas->file->GetCluster(static_cast<int>(i));
		
		_clusterInfo[i] = ClusterInfoPtr(new ClusterInfo);
		_clusterInfo[i]->clusterNum = i;
		// Make sure each ClusterInfo structure can hold RouteInfo pointers to every other cluster
		_clusterInfo[i]->routeToCluster.resize(_clusterInfo.size());
	}
}

void tdmEAS::SetupElevatorStationStructures()
{
	_elevatorStations.clear();

	std::size_t numElevatorStations = 0;
	for (int i = 0; i < _elevators.Num(); i++)
	{
		CMultiStateMover* elevator = _elevators[i].GetEntity();
		const idList<MoverPositionInfo>& positionList = elevator->GetPositionInfoList();
		numElevatorStations += positionList.Num();
	}

	_elevatorStations.resize(numElevatorStations);
}

int tdmEAS::GetAreaNumForPosition(const idVec3& position)
{
	int areaNum = _aas->file->PointReachableAreaNum(position, _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);

	// If areaNum could not be determined, try again at a slightly higher position
	if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(position + idVec3(0,0,16), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);
	if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(position + idVec3(0,0,32), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);
	if (areaNum == 0) areaNum = _aas->file->PointReachableAreaNum(position + idVec3(0,0,48), _aas->DefaultSearchBounds(), AREA_REACHABLE_WALK, 0);

	return areaNum;
}

void tdmEAS::AssignElevatorsToClusters()
{
	std::size_t stationIndex = 0;
	for (int i = 0; i < _elevators.Num(); i++)
	{
		CMultiStateMover* elevator = _elevators[i].GetEntity();

		const idList<MoverPositionInfo>& positionList = elevator->GetPositionInfoList();

		for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
		{
			CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();
									
			int areaNum = GetAreaNumForPosition(positionEnt->GetPhysics()->GetOrigin());

			if (areaNum == 0)
			{
				gameLocal.Warning("[%s]: Cannot assign multistatemover position to AAS area:  %s", _aas->name.c_str(), positionEnt->name.c_str());
				continue;
			}

			const aasArea_t& area = _aas->file->GetArea(areaNum);
			_clusterInfo[area.cluster]->numElevatorStations++;

			// Allocate a new ElevatorStationStructure for this station and fill in the data
			_elevatorStations[stationIndex] = ElevatorStationInfoPtr(new ElevatorStationInfo);
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
	// First, find all the reachable elevator stations (for each cluster)
	SetupReachableElevatorStations();

	// At this point, all clusters know their reachable elevator stations (numReachableElevatorStations is set)
	SetupRoutesBetweenClusters();
}

void tdmEAS::SetupRoutesBetweenClusters()
{
	for (std::size_t startCluster = 0; startCluster < _clusterInfo.size(); startCluster++)
	{
		int startArea = _aas->GetAreaInCluster(startCluster);

		if (startArea <= 0) continue;
		
		for (std::size_t goalCluster = 0; goalCluster < _clusterInfo[startCluster]->routeToCluster.size(); goalCluster++)
		{
			// No routes so far, clear the list
			_clusterInfo[startCluster]->routeToCluster[goalCluster].clear();

			if (goalCluster == startCluster) continue;

			int goalArea = _aas->GetAreaInCluster(goalCluster);
			if (goalArea <= 0) continue;

			_routingIterations = 0;
			FindRoutesToCluster(startCluster, startArea, goalCluster, goalArea);
		}
	}

	CondenseRouteInfo();
}

void tdmEAS::CondenseRouteInfo()
{
	// Disregard empty or invalid RouteInfo structures
	for (std::size_t startCluster = 0; startCluster < _clusterInfo.size(); startCluster++)
	{
		for (std::size_t goalCluster = 0; goalCluster < _clusterInfo[startCluster]->routeToCluster.size(); goalCluster++)
		{
			for (RouteInfoList::iterator i = _clusterInfo[startCluster]->routeToCluster[goalCluster].begin();
				 i != _clusterInfo[startCluster]->routeToCluster[goalCluster].end(); /* in-loop increment */)
			{
				RouteInfoPtr& info = *i;
				if (info == NULL || info->routeNodes.empty())
				{
					// clear out empty RouteInfos
					_clusterInfo[startCluster]->routeToCluster[goalCluster].erase(i++);
					continue;
				}
				else
				{
					i++;
				}
			}
		}
	}
}

void tdmEAS::SetupReachableElevatorStations()
{
	for (std::size_t cluster = 0; cluster < _clusterInfo.size(); cluster++)
	{
		// Find an area within that cluster
		int areaNum = _aas->GetAreaInCluster(static_cast<int>(cluster));

		if (areaNum <= 0)
		{
			continue;
		}

		idList<int> elevatorStationIndices;

		// For each cluster, try to setup a route to all elevator stations
		for (std::size_t e = 0; e < _elevatorStations.size(); e++)
		{
			if (_elevatorStations[e] == NULL)
			{
				continue;
			}

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
				//gameLocal.Printf("Cluster %d can reach elevator station %s\n", cluster, _elevatorStations[e]->elevatorPosition.GetEntity()->name.c_str());
				// Add the elevator index to the list
				_clusterInfo[cluster]->reachableElevatorStations.push_back(_elevatorStations[e]);
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

bool tdmEAS::InsertUniqueRouteInfo(int startCluster, int goalCluster, RouteInfoPtr route)
{
	RouteInfoList& routeList = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	for (RouteInfoList::iterator i = routeList.begin(); i != routeList.end(); i++)
	{
		RouteInfoPtr& existing = *i;
		
		if (*route == *existing)
		{
			return false; // Duplicate
		}
	}

	routeList.insert(route);
	return true;
}

void tdmEAS::CleanRouteInfo(int startCluster, int goalCluster)
{
	RouteInfoList& routeList = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	for (RouteInfoList::iterator i = routeList.begin(); i != routeList.end(); /* in-loop increment */)
	{
		if ((*i)->routeNodes.empty())
		{
			routeList.erase(i++);
		}
		else
		{
			i++;
		}
	}
}

RouteInfoList tdmEAS::FindRoutesToCluster(int startCluster, int startArea, int goalCluster, int goalArea)
{
	_routingIterations++;
	DM_LOG(LC_AI, LT_INFO).LogString("EAS routing iteration level = %d\r", _routingIterations);

	/**
	 * greebo: Pseudo-Code:
	 *
	 * 1. Check if we already have routing information to the target cluster, exit if yes
	 * 2. Check if we can walk right up to the target cluster, if yes: fill in the RouteNode and exit
	 * 3. Check all reachable elevator stations and all clusters reachable from there. Go to 1. for each of those.
	 */

	if (startCluster == goalCluster)
	{
		// Do nothing for start == goal
	}
	else if (_clusterInfo[startCluster]->routeToCluster[goalCluster].size() > 0)
	{
		// Routing information to the goal cluster is right there, return it
		DM_LOG(LC_AI, LT_INFO).LogString("Route from cluster %d to %d already exists.\r", startCluster, goalCluster);
	}
	else
	{
		DM_LOG(LC_AI, LT_INFO).LogString("Route from cluster %d to %d doesn't exist yet, check walk path.\r", startCluster, goalCluster);

		// Insert a dummy route into the _clusterInfo matrix, so that we don't come here again
		RouteInfoPtr dummyRoute(new RouteInfo(ROUTE_DUMMY, goalCluster));
		_clusterInfo[startCluster]->routeToCluster[goalCluster].insert(dummyRoute);

		// No routing information, check walk path to the goal cluster
		idReachability* reach;
		int travelTime = 0;
		bool routeFound = _aas->RouteToGoalArea(startArea, _aas->AreaCenter(startArea), 
			goalArea, TFL_WALK|TFL_AIR, travelTime, &reach, NULL);

		if (routeFound) 
		{
			DM_LOG(LC_AI, LT_INFO).LogString("Can walk from cluster %d to %d.\r", startCluster, goalCluster);

			// Walk path possible, allocate a new RouteInfo with a WALK node
			RouteInfoPtr info(new RouteInfo(ROUTE_TO_CLUSTER, goalCluster));

			RouteNodePtr node(new RouteNode(ACTION_WALK, goalArea));
			info->routeNodes.push_back(node);

			// Save this WALK route into the cluster
			InsertUniqueRouteInfo(startCluster, goalCluster, info);
		}
		else 
		{
			DM_LOG(LC_AI, LT_INFO).LogString("Can NOT walk from cluster %d to %d.\r", startCluster, goalCluster);

			// No walk path possible, check all elevator stations that are reachable from this cluster
			for (ElevatorStationInfoList::const_iterator station = _clusterInfo[startCluster]->reachableElevatorStations.begin();
				 station != _clusterInfo[startCluster]->reachableElevatorStations.end(); station++)
			{
				const ElevatorStationInfoPtr& elevatorInfo = *station;

				// Get all stations reachable via this elevator
				const idList<MoverPositionInfo>& positionList = elevatorInfo->elevator.GetEntity()->GetPositionInfoList();

				// This will be a set of area candidates to spread out the pathing
				idList<int> areaCandidates;
				// Add the source area first, this one will be skipped
				areaCandidates.Append(elevatorInfo->areaNum);

				// Add possible candidates
				for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
				{
					CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();
					areaCandidates.AddUnique(GetAreaNumForPosition(positionEnt->GetPhysics()->GetOrigin()));
				}

				DM_LOG(LC_AI, LT_INFO).LogString("Found %d elevator stations reachable from cluster %d (goal cluster = %d).\r", areaCandidates.Num(), startCluster, goalCluster);

				// Now look at all elevator floors and route from there (but skip the first one, which is the source area)
				for (int i = 1; i < areaCandidates.Num(); i++)
				{
					int nextArea = areaCandidates[i];
					if (nextArea <= 0) continue;

					int nextCluster = _aas->file->GetArea(nextArea).cluster;

					DM_LOG(LC_AI, LT_INFO).LogString("Checking elevator station %d reachable from cluster %d (goal cluster = %d).\r", i, startCluster, goalCluster);

					if (nextCluster == goalCluster)
					{
						DM_LOG(LC_AI, LT_INFO).LogString("Elevator leads right to the target cluster %d.\r", goalCluster);
						// Hooray, the elevator leads right to the goal cluster, write that down
						RouteInfoPtr info(new RouteInfo(ROUTE_TO_CLUSTER, goalCluster));
						info->routeNodes.push_back(RouteNodePtr(new RouteNode(ACTION_WALK, elevatorInfo->areaNum)));
						info->routeNodes.push_back(RouteNodePtr(new RouteNode(ACTION_USE_ELEVATOR, nextArea)));

						// Save this USE_ELEVATOR route into this startCluster
						InsertUniqueRouteInfo(startCluster, goalCluster, info);
					}
					else 
					{
						DM_LOG(LC_AI, LT_INFO).LogString("Investigating route to target cluster %d, starting from station cluster %d.\r", goalCluster, nextCluster);
						// The elevator station does not start in the goal cluster, find a way from there
						RouteInfoList routes = FindRoutesToCluster(nextCluster, nextArea, goalCluster, goalArea);

						for (RouteInfoList::iterator i = routes.begin(); i != routes.end(); i++)
						{
							RouteInfoPtr& route = *i;
							if (route->routeNodes.empty()) continue;

							// Append the valid route objects to the existing chain
							InsertUniqueRouteInfo(startCluster, goalCluster, *i);
						}
					}
				}
			}
		}
	}

	// Purge all empty RouteInfo nodes
	CleanRouteInfo(startCluster, goalCluster);

	// Keep one dummy node anyway, to signal that we already traversed this combination
	if (_clusterInfo[startCluster]->routeToCluster[goalCluster].empty())
	{
		RouteInfoPtr dummyRoute(new RouteInfo(ROUTE_DUMMY, goalCluster));
		_clusterInfo[startCluster]->routeToCluster[goalCluster].insert(dummyRoute);
	}

	assert(_routingIterations > 0);
	_routingIterations--;

	return _clusterInfo[startCluster]->routeToCluster[goalCluster];
}

void tdmEAS::DrawRoute(int startArea, int goalArea)
{
	int startCluster = _aas->file->GetArea(startArea).cluster;
	int goalCluster = _aas->file->GetArea(goalArea).cluster;

	if (startCluster < 0 || goalCluster < 0)
	{
		gameLocal.Warning("Cannot draw route, cluster numbers < 0.\r");
		return;
	}

	const RouteInfoList& routes = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	// Draw all routes to the target area
	for (RouteInfoList::const_iterator r = routes.begin(); r != routes.end(); r++)
	{
		const RouteInfoPtr& route = *r;

		RouteNodePtr prevNode(new RouteNode(ACTION_WALK, startArea));
		
		for (RouteNodeList::const_iterator n = route->routeNodes.begin(); n != route->routeNodes.end(); n++)
		{
			RouteNodePtr node = *n;

			if (prevNode != NULL)
			{
				idVec4 colour = (node->type == ACTION_WALK) ? colorBlue : colorCyan;
				idVec3 start = _aas->file->GetArea(node->index).center;
				idVec3 end = _aas->file->GetArea(prevNode->index).center;
				gameRenderWorld->DebugArrow(colour, start, end, 1, 5000);
			}

			prevNode = node;
		}
	}
}
