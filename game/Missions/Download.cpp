/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Download.h"
#include "../Http/HttpConnection.h"
#include "../ZipLoader/ZipLoader.h"
#include "MissionManager.h"

CDownload::CDownload(const idStr& url, const idStr& destFilename, bool enablePK4check) :
	_curUrl(0),
	_destFilename(destFilename),
	_status(NOT_STARTED_YET),
	_pk4CheckEnabled(enablePK4check),
	_relatedDownload(-1)
{
	_urls.Append(url);
}

CDownload::CDownload(const idStringList& urls, const idStr& destFilename, bool enablePK4check) :
	_urls(urls),
	_curUrl(0),
	_destFilename(destFilename),
	_status(NOT_STARTED_YET),
	_pk4CheckEnabled(enablePK4check),
	_relatedDownload(-1)
{}

CDownload::~CDownload()
{
	Stop(false);
}

CDownload::DownloadStatus CDownload::GetStatus()
{
	return _status;
}

void CDownload::Start()
{
	if (_status != NOT_STARTED_YET)
	{
		return; // don't allow double starts
	}

	// Construct the temporary filename
	idStr filename;
	_destFilename.ExtractFileName(filename);
	_destFilename.ExtractFilePath(_tempFilename);

	// /path/to/fms/_filename.pk4 (including underscore)
	_tempFilename += "_" + filename;
	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Downloading to temporary file %s.\r", _tempFilename.c_str());

	_status = IN_PROGRESS;
	_thread = ThreadPtr(new std::thread(std::bind(&CDownload::Perform, this)));
}

void CDownload::Stop(bool canceled)
{
	if (_thread != NULL && _request != NULL)
	{
		// Set the URL index beyond the list size to prevent 
		// the worker thread from proceeding to the next URL
		_curUrl = _urls.Num();

		// Cancel the request
		_request->Cancel();

		// Wait for the thread to finish
		_thread->join();

		_thread.reset();
		_request.reset();

		// Don't reset successful stati
		if (_status != SUCCESS)
		{
			_status = FAILED;
		}
		if (canceled){
			_status = CANCELED;
		}

		// Remove temporary file
		CMissionManager::DoRemoveFile(_tempFilename.c_str());
	}
}

double CDownload::GetProgressFraction()
{
	return _request != NULL ? _request->GetProgressFraction() : 0.0;
}

void CDownload::EnableValidPK4Check(bool enable)
{
	_pk4CheckEnabled = enable;
}

int CDownload::GetRelatedDownloadId()
{
	return _relatedDownload;
}

void CDownload::SetRelatedDownloadId(int relatedDownloadId)
{
	_relatedDownload = relatedDownloadId;
}

void CDownload::Perform()
{
	while (_curUrl < _urls.Num())
	{
		// Remove any previous temporary file
		CMissionManager::DoRemoveFile(_tempFilename.c_str());

		const idStr& url = _urls[_curUrl];

		// Create a new request
		_request = gameLocal.m_HttpConnection->CreateRequest(url.c_str(), _tempFilename.c_str());
	
		// Start the download, blocks until finished or aborted
		_request->Perform();

		if (_request->GetStatus() == CHttpRequest::OK)
		{
			// Check the downloaded file
			if (_pk4CheckEnabled)
			{
				bool valid = CheckValidPK4(_tempFilename);

				if (!valid)
				{
					_curUrl++;
					continue;
				}
			}

			// Make sure the destination file is overwritten
			if (fs::exists(_destFilename.c_str()))
			{
				CMissionManager::DoRemoveFile(_destFilename.c_str());
			}

			// Move temporary file to the real one
			if (CMissionManager::DoMoveFile(_tempFilename.c_str(), _destFilename.c_str()))
			{
				_status = SUCCESS;
			}
			else
			{
				// Move failed
				_status = FAILED;
			}

			// Download succeeded, exit the loop
			break;
		}
		else 
		{
			// Download error
			if (_request->GetStatus() == CHttpRequest::ABORTED)
			{
				DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Download from '%s' aborted.", url.c_str());
			}
			else
			{
				DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Connection Error (status = %i) for URL '%s'.", _request->GetStatus(), url.c_str());
			}

			// Proceed to the next URL
			_curUrl++;
		}
	} // while

	// Have we run out of URLs
	if (_curUrl >= _urls.Num())
	{
		// This was our last URL, set the status to FAILED
		_status = FAILED;
	}
}

bool CDownload::CheckValidPK4(const idStr& path)
{
	CZipFilePtr zipFile = CZipLoader::Instance().OpenFile(path);

	return (zipFile != NULL);
}
