
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/Random.hpp>

#include <sys/timeb.h>
#include <sys/types.h>

#include <stdlib.h>
#include <time.h>

#include <RCF/InitDeinit.hpp>
#include <RCF/ThreadLibrary.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    int generateRandomNumber()
    {
        static bool first = true;
        if (first)
        {
            first = false;
        }
        // TODO: document why we need a mutex here
        static Mutex m;
        {
            Lock lock(m); RCF_UNUSED_VARIABLE(lock);
            return rand();
        }
    }

    // thread-safe initialization
    RCF_ON_INIT_NAMED( generateRandomNumber(), generateRandomNumberInit )

}
