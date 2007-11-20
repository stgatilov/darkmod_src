
#ifndef INCLUDE_UTIL_PLATFORM_OS_UNIX_THREADID_HPP
#define INCLUDE_UTIL_PLATFORM_OS_UNIX_THREADID_HPP

#include <boost/config.hpp>

#ifdef BOOST_HAS_PTHREADS
#include <pthread.h>
#endif

namespace Platform {

    namespace OS {

#ifdef BOOST_HAS_PTHREADS
#ifdef __CYGWIN__
        inline int GetCurrentThreadId() { return reinterpret_cast<int>(static_cast<void*>(pthread_self())); }
#else
        inline int GetCurrentThreadId() { return static_cast<int>(pthread_self()); }
#endif
#else
        inline int GetCurrentThreadId() { return 0; }
#endif

    }
}

#endif // ! INCLUDE_UTIL_PLATFORM_OS_UNIX_THREADID_HPP
