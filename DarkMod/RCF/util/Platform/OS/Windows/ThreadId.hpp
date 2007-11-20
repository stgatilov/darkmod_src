
#ifndef INCLUDE_UTIL_PLATFORM_OS_WINDOWS_THREADID_HPP
#define INCLUDE_UTIL_PLATFORM_OS_WINDOWS_THREADID_HPP

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include "Windows.h"

namespace Platform {

    namespace OS {

        inline int GetCurrentThreadId() { return ::GetCurrentThreadId(); }

    }
}

#endif // ! INCLUDE_UTIL_PLATFORM_OS_WINDOWS_THREADID_HPP
