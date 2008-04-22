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
	// A unique ID of this object (needed for saving and restoring)
	int id;

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

	DoorInfo();

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

private:
	static int highestId;
};
typedef boost::shared_ptr<DoorInfo> DoorInfoPtr;

} // namespace ai

#endif /* __DOOR_INFO_H__ */
