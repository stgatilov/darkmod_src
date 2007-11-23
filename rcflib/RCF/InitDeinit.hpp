
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_INITDEINIT_HPP
#define INCLUDE_RCF_INITDEINIT_HPP

#include <RCF/util/InitDeinit.hpp>

/// RCF_ON_INIT - macro for specifying code that should be run upon RCF initialization
/// \param statements Code (statements) to execute.
#define RCF_ON_INIT(statements)                 UTIL_ON_INIT(statements)

/// RCF_ON_INIT - macro for specifying code that should be run upon RCF deinitialization
/// \param statements Code (statements) to execute.
#define RCF_ON_DEINIT(statements)               UTIL_ON_DEINIT(statements)

/// RCF_ON_INIT - macro for specifying code that should be run upon RCF initialization and deinitialization
/// \param initStatements Code (statements) to execute upon initialization.
/// \param deinitStatements Code (statements) to execute upon deinitialization.
#define RCF_ON_INIT_DEINIT(initStatements,deinitStatements)       UTIL_ON_INIT_DEINIT(initStatements,deinitStatements)

#define RCF_ON_INIT_NAMED(x, name)              UTIL_ON_INIT_NAMED(x, name)
#define RCF_ON_DEINIT_NAMED(x, name)            UTIL_ON_DEINIT_NAMED(x, name)
#define RCF_ON_INIT_DEINIT_NAMED(x,y, name)     UTIL_ON_INIT_DEINIT_NAMED(x,y, name)

namespace RCF {

    extern bool gIsInited;

    /// Initializes RCF framework. Should be called once by application.
    inline void init()
    {
        if (!gIsInited)
        {
            util::invokeInitCallbacks();
            gIsInited = true;
        }
    }

    /// Deinitializes RCF framework. Should be called once by application.
    inline void deinit()
    {
        if (gIsInited)
        {
            util::invokeDeinitCallbacks();
            gIsInited = false;
        }
    }

} // namespace RCF

#endif // ! INCLUDE_RCF_INITDEINIT_HPP
