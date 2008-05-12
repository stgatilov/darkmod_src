/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-12 18:53:28 +0200 (Mo, 12 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_EAS_CLUSTERINFO_H__
#define __AI_EAS_CLUSTERINFO_H__

#include "ElevatorStationInfo.h"
#include "RouteInfo.h"
#include <boost/shared_ptr.hpp>

namespace eas {

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

} // namespace eas

#endif /* __AI_EAS_CLUSTERINFO_H__ */
