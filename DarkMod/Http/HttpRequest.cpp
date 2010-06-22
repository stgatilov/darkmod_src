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

#include "HttpRequest.h"
#include "HttpConnection.h"

#include <winsock2.h> // greebo: need to include winsock2 before curl/curl.h
#include <curl/curl.h>

struct MemoryStruct
{
	char *memory;
	size_t size;
};
 
static void* myrealloc(void *ptr, size_t size)
{
	/* There might be a realloc() out there that doesn't like reallocing
	 NULL pointers, so we take care of it here */ 
	if (ptr)
		return realloc(ptr, size);
	else
		return malloc(size);
}
 
CHttpRequest::CHttpRequest(CHttpConnection& conn, const std::string& url) :
	_conn(conn),
	_url(url),
	_handle(NULL)
{
	// Init the curl session
	_handle = curl_easy_init();

	// specify URL to get
	curl_easy_setopt(_handle, CURLOPT_URL, url.c_str());

	// send all data to this function
	curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION, CHttpRequest::WriteMemoryCallback);

	// We pass ourselves as user data pointer to the callback function
	curl_easy_setopt(_handle, CURLOPT_WRITEDATA, this);

	// Set agent
	curl_easy_setopt(_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
}

void CHttpRequest::Perform()
{
	CURLcode result = curl_easy_perform(_handle);

	curl_easy_cleanup(_handle);

	_handle = NULL;
}

std::string CHttpRequest::GetResultString()
{
	return _buffer.empty() ? "" : std::string(&_buffer.front());
}

size_t CHttpRequest::WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, CHttpRequest* self)
{
	// Needed size
	std::size_t bytesToCopy = size * nmemb;

	std::vector<char>& buf = self->_buffer; // shortcut 

	std::size_t appendPosition = buf.size() > 0 ? buf.size() - 2 : 0;

	// The first allocation should request one extra byte for the trailing \0
	self->_buffer.resize(buf.size() > 0 ? buf.size() + bytesToCopy : bytesToCopy + 1);

	// Push the bytes
	memcpy(&(buf[appendPosition]), ptr, bytesToCopy);

	// Append trailing \0 if possible
	if (buf.size() > 0)
	{
		buf[buf.size() - 1] = 0;
	}

	return static_cast<size_t>(bytesToCopy);
}
