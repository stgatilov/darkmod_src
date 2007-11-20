
#ifndef INCLUDE_UTIL_PLATFORM_OS_WINDOWS_SLEEP_HPP
#define INCLUDE_UTIL_PLATFORM_OS_WINDOWS_SLEEP_HPP

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include "Windows.h"

namespace Platform {

    namespace OS {

        // +1 here causes a context switch if Sleep(0) is called
        inline void Sleep(unsigned int seconds) { ::Sleep(1000*seconds+1); }

    }

}

#endif // ! INCLUDE_UTIL_PLATFORM_OS_WINDOWS_SLEEP_HPP
