
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_UTIL_INITDEINIT_HPP
#define INCLUDE_UTIL_INITDEINIT_HPP

// memset
#include <memory.h>

#include <stdexcept>

#include <boost/preprocessor/cat.hpp>

#include "AutoRun.hpp"

namespace util {

    typedef void(*InitCallback)();
    typedef void(*DeinitCallback)();

    static const int InitCallbackMaxCount = 25;
    static const int DeinitCallbackMaxCount = 25;

    class InitCallbackArray
    {
    public:
        static InitCallback *get()
        {
            static bool first = true;
            static InitCallback initCallbacks[InitCallbackMaxCount];
            if (first)
            {
                first = false;
                memset(initCallbacks, 0, sizeof(InitCallback)*InitCallbackMaxCount);
            }
            return initCallbacks;
        }
        static bool &getInitCallbacksInvoked()
        {
            static bool initCallbacksInvoked = false;
            return initCallbacksInvoked;
        }
    };

    class DeinitCallbackArray
    {
    public:
        static DeinitCallback *get()
        {
            static bool first = true;
            static DeinitCallback deinitCallbacks[DeinitCallbackMaxCount];
            if (first)
            {
                first = false;
                memset(deinitCallbacks, 0, sizeof(DeinitCallback)*DeinitCallbackMaxCount);
            }
            return deinitCallbacks;
        }
    };

    inline InitCallback *getInitCallbackArray()
    {
        return InitCallbackArray::get();
    }

    inline DeinitCallback *getDeinitCallbackArray()
    {
        return DeinitCallbackArray::get();
    }

    inline bool getInitCallbacksInvoked()
    {
        return InitCallbackArray::getInitCallbacksInvoked();
    }

    inline void setInitCallbacksInvoked(bool initCallbacksInvoked)
    {
        InitCallbackArray::getInitCallbacksInvoked() = initCallbacksInvoked;
    }

    template<typename Callback>
    inline bool addCallback(Callback *callbackArray, Callback callback, const int callbacksMaxCount)
    {
        for (int i=0; i<callbacksMaxCount; ++i)
        {
            if (0 == callbackArray[i])
            {
                callbackArray[i] = callback;
                return true;
            }
            else if (callback == callbackArray[i])
            {
                return true;
            }
        }
        return false;
    }

    template<typename Callback>
    inline void invokeCallbacks(Callback *callbackArray,  const int callbacksMaxCount)
    {
        int i=0;
        while (i<callbacksMaxCount && callbackArray[i])
        {
            Callback callback = callbackArray[i];
            (*callback)();
            i++;
        }
    }
   
    template<typename T>
    inline bool addInitCallback(T (*initCallback)())
    {
       
        if (getInitCallbacksInvoked())
        {
            initCallback();
        }
        bool ok = addCallback(getInitCallbackArray(), InitCallback(initCallback), InitCallbackMaxCount);
        if (!ok)
        {
            throw std::runtime_error("addCallback() failed (no free init/deinit callback slots?)");
        }
        return true;
    }

    inline bool addDeinitCallback(DeinitCallback deinitCallback)
    {
        bool ok = addCallback(getDeinitCallbackArray(), DeinitCallback(deinitCallback), DeinitCallbackMaxCount);
        if (!ok)
        {
            throw std::runtime_error("addCallback() failed (no free init/deinit callback slots?)");
        }

        return true;
    }

    inline void invokeInitCallbacks()
    {
        invokeCallbacks(getInitCallbackArray(), InitCallbackMaxCount);
        setInitCallbacksInvoked(true);
    }

    inline void invokeDeinitCallbacks()
    {
        invokeCallbacks(getDeinitCallbackArray(), DeinitCallbackMaxCount);
        setInitCallbacksInvoked(false);
    }

} // namespace util

#define UTIL_ON_INIT_DEINIT_IMPL(statements, functorName, whichAction)      \
class functorName {                                                         \
public:                                                                     \
    functorName()                                                           \
    {                                                                       \
        static bool first = true;                                           \
        if (first)                                                          \
        {                                                                   \
            first = false;                                                  \
            whichAction(&functorName::invoke);                              \
        }                                                                   \
    }                                                                       \
    static void invoke()                                                    \
    {                                                                       \
        /*RCF5_TRACE("")(#functorName);*/                                   \
        statements;                                                         \
        /*RCF5_TRACE("")(#functorName);*/                                   \
    }                                                                       \
};                                                                          \
static functorName BOOST_PP_CAT(functorName, Inst);

#define UTIL_ON_INIT_NAMED(statements, name)                    UTIL_ON_INIT_DEINIT_IMPL(statements, BOOST_PP_CAT(name, __LINE__), ::util::addInitCallback )
#define UTIL_ON_DEINIT_NAMED(statements, name)                  UTIL_ON_INIT_DEINIT_IMPL(statements, BOOST_PP_CAT(name, __LINE__), ::util::addDeinitCallback )
#define UTIL_ON_INIT_DEINIT_NAMED(initFunc, deinitFunc, name)   UTIL_ON_INIT_NAMED(initFunc, BOOST_PP_CAT(name, Init)) UTIL_ON_DEINIT_NAMED(deinitFunc, BOOST_PP_CAT(name, Deinit))

#define UTIL_ON_INIT(statements)                                UTIL_ON_INIT_NAMED(statements, selfRegisteringInitFunctor)
#define UTIL_ON_DEINIT(statements)                              UTIL_ON_DEINIT_NAMED(statements, selfRegisteringDeinitFunctor)
#define UTIL_ON_INIT_DEINIT(initFunc, deinitFunc)               UTIL_ON_INIT(initFunc) UTIL_ON_DEINIT(deinitFunc)

#endif // ! INCLUDE_UTIL_INITDEINIT_HPP
