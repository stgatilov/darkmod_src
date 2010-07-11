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

#include "Download.h"
#include "../Http/HttpConnection.h"

#include <boost/bind.hpp>

CDownload::CDownload(const idStr& url, const idStr& destFilename) :
	_url(url),
	_destFilename(destFilename),
	_status(NOT_STARTED_YET)
{}

CDownload::~CDownload()
{
	Stop();
}

CDownload::Status CDownload::GetStatus()
{
	return _status;
}

void CDownload::Start()
{
	if (_status != NOT_STARTED_YET)
	{
		return; // don't allow double starts
	}

	_status = IN_PROGRESS;
	_thread = ThreadPtr(new boost::thread(boost::bind(&CDownload::Perform, this)));
}

void CDownload::Stop()
{
	if (_thread != NULL && _request != NULL)
	{
		// Cancel the request
		_request->Cancel();

		// Wait for the thread to finish
		_thread->join();

		_thread.reset();
		_request.reset();

		_status = FAILED;
	}
}

double CDownload::GetProgressFraction()
{
	return _request != NULL ? _request->GetProgressFraction() : 0.0;
}

void CDownload::Perform()
{
	_request = gameLocal.m_HttpConnection->CreateRequest(_url.c_str(), _destFilename.c_str());

	_request->Perform(); // blocks until finished or aborted

	if (_request->GetStatus() != CHttpRequest::OK)
	{
		if (_request->GetStatus() == CHttpRequest::ABORTED)
		{
			gameLocal.Printf("Download from %s aborted.\n", _url.c_str());
		}
		else
		{
			gameLocal.Printf("Connection Error.\n");
		}

		_status = FAILED;
	}
	else
	{
		_status = SUCCESS;
	}
}
