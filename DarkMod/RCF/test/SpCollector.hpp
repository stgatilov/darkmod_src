
#ifndef INCLUDE_RCF_UTIL_SPCOLLECTOR_HPP
#define INCLUDE_RCF_UTIL_SPCOLLECTOR_HPP

#include <RCF/util/InitDeinit.hpp>

#if defined(BOOST_SP_ENABLE_DEBUG_HOOKS) && !defined(__MINGW32__)

// this causes naming collisions for classes named "X" in RCF tests...
//#include <boost/config/../../libs/smart_ptr/src/sp_collector.cpp>

// exported from sp_collector.cpp
std::size_t find_unreachable_objects(bool report);
void free_unreachable_objects();

void checkNoCycles()
{
    std::cout <<  "Checking for shared_ptr cycles..." << std::endl;
    std::size_t count = find_unreachable_objects(true);
    if (count > 0)
    {
        std::cout <<  "Detected shared_ptr cycles!" << std::endl;
        abort();
    }
    else
    {
        std::cout <<  "No shared_ptr cycles detected." << std::endl;
    }
}

#else

void checkNoCycles()
{
    std::cout <<  "shared_ptr cycle detection not enabled." << std::endl;
}

#endif

UTIL_ON_DEINIT( checkNoCycles() )

#endif // ! INCLUDE_RCF_UTIL_SPCOLLECTOR_HPP
