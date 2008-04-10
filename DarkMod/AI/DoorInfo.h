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

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);

	void Clear();
};
typedef boost::shared_ptr<DoorInfo> DoorInfoPtr;

} // namespace ai

#endif /* __DOOR_INFO_H__ */
