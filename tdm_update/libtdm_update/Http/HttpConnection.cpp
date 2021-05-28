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

#include "HttpConnection.h"
#include "HttpRequest.h"

#ifdef WIN32
#include <winsock2.h> // greebo: need to include winsock2 before curl/curl.h
#include <Ws2tcpip.h>
#include <Wspiapi.h>
#endif

#include <curl/curl.h>

namespace tdm
{

HttpConnection::HttpConnection() :
	_bytesDownloaded(0)
{
	curl_global_init(CURL_GLOBAL_ALL);
}

HttpConnection::~HttpConnection()
{
	// Clean up cURL
	curl_global_cleanup();
}

bool HttpConnection::HasProxy()
{
	return !_proxyHost.empty();
}

std::string HttpConnection::GetProxyHost()
{
	return _proxyHost;
}

std::string HttpConnection::GetProxyUsername()
{
	return _proxyUser;
}

std::string HttpConnection::GetProxyPassword()
{
	return _proxyPass;
}

void HttpConnection::SetProxyHost(const std::string& host)
{
	_proxyHost = host;
}

void HttpConnection::SetProxyUsername(const std::string& user)
{
	_proxyUser = user;
}

void HttpConnection::SetProxyPassword(const std::string& pass)
{
	_proxyPass = pass;
}

HttpRequestPtr HttpConnection::CreateRequest(const std::string& url)
{
	return HttpRequestPtr(new HttpRequest(*this, url));
}

HttpRequestPtr HttpConnection::CreateRequest(const std::string& url, const std::string& destFilename)
{
	return HttpRequestPtr(new HttpRequest(*this, url, destFilename));
}

void HttpConnection::AddBytesDownloaded(std::size_t bytes)
{
	// Make sure only one thread is accessing the counter at a time
	std::lock_guard<std::mutex> lock(_bytesDownloadedMutex);

	_bytesDownloaded += bytes;
}

std::size_t HttpConnection::GetBytesDownloaded() const
{
	return _bytesDownloaded;
}

}
