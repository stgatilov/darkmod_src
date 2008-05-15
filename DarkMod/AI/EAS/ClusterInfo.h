/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
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

	// Save/Restore routines
	void Save(idSaveGame* savefile) const
	{
		savefile->WriteInt(clusterNum);
		savefile->WriteUnsignedInt(static_cast<unsigned int>(numElevatorStations));

		savefile->WriteInt(static_cast<int>(reachableElevatorStations.size()));
		for (ElevatorStationInfoList::const_iterator i = reachableElevatorStations.begin();
			 i != reachableElevatorStations.end(); i++)
		{
			(*i)->Save(savefile);
		}

		savefile->WriteInt(static_cast<int>(routeToCluster.size()));
		for (std::size_t i = 0; i < routeToCluster.size(); i++)
		{
			const RouteInfoList& list = routeToCluster[i];
			savefile->WriteInt(static_cast<int>(list.size()));
			for (RouteInfoList::const_iterator j = list.begin(); j != list.end(); j++)
			{
				(*j)->Save(savefile);
			}
		}
	}

	void Restore(idRestoreGame* savefile)
	{
		savefile->ReadInt(clusterNum);
		unsigned int temp;
		savefile->ReadUnsignedInt(temp);
		numElevatorStations = static_cast<unsigned short>(temp);

		int num;
		savefile->ReadInt(num);
		reachableElevatorStations.clear();
		for (int i = 0; i < num; i++)
		{
			ElevatorStationInfoPtr info(new ElevatorStationInfo);
			info->Restore(savefile);
			reachableElevatorStations.push_back(info);
		}

		savefile->ReadInt(num);
		routeToCluster.resize(num);
		for (int i = 0; i < num; i++)
		{
			RouteInfoList& list = routeToCluster[i];
			list.clear();

			int num2;
			savefile->ReadInt(num2);
			for (int j = 0; j < num2; j++)
			{
				RouteInfoPtr routeInfo(new RouteInfo);
				routeInfo->Restore(savefile);
				list.push_back(routeInfo);
			}
		}
	}
};
typedef boost::shared_ptr<ClusterInfo> ClusterInfoPtr;

} // namespace eas

#endif /* __AI_EAS_CLUSTERINFO_H__ */
