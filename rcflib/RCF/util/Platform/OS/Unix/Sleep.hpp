
#ifndef INCLUDE_UTIL_PLATFORM_OS_UNIX_SLEEP_HPP
#define INCLUDE_UTIL_PLATFORM_OS_UNIX_SLEEP_HPP

#include <unistd.h>

namespace Platform {

    namespace OS {

        inline void Sleep(unsigned int seconds) { ::sleep(seconds); }

    }

}

#endif // ! INCLUDE_UTIL_PLATFORM_OS_UNIX_SLEEP_HPP
