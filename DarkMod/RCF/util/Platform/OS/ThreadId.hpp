
#ifndef INCLUDE_UTIL_PLATFORM_OS_THREADID_HPP
#define INCLUDE_UTIL_PLATFORM_OS_THREADID_HPP

#include <boost/config.hpp>

#if defined(BOOST_WINDOWS) || defined(_WIN32)
#include "Windows/ThreadId.hpp"
#elif defined(__CYGWIN__)
#include "Unix/ThreadId.hpp"
#elif defined(__unix__)
#include "Unix/ThreadId.hpp"
#else
#include "UnknownOS/ThreadId.hpp"
#endif

#endif // ! INCLUDE_UTIL_PLATFORM_OS_THREADID_HPP
