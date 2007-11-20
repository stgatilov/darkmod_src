
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

//*****************************
// DEPRECATED!!!
//*****************************

#ifndef INCLUDE_UTIL_AUTORUN_HPP
#define INCLUDE_UTIL_AUTORUN_HPP

#define AUTO_RUN(func)                              AUTO_RUN_(func,  __LINE__)
#define AUTO_RUN_(func, nID)                        AUTO_RUN__(func, nID)
#define AUTO_RUN__(func, nID)                       AUTO_RUN___(func, nID)
#define AUTO_RUN___(func, ID)                                                           \
    namespace {                                                                         \
        struct AutoRun##ID {                                                            \
            AutoRun##ID() {                                                             \
                func;                                                                   \
            }                                                                           \
        } AutoRunInst##ID;                                                              \
    }

    // More concise to implement AUTO_RUN using the following, but BCB emits an ICE on it.
    //static bool AutoRun##ID = ( func , false);


#define AUTO_RUN_1(func)                            AUTO_RUN_NAMED(func, 1)                   
#define AUTO_RUN_2(func)                            AUTO_RUN_NAMED(func, 2)                   
#define AUTO_RUN_3(func)                            AUTO_RUN_NAMED(func, 3)                   
#define AUTO_RUN_4(func)                            AUTO_RUN_NAMED(func, 4)                   
#define AUTO_RUN_5(func)                            AUTO_RUN_NAMED(func, 5)                   

#define AUTO_RUN_NAMED(func, name)                  AUTO_RUN_NAMED_(func, name, __LINE__)
#define AUTO_RUN_NAMED_(func, name, nID)            AUTO_RUN_NAMED__(func, name, nID)
#define AUTO_RUN_NAMED__(func, name, nID)           AUTO_RUN___(func, _##name##_##nID)

#define AUTO_RUN_ONCE(func)                         AUTO_RUN_ONCE_(func,  __LINE__)
#define AUTO_RUN_ONCE_(func, nID)                   AUTO_RUN_ONCE__(func, nID)
#define AUTO_RUN_ONCE__(func, nID)                  AUTO_RUN_ONCE___(func, nID)
#define AUTO_RUN_ONCE___(func, ID)                                                      \
    struct AutoRunOnce##ID {                                                            \
        AutoRunOnce##ID() {                                                             \
            if (!init()) {                                                              \
                init() = true;                                                          \
                func;                                                                   \
            }                                                                           \
        }                                                                               \
        static bool &init() {                                                           \
            static bool bInit = false;                                                  \
            return bInit;                                                               \
        }                                                                               \
    };                                                                                  \
    static AutoRunOnce##ID AutoRunOnceInst##ID;

#define AUTO_RUN_ONCE_1(func)                           AUTO_RUN_ONCE_NAMED(func, 1)                   
#define AUTO_RUN_ONCE_2(func)                           AUTO_RUN_ONCE_NAMED(func, 2)                   
#define AUTO_RUN_ONCE_3(func)                           AUTO_RUN_ONCE_NAMED(func, 3)                   
#define AUTO_RUN_ONCE_4(func)                           AUTO_RUN_ONCE_NAMED(func, 4)                   
#define AUTO_RUN_ONCE_5(func)                           AUTO_RUN_ONCE_NAMED(func, 5)                   

#define AUTO_RUN_ONCE_NAMED(func, name)                 AUTO_RUN_ONCE_NAMED_(func, name, __LINE__)
#define AUTO_RUN_ONCE_NAMED_(func, name, nID)           AUTO_RUN_ONCE_NAMED__(func, name, nID)
#define AUTO_RUN_ONCE_NAMED__(func, name, nID)          AUTO_RUN_ONCE___(func, _##name##_##nID)

#endif // ! INCLUDE_UTIL_AUTORUN_HPP
