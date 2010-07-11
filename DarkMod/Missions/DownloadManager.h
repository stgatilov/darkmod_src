/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _MISSION_DOWNLOAD_MANAGER_H_
#define _MISSION_DOWNLOAD_MANAGER_H_

#include <list>
#include "Download.h"

/**
 * The class handling the actual mission downloads.
 */
class CDownloadManager
{
private:
	// Ongoing downloads
	typedef std::list<CDownloadPtr> Downloads;
	Downloads _downloads;

public:
	void AddDownload(const CDownloadPtr& download);

	// Returns true if there is a download already in progress
	bool DownloadInProgress();
};
typedef boost::shared_ptr<CDownloadManager> CDownloadManagerPtr;

#endif /* _MISSION_DOWNLOAD_MANAGER_H_ */
