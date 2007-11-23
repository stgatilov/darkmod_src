
#ifndef INCLUDE_UTIL_PLATFORM_OS_GETCURRENTTIME_HPP
#define INCLUDE_UTIL_PLATFORM_OS_GETCURRENTTIME_HPP

#if defined(__WINDOWS__) || defined(_WIN32)
#include "Windows/GetCurrentTime.hpp"
#elif defined(__CYGWIN__)
#include "Unix/GetCurrentTime.hpp"
#elif defined(__unix__)
#include "Unix/GetCurrentTime.hpp"
#else
#include "UnknownOS/GetCurrentTime.hpp"
#endif

#endif // ! INCLUDE_UTIL_PLATFORM_OS_GETCURRENTTIME_HPP
