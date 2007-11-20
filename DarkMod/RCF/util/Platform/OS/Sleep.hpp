
#ifndef INCLUDE_UTIL_PLATFORM_OS_SLEEP_HPP
#define INCLUDE_UTIL_PLATFORM_OS_SLEEP_HPP

#include <boost/config.hpp>

#if defined(BOOST_WINDOWS) || defined(_WIN32)
#include "Windows/Sleep.hpp"
#elif defined(__CYGWIN__)
#include "Unix/Sleep.hpp"
#elif defined(__unix__)
#include "Unix/Sleep.hpp"
#else
#include "UnknownOS/Sleep.hpp"
#endif

#endif // ! INCLUDE_UTIL_PLATFORM_OS_SLEEP_HPP
