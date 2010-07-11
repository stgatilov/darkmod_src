/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <fstream>
#include <boost/shared_ptr.hpp>

class CHttpConnection;

#include "../pugixml/pugixml.hpp"
typedef void CURL;

// Shared_ptr typedef
typedef boost::shared_ptr<pugi::xml_document> XmlDocumentPtr;

/**
 * greebo: An object representing a single HttpRequest, holding 
 * the result (string) and status information.
 *
 * Use the Perform() method to execute the request.
 */
class CHttpRequest
{
public:

	enum RequestStatus
	{
		NOT_PERFORMED_YET,
		OK,	// successful
		IN_PROGRESS,
		FAILED
	};

private:
	// The connection we're working with
	CHttpConnection& _conn;

	// The URL we're supposed to query
	std::string _url;

	std::vector<char> _buffer;

	// The curl handle
	CURL* _handle;

	// The current state
	RequestStatus _status;

	std::string _destFilename;

	std::ofstream _destStream;

public:
	CHttpRequest(CHttpConnection& conn, const std::string& url);

	CHttpRequest(CHttpConnection& conn, const std::string& url, const std::string& destFilename);

	// Callbacks for CURL
	static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, CHttpRequest* self);
	static size_t WriteFileCallback(void* ptr, size_t size, size_t nmemb, CHttpRequest* self);

	RequestStatus GetStatus();

	// Perform the request
	void Perform();

	// Returns the result string
	std::string GetResultString();

	// Returns the result as XML document
	XmlDocumentPtr GetResultXml();

private:
	// shared constructor code
	void Construct();
};
typedef boost::shared_ptr<CHttpRequest> CHttpRequestPtr;

#endif /* _HTTP_REQUEST_H_ */
