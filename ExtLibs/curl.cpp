#include "stdafx.h"
#include "curl.h"

namespace ExtLibs {
	CURLcode curl_easy_setopt( CURL *curl, CURLoption option, ... ) {
		va_list args;
		va_start( args, option );
		return ::curl_easy_setopt( curl, option, args );
		va_end( args );
	}

	CURL *curl_easy_init( void ) {
		return ::curl_easy_init();
	}

	CURLcode curl_easy_perform( CURL *curl ) {
		return ::curl_easy_perform( curl );
	}

	void curl_easy_cleanup( CURL *curl ) {
		return ::curl_easy_cleanup( curl );
	}

	CURLcode curl_global_init( long flags ) {
		return ::curl_global_init( flags );
	}

	void curl_global_cleanup( void ) {
		return ::curl_global_cleanup();
	}

	CURL_EXTERN CURLcode curl_easy_getinfo( CURL *curl, CURLINFO info, ... ) {
		va_list args;
		va_start( args, info );
		return ::curl_easy_getinfo( curl, info, args );
		va_end( args );
	}
}