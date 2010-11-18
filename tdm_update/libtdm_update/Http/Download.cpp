#include "Download.h"
#include "../Http/HttpConnection.h"
#include "../Zip/Zip.h"
#include "../TraceLog.h"
#include "../File.h"
#include "../Constants.h"

#include <boost/bind.hpp>

namespace fs = boost::filesystem;

namespace tdm
{

Download::Download(const HttpConnectionPtr& conn, const std::string& url, const fs::path& destFilename) :
	_curUrl(0),
	_destFilename(destFilename),
	_conn(conn),
	_status(NOT_STARTED_YET),
	_pk4CheckEnabled(false)
{
	_urls.push_back(url);
}

Download::Download(const HttpConnectionPtr& conn, const std::vector<std::string>& urls, const fs::path& destFilename) :
	_urls(urls),
	_curUrl(0),
	_destFilename(destFilename),
	_conn(conn),
	_status(NOT_STARTED_YET),
	_pk4CheckEnabled(false)
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
			throw std::runtime_error("Could not create directory: " + folder.file_string());
		}
	}

	// /path/to/fms/TMP_FILE_PREFIXfilename.pk4 (TMP_FILE_PREFIX is usually an underscore)
	_tempFilename = folder / (TMP_FILE_PREFIX + filename.string());
	TraceLog::WriteLine(LOG_VERBOSE, "Downloading to temporary file " + _tempFilename.string());

	_status = IN_PROGRESS;
	_thread = ThreadPtr(new boost::thread(boost::bind(&Download::Perform, this)));
}

void Download::Stop()
{
	if (_thread != NULL && _request != NULL)
	{
		// Set the URL index beyond the list size to prevent 
		// the worker thread from proceeding to the next URL
		_curUrl = _urls.size();

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

void Download::Perform()
{
	while (_curUrl < _urls.size())
	{
		// Remove any previous temporary file
		File::Remove(_tempFilename);

		const std::string& url = _urls[_curUrl];

		// Create a new request
		_request = _conn->CreateRequest(url, _tempFilename.file_string());
	
		// Start the download, blocks until finished or aborted
		_request->Perform();

		if (_request->GetStatus() == HttpRequest::OK)
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
				else
				{
					TraceLog::WriteLine(LOG_VERBOSE, "Downloaded file is a valid archive.");
				}
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
	return _destFilename.leaf();
}

bool Download::CheckValidPK4(const fs::path& path)
{
	ZipFileReadPtr zipFile = Zip::OpenFileRead(path.file_string());

	return (zipFile != NULL);
}

}
