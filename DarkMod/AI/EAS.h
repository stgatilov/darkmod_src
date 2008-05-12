/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-11 18:53:28 +0200 (Su, 11 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_EAS_H__
#define __AI_EAS_H__

#include "../idlib/precompiled.h"
#include "../MultiStateMover.h"
#include "../../game/ai/aas_local.h"
#include <set>
#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>

class idAASLocal;

enum RouteType {
	ROUTE_DUMMY = 0,	// placeholder
	ROUTE_TO_AREA,		// a route to an AAS area
	ROUTE_TO_CLUSTER,	// a route to an AAS cluster
	NUM_ROUTE_TYPES,
};

enum ActionType {
	ACTION_WALK = 0,		// AI just needs to walk to the target
	ACTION_USE_ELEVATOR,	// AI needs to use an elevator
	NUM_ACTIONS,
};

struct RouteNode
{
	ActionType type;		// what needs to be done in this route section (walk?, use elevator?)
	int toArea;				// the target AAS area number
	int toCluster;			// the target AAS cluster number
	int elevator;			// the elevator number (is -1 if no elevator to be used in this node)

	RouteNode() :
		type(ACTION_WALK),
		toArea(0),
		toCluster(0),
		elevator(-1)
	{}

	RouteNode(ActionType t, int goalArea, int goalCluster, int elevatorNum = -1) :
		type(t),
		toArea(goalArea),
		toCluster(goalCluster),
		elevator(elevatorNum)
	{}

	// Copy constructor
	RouteNode(const RouteNode& other) :
		type(other.type),
		toArea(other.toArea),
		toCluster(other.toCluster),
		elevator(other.elevator)
	{}

	bool operator==(const RouteNode& other) const
	{
		return (type == other.type && toArea == other.toArea && toCluster == other.toCluster && elevator == other.elevator);
	}

	bool operator!=(const RouteNode& other) const
	{
		return !operator==(other);
	}
};
typedef boost::shared_ptr<RouteNode> RouteNodePtr;
typedef std::list<RouteNodePtr> RouteNodeList;

// A route info contains information of how to get to a specific target
struct RouteInfo
{
	RouteType routeType;		// ROUTE_TO_AREA or ROUTE_TO_CLUSTER, ...
	int target;					// either the target AREA or the target CLUSTER number, depending on routeType
	RouteNodeList routeNodes;	// contains the actual route node chain (WALK, USE_ELEVATOR, WALK, etc.)

	RouteInfo() :
		routeType(ROUTE_TO_CLUSTER),
		target(-1)
	{}

	RouteInfo(RouteType type, int targetNum) :
		routeType(type),
		target(targetNum)
	{}

	// Copy constructor
	RouteInfo(const RouteInfo& other) :
		routeType(other.routeType),
		target(other.target)
	{
		// Copy the RouteNodes of the other list, one by one
		for (RouteNodeList::const_iterator otherNode = other.routeNodes.begin();
			otherNode != other.routeNodes.end(); otherNode++)
		{
			RouteNodePtr newNode(new RouteNode(**otherNode));
			routeNodes.push_back(newNode);
		}
	}

	bool operator==(const RouteInfo& other) const
	{
		if (routeType == other.routeType && target == other.target && routeNodes.size() == other.routeNodes.size())
		{
			for (RouteNodeList::const_iterator i = routeNodes.begin(), j = other.routeNodes.begin(); i != routeNodes.end(); i++, j++)
			{
				if (*i != *j)
				{
					return false; // RouteNode mismatch
				}
			}

			return true; // everything matched
		}

		return false; // routeType, routeNodes.size() or target mismatched
	}

	bool operator!=(const RouteInfo& other) const
	{
		return !operator==(other);
	}
};
typedef boost::shared_ptr<RouteInfo> RouteInfoPtr;
typedef std::list<RouteInfoPtr> RouteInfoList;
typedef std::vector<RouteInfoList> RouteInfoListVector;

struct ElevatorStationInfo
{
	idEntityPtr<CMultiStateMover> elevator;					// The elevator this station is belonging to
	idEntityPtr<CMultiStateMoverPosition> elevatorPosition;	// The elevator position entity
	int areaNum;											// The area number of this elevator station
	int clusterNum;											// The cluster number of this elevator station
	int elevatorNum;										// The elevator number this position is belonging to

	ElevatorStationInfo() :
		areaNum(-1),
		clusterNum(-1),
		elevatorNum(-1)
	{
		elevator = NULL;
		elevatorPosition = NULL;
	}
};
typedef boost::shared_ptr<ElevatorStationInfo> ElevatorStationInfoPtr;
typedef std::list<ElevatorStationInfoPtr> ElevatorStationInfoList;

struct ClusterInfo 
{
	int clusterNum;										// The number of this cluster
	unsigned short numElevatorStations;				// the number of elevator stations in this cluster
	ElevatorStationInfoList reachableElevatorStations;	// references to the reachable elevator stations
	RouteInfoListVector routeToCluster;					// for each cluster, a std::list of possible routes (can be empty)

	ClusterInfo() :
		clusterNum(-1),
		numElevatorStations(0)
	{}
};
typedef boost::shared_ptr<ClusterInfo> ClusterInfoPtr;

/**
 * greebo: The EAS ("Elevator Awareness System") provides some extended
 * routing functionality between AAS clusters for all AI which are able
 * to handle elevators (i.e. travelFlag TFL_ELEVATOR set to 1).
 *
 * This class is tightly bound to its owning idAASLocal class and is always
 * constructed and destructed along with it.
 */
class tdmEAS
{
	// The owning AAS
	idAASLocal* _aas;

	// The list of elevators
	idList< idEntityPtr<CMultiStateMover> > _elevators;

	// The array of ClusterInfoStructures
	typedef std::vector<ClusterInfoPtr> ClusterInfoVector;
	ClusterInfoVector _clusterInfo;

	// An array of dynamically allocated elevator stations
	typedef std::vector<ElevatorStationInfoPtr> ElevatorStationVector;
	ElevatorStationVector _elevatorStations;

	// Temporary calculation variables, don't need to be saved
	mutable int _routingIterations;

public:
	// Initialise the EAS with a valid AAS reference
	tdmEAS(idAASLocal* aas);

	/**
	 * greebo: Computes the routing tables for the elevators.
	 * All elevators must have been added beforehand using AddElevator().
	 */
	void Compile();

	// Clears all data
	void Clear();

	/**
	 * greebo: Adds a new elevator to the EAS.
	 */
	void AddElevator(CMultiStateMover* mover);

	// Save/Restore routines
	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	void DrawRoute(int startArea, int goalArea);

private:
	void SetupClusterInfoStructures();
	void SetupElevatorStationStructures();

	void AssignElevatorsToClusters();

	void SetupClusterRouting();
	void SetupReachableElevatorStations();
	void SetupRoutesBetweenClusters();

	// Removes all empty and dummy routes
	void CondenseRouteInfo();

	// Sorts the existing routes (fastest first)
	void SortRoutes();
	void SortRoute(int startCluster, int goalCluster);

	// Retrieves the internal index of the given mover (or -1 if the mover is not registered)
	int GetElevatorIndex(CMultiStateMover* mover);

	// Calculates all possible routes from the startCluster/startArea to goalCluster/goalArea 
	RouteInfoList FindRoutesToCluster(int startCluster, int startArea, int goalCluster, int goalArea);

	// Returns the AAS area number for the given position
	int GetAreaNumForPosition(const idVec3& position);

	// Inserts the routeInfo for startCluster => goalCluster, but checks for duplicates, returns TRUE if inserted
	bool InsertUniqueRouteInfo(int startCluster, int goalCluster, RouteInfoPtr route);

	// Removes empty routeInfo structures
	void CleanRouteInfo(int startCluster, int goalCluster);

	// Checks the route for redundancies, returns TRUE if the route is accepted
	bool EvaluateRoute(int startCluster, int goalCluster, int forbiddenElevator, RouteInfoPtr route);
};

#endif /* __AI_EAS_H__ */
