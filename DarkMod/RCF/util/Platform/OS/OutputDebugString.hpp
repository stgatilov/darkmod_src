
#ifndef INCLUDE_UTIL_PLATFORM_OS_OUTPUTDEBUGSTRING_HPP
#define INCLUDE_UTIL_PLATFORM_OS_OUTPUTDEBUGSTRING_HPP

#include <boost/config.hpp>

#if defined(BOOST_WINDOWS) || defined(_WIN32)
#include "Windows/OutputDebugString.hpp"
#elif defined(__CYGWIN__)
#include "Unix/OutputDebugString.hpp"
#elif defined(__unix__)
#include "Unix/OutputDebugString.hpp"
#else
#include "UnknownOS/OutputDebugString.hpp"
#endif

#endif // ! INCLUDE_UTIL_PLATFORM_OS_OUTPUTDEBUGSTRING_HPP
