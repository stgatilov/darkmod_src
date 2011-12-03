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

#include <map>
#include "Download.h"

/**
 * The class handling the actual mission downloads.
 */
class CDownloadManager
{
private:
	// Ongoing downloads
	typedef std::map<int, CDownloadPtr> Downloads;
	Downloads _downloads;

	int _nextAvailableId;

	bool _allDownloadsDone;

public:
	CDownloadManager();

	void ProcessDownloads();

	void ClearDownloads();

	int AddDownload(const CDownloadPtr& download);
	void RemoveDownload(int id);

	CDownloadPtr GetDownload(int id);

	// Returns true if there is a download already in progress
	bool DownloadInProgress();
};
typedef boost::shared_ptr<CDownloadManager> CDownloadManagerPtr;

#endif /* _MISSION_DOWNLOAD_MANAGER_H_ */
