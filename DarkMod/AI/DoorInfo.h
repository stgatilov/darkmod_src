/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: angua $
 *
 ***************************************************************************/

#ifndef __DOOR_INFO_H__
#define __DOOR_INFO_H__

#include <boost/shared_ptr.hpp>

namespace ai
{

struct DoorInfo
{
	// When this door was seen the last time (-1 == never)
	int lastTimeSeen;

	// The last time this door was attempted to be opened (-1 == never)
	int lastTimeTriedToOpen;

	// Whether this door was open the last time it was seen
	bool wasOpen;

	// Whether this door was locked at the last open attempt
	bool wasLocked;

	// Whether this door was blocked at the last open attempt
	bool wasBlocked;

	DoorInfo() :
		lastTimeSeen(-1),
		lastTimeTriedToOpen(-1),
		wasOpen(false),
		wasLocked(false),
		wasBlocked(false)
	{}

	void Save(idSaveGame* savefile) const
	{
		savefile->WriteInt(lastTimeSeen);
		savefile->WriteInt(lastTimeTriedToOpen);
		savefile->WriteBool(wasOpen);
		savefile->WriteBool(wasLocked);
		savefile->WriteBool(wasBlocked);
	}

	void Restore(idRestoreGame* savefile)
	{
		savefile->ReadInt(lastTimeSeen);
		savefile->ReadInt(lastTimeTriedToOpen);
		savefile->ReadBool(wasOpen);
		savefile->ReadBool(wasLocked);
		savefile->ReadBool(wasBlocked);
	}
};
typedef boost::shared_ptr<DoorInfo> DoorInfoPtr;

} // namespace ai

#endif /* __DOOR_INFO_H__ */
