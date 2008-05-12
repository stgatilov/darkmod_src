/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-12 18:53:28 +0200 (Mo, 12 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_EAS_ELEVATOR_STATION_INFO_H__
#define __AI_EAS_ELEVATOR_STATION_INFO_H__

#include <list>
#include <boost/shared_ptr.hpp>
#include "RouteNode.h"
#include "../../../game/game_local.h"

namespace eas {

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

	void Save(idSaveGame* savefile) const
	{
		elevator.Save(savefile);
		elevatorPosition.Save(savefile);

		savefile->WriteInt(areaNum);
		savefile->WriteInt(clusterNum);
		savefile->WriteInt(elevatorNum);
	}

	void Restore(idRestoreGame* savefile)
	{
		elevator.Restore(savefile);
		elevatorPosition.Restore(savefile);

		savefile->ReadInt(areaNum);
		savefile->ReadInt(clusterNum);
		savefile->ReadInt(elevatorNum);
	}
};
typedef boost::shared_ptr<ElevatorStationInfo> ElevatorStationInfoPtr;
typedef std::list<ElevatorStationInfoPtr> ElevatorStationInfoList;

} // namespace eas

#endif /* __AI_EAS_ELEVATOR_STATION_INFO_H__ */
