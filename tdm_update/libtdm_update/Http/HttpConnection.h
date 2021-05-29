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

#pragma once

#include <string>
#include <memory>
#include <mutex>

namespace tdm
{

class HttpRequest;
typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

/**
 * greebo: An object representing a single HttpConnection, holding 
 * proxy settings and providing error handling.
 *
 * Use the CreateRequest() method to generate a new request object.
 */
class HttpConnection
{
private:
	std::string _proxyHost;
	std::string _proxyUser;
	std::string _proxyPass;

	// a thread-safe counter, measuring the download bandwidth used
	std::size_t _bytesDownloaded;

	// The mutex for managing access to the counter above
	std::mutex _bytesDownloadedMutex;

public:
	HttpConnection();
	~HttpConnection();

	bool HasProxy();

	std::string GetProxyHost();
	std::string GetProxyUsername();
	std::string GetProxyPassword();

	void SetProxyHost(const std::string& host);
	void SetProxyUsername(const std::string& user);
	void SetProxyPassword(const std::string& pass);

	// Add downlodded bytes to the counter
	void AddBytesDownloaded(std::size_t bytes);

	// Returns the total number of bytes downloaded through this connection
	std::size_t GetBytesDownloaded() const;

	/**
	 * Constructs a new HTTP request using the given URL (optional: filename)
	 */ 
	HttpRequestPtr CreateRequest(const std::string& url);
	HttpRequestPtr CreateRequest(const std::string& url, const std::string& destFilename);
};
typedef std::shared_ptr<HttpConnection> HttpConnectionPtr;

}
