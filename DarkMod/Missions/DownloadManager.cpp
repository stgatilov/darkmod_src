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

#include "DownloadManager.h"

void CDownloadManager::AddDownload(const CDownloadPtr& download)
{
	_downloads.push_back(download);

	// Go ahead and start the download (TODO)
	download->Start();
}

bool CDownloadManager::DownloadInProgress()
{
	return false;
}
