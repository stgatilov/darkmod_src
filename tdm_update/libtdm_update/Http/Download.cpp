/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod Updater (http://www.thedarkmod.com/)
 
******************************************************************************/

#include "Download.h"
#include "../Http/HttpConnection.h"
#include "../Zip/Zip.h"
#include "../TraceLog.h"
#include "../File.h"
#include "../CRC.h"
#include "../Util.h"
#include "../Constants.h"
#include "../tdmsync/tdmsync_curl.h"

#include <functional>

namespace fs = boost::filesystem;

namespace tdm
{

Download::Download(const HttpConnectionPtr& conn, const std::string& url, const fs::path& destFilename) :
	_curUrl(0),
	_destFilename(destFilename),
	_conn(conn),
	_status(NOT_STARTED_YET),
	_pk4CheckEnabled(false),
	_crcCheckEnabled(false),
	_filesizeCheckEnabled(false),
	_requiredFilesize(0),
	_requiredCrc(0)
{
	_urls.push_back(url);
}

Download::Download(const HttpConnectionPtr& conn, const std::vector<std::string>& urls, const fs::path& destFilename) :
	_urls(urls),
	_curUrl(0),
	_destFilename(destFilename),
	_conn(conn),
	_status(NOT_STARTED_YET),
	_pk4CheckEnabled(false),
	_crcCheckEnabled(false),
	_filesizeCheckEnabled(false),
	_requiredFilesize(0),
	_requiredCrc(0)
{}

Download::~Download()
{
	Stop();
}

Download::DownloadStatus Download::GetStatus()
{
	return _status;
}

bool Download::IsFinished()
{
	return _status == FAILED || _status == SUCCESS;
}

void Download::Start()
{
	if (_status != NOT_STARTED_YET)
	{
		return; // don't allow double starts
	}

	// Construct the temporary filename
	fs::path filename = fs::path(_destFilename).leaf();
	fs::path folder = fs::path(_destFilename).branch_path();

	if (!fs::exists(folder))
	{
		// Make sure the folder exists
		if (!fs::create_directories(folder))
		{
			throw std::runtime_error("Could not create directory: " + folder.string());
		}
	}

	// /path/to/fms/TMP_FILE_PREFIXfilename.pk4 (TMP_FILE_PREFIX is usually an underscore)
	_tempFilename = folder / (TMP_FILE_PREFIX + filename.string());
	TraceLog::WriteLine(LOG_VERBOSE, "Downloading to temporary file " + _tempFilename.string());

	_status = IN_PROGRESS;
	_thread = ThreadPtr(new std::thread(std::bind(&Download::Perform, this)));
}

void Download::Stop()
{
	if (_thread != NULL)
	{
		// Set the URL index beyond the list size to prevent 
		// the worker thread from proceeding to the next URL
		//TODO: better way of stopping download thread!
		_curUrl = _urls.size();

		// Cancel the request
		if (_request)
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

		// Remove temporary file
		File::Remove(_tempFilename);
	}
}

double Download::GetProgressFraction()
{
	return _request != NULL ? _request->GetProgressFraction() : 0.0;
}

double Download::GetDownloadSpeed()
{
	return _request != NULL ? _request->GetDownloadSpeed() : 0.0;
}

std::size_t Download::GetDownloadedBytes()
{
	return _request != NULL ? _request->GetDownloadedBytes() : 0;
}

void Download::EnableValidPK4Check(bool enable)
{
	_pk4CheckEnabled = enable;
}

void Download::EnableCrcCheck(bool enable)
{
	_crcCheckEnabled = enable;
}

void Download::EnableFilesizeCheck(bool enable)
{
	_filesizeCheckEnabled = enable;
}

void Download::SetRequiredCrc(uint32_t requiredCrc)
{
	_requiredCrc = requiredCrc;
}

void Download::SetRequiredFilesize(std::size_t requiredSize)
{
	_requiredFilesize = requiredSize;
}

void Download::Perform()
{
	//first pass: only download if tdmsync metainfo is available
	for (_curUrl = 0; _curUrl < _urls.size(); _curUrl++) {
		const std::string& url = _urls[_curUrl];
		using namespace TdmSync;

		try {
			std::string dataUri = url;
			std::string metaUri = url + ".tdmsync";
			std::string localFn = _destFilename.string();
			std::string metaFn = _tempFilename.string() + ".tdmsync";
			std::string downFn = _tempFilename.string() + ".bindiff";
			std::string resultFn = _tempFilename.string();

			//download metainfo file (if present)
			StdioFile metaFile;
			metaFile.open(metaFn.c_str(), StdioFile::Write);
			CurlDownloader curlWrapper;
			curlWrapper.downloadMeta(metaFile, metaUri.c_str());
			metaFile.flush();

			//read downloaded metainfo file
			metaFile.open(metaFn.c_str(), StdioFile::Read);
			FileInfo info;
			info.deserialize(metaFile);

			//if file being downloaded is not present, create it empty
			if (!fs::exists(_destFilename))
				StdioFile().open(localFn.c_str(), StdioFile::Write);

			//devise the update plan from local file and metainfo
			StdioFile localFile;
			localFile.open(localFn.c_str(), StdioFile::Read);
			UpdatePlan plan = info.createUpdatePlan(localFile);
			//plan.print();

			//download the missing parts --- smart "differential update"
			StdioFile downloadFile;
			downloadFile.open(downFn.c_str(), StdioFile::Write);
			CurlDownloader curlWrapper2;
			curlWrapper2.downloadMissingParts(downloadFile, plan, dataUri.c_str());
			downloadFile.flush();
			auto mode = curlWrapper2.getModeUsed();
			if (mode == CurlDownloader::dmManyByteranges)
				TraceLog::WriteLine(LOG_STANDARD, "Fallback to many byterange requests");

			//perform update and rewrite the old version of destination file
			downloadFile.open(downFn.c_str(), StdioFile::Read);
			StdioFile resultFile;
			resultFile.open(resultFn.c_str(), StdioFile::Write);
			plan.apply(localFile, downloadFile, resultFile);
			resultFile.flush();
		}
		catch(const std::exception &e) {
			TraceLog::WriteLine(LOG_VERBOSE, std::string("tdmsync error: ") + e.what());
			continue;
		}

		bool valid = CheckIntegrity();
		if (valid) {
			TraceLog::WriteLine(LOG_VERBOSE, "Downloaded file passed the integrity checks.");
			File::Remove(_destFilename);
			if (File::Move(_tempFilename, _destFilename))
				_status = SUCCESS;
			else
				_status = FAILED;
			return;
		}
	}

	//second pass: just download the file if possible
	_curUrl = 0;
	while (_curUrl < _urls.size())
	{
		// Remove any previous temporary file
		File::Remove(_tempFilename);

		const std::string& url = _urls[_curUrl];

		// Create a new request
		_request = _conn->CreateRequest(url, _tempFilename.string());
	
		// Start the download, blocks until finished or aborted
		_request->Perform();

		if (_request->GetStatus() == HttpRequest::OK)
		{
			// Check the downloaded file
			bool valid = CheckIntegrity();

			if (!valid)
			{
				_curUrl++;
				continue;
			}
			else
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Downloaded file passed the integrity checks.");
			}

			// Remove the destination filename before moving the temporary file over
			File::Remove(_destFilename);

			// Move temporary file to the real one
			if (File::Move(_tempFilename, _destFilename))
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
			if (_request->GetStatus() == HttpRequest::ABORTED)
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Download aborted.");
			}
			else
			{
				TraceLog::WriteLine(LOG_VERBOSE, "Connection Error.");
			}

			// Proceed to the next URL
			_curUrl++;
		}
	} // while

	// Have we run out of URLs
	if (_curUrl >= _urls.size())
	{
		// This was our last URL, set the status to FAILED
		_status = FAILED;
	}
}

const fs::path& Download::GetDestFilename() const
{
	return _destFilename;
}

std::string Download::GetFilename() const
{
	return _destFilename.leaf().string();
}

bool Download::CheckIntegrity()
{
	if (_filesizeCheckEnabled)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Checking filesize of downloaded file, expecting %d") %
				_requiredFilesize).str());

		if (fs::file_size(_tempFilename) != _requiredFilesize)
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Downloaded file has the wrong size, expected %d but found %d") %
				_requiredFilesize % fs::file_size(_tempFilename)).str());
			return false; // failed the file size check
		}
	}

	if (_pk4CheckEnabled)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Checking download for 'is-a-zipfile'.")).str());

		ZipFileReadPtr zipFile = Zip::OpenFileRead(_tempFilename);

		if (zipFile == NULL) 
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Downloaded file failed the zip check: %s") %
				_tempFilename.string()).str());
			return false; // failed the ZIP check
		}
	}

	if (_crcCheckEnabled)
	{
		TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Checking CRC of downloaded file, expecting %x") %
				_requiredCrc).str());

		uint32_t crc = CRC::GetCrcForFile(_tempFilename);

		if (crc != _requiredCrc)
		{
			TraceLog::WriteLine(LOG_VERBOSE, (boost::format("Downloaded file has the wrong size, expected %x but found %x") %
				_requiredCrc % crc).str());
			return false; // failed the crc check
		}
	}

	return true; // no failed checks, assume OK
}

}
