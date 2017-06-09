#ifdef MAKEDLL
#  define BUILDING_LIBCURL 1
#else
#endif

#include "../include/curl/curl.h"

namespace ExtLibs {
	CURL_EXTERN CURL *curl_easy_init( void );
	CURL_EXTERN CURLcode curl_easy_setopt( CURL *curl, CURLoption option, ... );
	CURL_EXTERN CURLcode curl_easy_perform( CURL *curl );
	CURL_EXTERN void curl_easy_cleanup( CURL *curl );
	CURL_EXTERN CURLcode curl_global_init( long flags );
	CURL_EXTERN void curl_global_cleanup( void );
	CURL_EXTERN CURLcode curl_easy_getinfo( CURL *curl, CURLINFO info, ... );

}