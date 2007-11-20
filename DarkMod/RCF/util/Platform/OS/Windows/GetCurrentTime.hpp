
#ifndef INCLUDE_UTIL_PLATFORM_OS_WINDOWS_GETCURRENTTIME_HPP
#define INCLUDE_UTIL_PLATFORM_OS_WINDOWS_GETCURRENTTIME_HPP

#include <sys/types.h>
#include <sys/timeb.h>

namespace Platform {

    namespace OS {

#ifdef __BORLANDC__
#define _timeb timeb
#define _ftime ftime
#endif

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 )  // warning C4996: '...' was declared deprecated
#endif

        inline unsigned int getCurrentTimeMs()
        {
            struct _timeb timebuffer;
            _ftime( &timebuffer );
            unsigned int timeS = static_cast<unsigned int>(timebuffer.time);
            unsigned int timeMs = timebuffer.millitm;
            return (timeS & 0xffff)*1000 + timeMs;
        }

#ifdef _MSC_VER
#pragma warning( pop )
#endif

    }

}

#endif // ! INCLUDE_UTIL_PLATFORM_OS_WINDOWS_GETCURRENTTIME_HPP
