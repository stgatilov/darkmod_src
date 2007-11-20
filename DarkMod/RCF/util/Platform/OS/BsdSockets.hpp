
#ifndef INCLUDE_UTIL_PLATFORM_OS_BSDSOCKETS_HPP
#define INCLUDE_UTIL_PLATFORM_OS_BSDSOCKETS_HPP

#include <boost/config.hpp>

#if defined(BOOST_WINDOWS) || defined(_WIN32)
#include "Windows/BsdSockets.hpp"
#elif defined(__CYGWIN__)
#include "Unix/BsdSockets.hpp"
#elif defined(__unix__)
#include "Unix/BsdSockets.hpp"
#else
#include "UnknownOS/BsdSockets.hpp"
#endif

#endif // ! INCLUDE_UTIL_PLATFORM_OS_BSDSOCKETS_HPP
