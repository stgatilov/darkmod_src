
#ifndef INCLUDE_UTIL_PLATFORM_OS_UNIX_GETCURRENTTIME_HPP
#define INCLUDE_UTIL_PLATFORM_OS_UNIX_GETCURRENTTIME_HPP

// Linux docs recommend using gettimeofday() instead of ftime()...

/*
#include <sys/time.h>

namespace Platform {

    namespace OS {

        inline unsigned int getCurrentTimeMs()
        {
            timeval tv;
            gettimeofday(&tv, NULL);
            unsigned int timeS = static_cast<unsigned int>(tv.tv_sec);
            unsigned int timeMs = static_cast<unsigned int>(tv.tv_usec/1000);
            return (timeS & 0xffff)*1000 + timeMs;
        }

    }

}
*/

#include <sys/types.h>
#include <sys/timeb.h>

namespace Platform {

    namespace OS {

        inline unsigned int getCurrentTimeMs()
        {
            struct timeb timebuffer;
            ftime( &timebuffer );
            unsigned int timeS = static_cast<unsigned int>(timebuffer.time);
            unsigned int timeMs = timebuffer.millitm;
            return (timeS & 0xffff)*1000 + timeMs;
        }

    }

}

#endif // ! INCLUDE_UTIL_PLATFORM_OS_UNIX_GETCURRENTTIME_HPP
