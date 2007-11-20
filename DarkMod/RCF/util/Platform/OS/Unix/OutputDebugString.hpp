
#ifndef INCLUDE_UTIL_PLATFORM_OS_UNIX_OUTPUTDEBUGSTRING_HPP
#define INCLUDE_UTIL_PLATFORM_OS_UNIX_OUTPUTDEBUGSTRING_HPP

#include <stdio.h>

namespace Platform {

    namespace OS {

        inline void OutputDebugString(const char *sz)
        {

#ifdef OUTPUTDEBUGSTRING_TO_STDERR
            fprintf(stderr, "%s", sz);
#endif

#ifdef OUTPUTDEBUGSTRING_TO_STDOUT
            fprintf(stdout, "%s", sz);
#endif

#ifdef OUTPUTDEBUGSTRING_TO_FILE
            static FILE *file = fopen( "OutputDebugString.txt", "w" );
            fprintf(file, "%s", sz);
#endif

        }

    }

}

#endif // ! INCLUDE_UTIL_PLATFORM_OS_UNIX_OUTPUTDEBUGSTRING_HPP
