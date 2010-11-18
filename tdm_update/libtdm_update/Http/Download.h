#pragma once

#include "HttpConnection.h"
#include "HttpRequest.h"
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace tdm
{

/**
 * An object representing a single download.
 * The actual work will be performed in a separate thread,
 * hence the Start() method will not block execution.
 *
 * Check the GetStatus() and GetProgressFraction() methods
 * to get some information about the actual status.
 * 
 * Once the Download object is destroyed any ongoing 
 * worker thread will be terminated.
 *
 * Download data is stored in a temporary file until the whole
 * file is finished - then the temporary will be renamed to
 * the destination filename. If the destination filename is already
 * in use, the move will fail and the download will remain
 * in the temporary file. The temporary file is named the same
 * as the destination filename, but with a prefixed underscore character:
 * e.g. target/directory/_download.pk4
 */
class Download
{
protected:
	// The target URLs we're trying to download from
	std::vector<std::string> _urls;

	// The URL index
	std::size_t _curUrl;

	// destination file
	fs::path _destFilename;

	// Temporary filename for partial download data
	fs::path _tempFilename;

public:
	enum DownloadStatus
	{
		NOT_STARTED_YET,
		IN_PROGRESS,
		FAILED,
		SUCCESS,
	};

protected:
	HttpConnectionPtr _conn;

	DownloadStatus _status;

	// The corresponding HTTP request
	HttpRequestPtr _request;

	typedef boost::shared_ptr<boost::thread> ThreadPtr;
	ThreadPtr _thread;

	bool _pk4CheckEnabled;

public:
	/** 
	 * greebo: Construct a new Download using the given URL.
	 * The download data will be saved to the given destFilename;
	 */
	Download(const HttpConnectionPtr& conn, const std::string& url, const fs::path& destFilename);

	/**
	 * greebo: Construct a new Download using the given list of 
	 * alternative URLs. If downloading from the first URL fails
	 * the next one will be tried until no more alternatives are left.
	 * The result will be saved to destFilename.
	 */
	Download(const HttpConnectionPtr& conn, const std::vector<std::string>& urls, const fs::path& destFilename);

	virtual ~Download();

	// Start the download. This will spawn a new thread and execution
	// will return to the calling code.
	void Start();

	// Cancel the download. If the download has already finished, this does nothing.
	void Stop();

	// Enable the validation of the downloaded file
	void EnableValidPK4Check(bool enable);
	
	// The current status of this download
	DownloadStatus GetStatus();

	// Returns true if the download is done (successful or not)
	bool IsFinished();

	// Get the progress fraction [0..1]
	double GetProgressFraction();

	// Get the download speed (bytes/sec)
	double GetDownloadSpeed();

	// Get the downloaded bytes
	std::size_t GetDownloadedBytes();

	// Return the destination filename of this download
	const fs::path& GetDestFilename() const;

	// Get the destination filename (leaf) of the current download (remote filename)
	std::string GetFilename() const;

	// Check method
	static bool CheckValidPK4(const fs::path& path);

protected:
	// Thread entry point
	void Perform();
};
typedef boost::shared_ptr<Download> DownloadPtr;

}
