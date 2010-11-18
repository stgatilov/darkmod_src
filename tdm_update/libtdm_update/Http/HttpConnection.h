#pragma once

#include <string>
#include <boost/shared_ptr.hpp>

namespace tdm
{

class HttpRequest;
typedef boost::shared_ptr<HttpRequest> HttpRequestPtr;

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

	/**
	 * Constructs a new HTTP request using the given URL (optional: filename)
	 */ 
	HttpRequestPtr CreateRequest(const std::string& url);
	HttpRequestPtr CreateRequest(const std::string& url, const std::string& destFilename);
};
typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;

}
