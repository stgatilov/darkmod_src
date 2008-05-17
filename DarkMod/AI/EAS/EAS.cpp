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

#include "EAS.h"

namespace eas {

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
			_elevatorStations[stationIndex]->elevatorNum = i;

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

	// Remove all dummy routes
	CondenseRouteInfo();

	// Sort the remaining routes (number of hops is the key)
	SortRoutes();
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
}

void tdmEAS::SortRoutes()
{
	for (std::size_t startCluster = 0; startCluster < _clusterInfo.size(); startCluster++)
	{
		for (std::size_t goalCluster = 0; goalCluster < _clusterInfo[startCluster]->routeToCluster.size(); goalCluster++)
		{
			SortRoute(startCluster, goalCluster);
		}
	}
}

void tdmEAS::SortRoute(int startCluster, int goalCluster)
{
	// Use a std::map to sort the Routes after their number of "hops"
	typedef std::map<std::size_t, RouteInfoPtr> RouteSortMap;

	RouteInfoList& routeList = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	RouteSortMap sorted;

	// Insert the routeInfo structures, using the hop size as index
	for (RouteInfoList::const_iterator i = routeList.begin(); i != routeList.end(); i++)
	{
		sorted.insert(RouteSortMap::value_type(
			(*i)->routeNodes.size(), // number of hops
			(*i)					 // RouteInfoPtr
		));
	}

	// Wipe the unsorted list
	routeList.clear();

	// Re-insert the items
	for (RouteSortMap::const_iterator i = sorted.begin(); i != sorted.end(); i++)
	{
		routeList.push_back(i->second);
	}
}

void tdmEAS::CondenseRouteInfo()
{
	// Disregard empty or invalid RouteInfo structures
	for (std::size_t startCluster = 0; startCluster < _clusterInfo.size(); startCluster++)
	{
		for (std::size_t goalCluster = 0; goalCluster < _clusterInfo[startCluster]->routeToCluster.size(); goalCluster++)
		{
			CleanRouteInfo(startCluster, goalCluster);
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
	// Elevators
	savefile->WriteInt(_elevators.Num());
	for (int i = 0; i < _elevators.Num(); i++)
	{
		_elevators[i].Save(savefile);
	}

	// ClusterInfos
	savefile->WriteInt(static_cast<int>(_clusterInfo.size()));
	for (std::size_t i = 0; i < _clusterInfo.size(); i++)
	{
		_clusterInfo[i]->Save(savefile);
	}

	// ElevatorStations
	savefile->WriteInt(static_cast<int>(_elevatorStations.size()));
	for (std::size_t i = 0; i < _elevatorStations.size(); i++)
	{
		_elevatorStations[i]->Save(savefile);
	}
}

void tdmEAS::Restore(idRestoreGame* savefile)
{
	int num;

	// Elevators
	savefile->ReadInt(num);
	_elevators.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		_elevators[i].Restore(savefile);
	}

	// Cluster Infos
	savefile->ReadInt(num);
	_clusterInfo.resize(num);
	for (int i = 0; i < num; i++)
	{
		_clusterInfo[i] = ClusterInfoPtr(new ClusterInfo);
		_clusterInfo[i]->Restore(savefile);
	}

	// ElevatorStations
	_elevatorStations.clear();
	savefile->ReadInt(num);
	_elevatorStations.resize(num);
	for (int i = 0; i < num; i++)
	{
		_elevatorStations[i] = ElevatorStationInfoPtr(new ElevatorStationInfo);
		_elevatorStations[i]->Restore(savefile);
	}
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

	routeList.push_back(route);
	return true;
}

void tdmEAS::CleanRouteInfo(int startCluster, int goalCluster)
{
	RouteInfoList& routeList = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	for (RouteInfoList::iterator i = routeList.begin(); i != routeList.end(); /* in-loop increment */)
	{
		if ((*i)->routeNodes.empty() || (*i)->routeType == ROUTE_DUMMY)
		{
			routeList.erase(i++);
		}
		else
		{
			i++;
		}
	}
}

ElevatorStationInfoPtr tdmEAS::GetElevatorStationInfo(int index)
{
	if (index >= 0 || index < static_cast<int>(_elevatorStations.size())) 
	{
		return _elevatorStations[static_cast<std::size_t>(index)];
	}
	else
	{
		return ElevatorStationInfoPtr();
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
		_clusterInfo[startCluster]->routeToCluster[goalCluster].push_back(dummyRoute);

		// No routing information, check walk path to the goal cluster
		idReachability* reach;
		int travelTime = 0;
		// Workaround: Include the TFL_INVALID flag to include deactivated AAS areas
		bool routeFound = _aas->RouteToGoalArea(startArea, _aas->AreaCenter(startArea), 
			goalArea, TFL_WALK|TFL_AIR|TFL_INVALID, travelTime, &reach, NULL);

		if (routeFound) 
		{
			DM_LOG(LC_AI, LT_INFO).LogString("Can walk from cluster %d to %d.\r", startCluster, goalCluster);

			// Walk path possible, allocate a new RouteInfo with a WALK node
			RouteInfoPtr info(new RouteInfo(ROUTE_TO_CLUSTER, goalCluster));

			RouteNodePtr node(new RouteNode(ACTION_WALK, goalArea, goalCluster));
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
				int startStationIndex = GetElevatorStationIndex(elevatorInfo);

				// Get all stations reachable via this elevator
				const idList<MoverPositionInfo>& positionList = elevatorInfo->elevator.GetEntity()->GetPositionInfoList();

				DM_LOG(LC_AI, LT_INFO).LogString("Found %d elevator stations reachable from cluster %d (goal cluster = %d).\r", positionList.Num(), startCluster, goalCluster);

				// Now look at all elevator floors and route from there
				for (int positionIdx = 0; positionIdx < positionList.Num(); positionIdx++)
				{
					CMultiStateMoverPosition* positionEnt = positionList[positionIdx].positionEnt.GetEntity();

					if (positionEnt == elevatorInfo->elevatorPosition.GetEntity())
					{
						continue; // this is the same position we are starting from, skip that
					}

					int targetStationIndex = GetElevatorStationIndex(positionEnt);

					int nextArea = GetAreaNumForPosition(positionEnt->GetPhysics()->GetOrigin());
					if (nextArea <= 0) continue;

					int nextCluster = _aas->file->GetArea(nextArea).cluster;

					DM_LOG(LC_AI, LT_INFO).LogString("Checking elevator station %d reachable from cluster %d (goal cluster = %d).\r", startStationIndex, startCluster, goalCluster);

					if (nextCluster == goalCluster)
					{
						// Hooray, the elevator leads right to the goal cluster, write that down
						RouteInfoPtr info(new RouteInfo(ROUTE_TO_CLUSTER, goalCluster));

						// Walk to the elevator station and use it
						RouteNodePtr walkNode(new RouteNode(ACTION_WALK, elevatorInfo->areaNum, elevatorInfo->clusterNum, elevatorInfo->elevatorNum, startStationIndex));
						// Use the elevator to reach the elevator station in the next cluster
						RouteNodePtr useNode(new RouteNode(ACTION_USE_ELEVATOR, nextArea, nextCluster, elevatorInfo->elevatorNum, targetStationIndex));

						info->routeNodes.push_back(walkNode);
						info->routeNodes.push_back(useNode);

						// Save this USE_ELEVATOR route into this startCluster
						InsertUniqueRouteInfo(startCluster, goalCluster, info);

						DM_LOG(LC_AI, LT_INFO).LogString("Elevator leads right to the target cluster %d.\r", goalCluster);
					}
					else 
					{
						DM_LOG(LC_AI, LT_INFO).LogString("Investigating route to target cluster %d, starting from station cluster %d.\r", goalCluster, nextCluster);
						// The elevator station does not start in the goal cluster, find a way from there
						RouteInfoList routes = FindRoutesToCluster(nextCluster, nextArea, goalCluster, goalArea);

						for (RouteInfoList::iterator i = routes.begin(); i != routes.end(); i++)
						{
							RouteInfoPtr& foundRoute = *i;

							// Evaluate the suggested route (check for redundancies)
							if (!EvaluateRoute(startCluster, goalCluster, elevatorInfo->elevatorNum, foundRoute))
							{
								continue;
							}

							// Route was accepted, copy it
							RouteInfoPtr newRoute(new RouteInfo(*foundRoute));

							// Append the valid route objects to the existing chain, but add a "walk to elevator station" to the front
							RouteNodePtr useNode(new RouteNode(ACTION_USE_ELEVATOR, nextArea, nextCluster, elevatorInfo->elevatorNum, targetStationIndex));
							RouteNodePtr walkNode(new RouteNode(ACTION_WALK, elevatorInfo->areaNum, elevatorInfo->clusterNum, elevatorInfo->elevatorNum, startStationIndex));

							newRoute->routeNodes.push_front(useNode);
							newRoute->routeNodes.push_front(walkNode);
							
							// Add the compiled information to our repository
							InsertUniqueRouteInfo(startCluster, goalCluster, newRoute);
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
		_clusterInfo[startCluster]->routeToCluster[goalCluster].push_back(dummyRoute);
	}

	assert(_routingIterations > 0);
	_routingIterations--;

	return _clusterInfo[startCluster]->routeToCluster[goalCluster];
}

bool tdmEAS::EvaluateRoute(int startCluster, int goalCluster, int forbiddenElevator, RouteInfoPtr route)
{
	// Don't regard empty or dummy routes
	if (route == NULL || route->routeType == ROUTE_DUMMY || route->routeNodes.empty()) 
	{
		return false;
	}

	// Does the route come across our source cluster?
	for (RouteNodeList::const_iterator node = route->routeNodes.begin(); 
		 node != route->routeNodes.end(); node++)
	{
		if ((*node)->toCluster == startCluster || (*node)->elevator == forbiddenElevator)
		{
			// Route is coming across the same cluster as we started, this is a loop, reject
			return false;
		}
	}
	
	return true; // accepted
}

int tdmEAS::GetElevatorIndex(CMultiStateMover* mover)
{
	for (int i = 0; i < _elevators.Num(); i++)
	{
		if (_elevators[i].GetEntity() == mover)
		{
			return i; // found!
		}
	}

	return -1; // not found
}

int tdmEAS::GetElevatorStationIndex(ElevatorStationInfoPtr info)
{
	for (std::size_t i = 0; i < _elevatorStations.size(); i++)
	{
		if (_elevatorStations[i] == info)
		{
			return static_cast<int>(i); // found!
		}
	}

	return -1;
}

int tdmEAS::GetElevatorStationIndex(CMultiStateMoverPosition* positionEnt)
{
	for (std::size_t i = 0; i < _elevatorStations.size(); i++)
	{
		if (_elevatorStations[i] != NULL && _elevatorStations[i]->elevatorPosition.GetEntity() == positionEnt)
		{
			return static_cast<int>(i); // found!
		}
	}

	return -1;
}

bool tdmEAS::FindRouteToGoal(aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin, int travelFlags, idActor* actor) 
{
	assert(_aas != NULL);
	int startCluster = _aas->file->GetArea(areaNum).cluster;
	int goalCluster = _aas->file->GetArea(goalAreaNum).cluster;

	// Check if we are starting from a portal
	if (startCluster < 0)
	{
		startCluster = _aas->file->GetPortal(-startCluster).clusters[0];
	}

	// Check if we are going to a portal
	if (goalCluster < 0)
	{
		goalCluster = _aas->file->GetPortal(-goalCluster).clusters[0];
	}

	const RouteInfoList& routes = _clusterInfo[startCluster]->routeToCluster[goalCluster];

	// Draw all routes to the target area
	if (!routes.empty())
	{
		const RouteInfoPtr& route = *routes.begin();

		// Notify the AI that it needs to use an elevator
		actor->NeedToUseElevator(route);
		path.moveGoal = goalOrigin;
		path.moveAreaNum = goalAreaNum;
		return true;
	}

	return false;
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

		RouteNodePtr prevNode(new RouteNode(ACTION_WALK, startArea, startCluster));
		
		for (RouteNodeList::const_iterator n = route->routeNodes.begin(); n != route->routeNodes.end(); n++)
		{
			RouteNodePtr node = *n;

			if (prevNode != NULL)
			{
				idVec4 colour = (node->type == ACTION_WALK) ? colorBlue : colorCyan;
				idVec3 start = _aas->file->GetArea(node->toArea).center;
				idVec3 end = _aas->file->GetArea(prevNode->toArea).center;
				gameRenderWorld->DebugArrow(colour, start, end, 1, 5000);
			}

			prevNode = node;
		}
	}
}

} // namespace eas
