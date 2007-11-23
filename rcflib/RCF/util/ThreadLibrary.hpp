
#ifndef INCLUDE_UTIL_THREADLIBRARY_HPP
#define INCLUDE_UTIL_THREADLIBRARY_HPP

#ifdef RCF_MULTI_THREADED
#include "Platform/Threads/BoostThreads.hpp"
#else
#include "Platform/Threads/ThreadsProxy.hpp"
#endif

namespace util {

    typedef Platform::Threads::mutex Mutex;
    typedef Platform::Threads::mutex::scoped_lock Lock;

    typedef Platform::Threads::try_mutex TryMutex;
    typedef Platform::Threads::try_mutex::scoped_try_lock TryLock;

    typedef Platform::Threads::read_write_mutex ReadWriteMutex;
    typedef Platform::Threads::read_write_mutex::scoped_read_lock ReadLock;
    typedef Platform::Threads::read_write_mutex::scoped_write_lock WriteLock;

    static const Platform::Threads::read_write_scheduling_policy ReaderPriority = Platform::Threads::reader_priority;
    static const Platform::Threads::read_write_scheduling_policy WriterPriority = Platform::Threads::writer_priority;

    typedef Platform::Threads::thread Thread;

    typedef Platform::Threads::condition Condition;

    template<typename T>
    struct ThreadSpecificPtr 
    {
        typedef typename Platform::Threads::thread_specific_ptr<T>::Val Val;
    };

} // namespace util

#endif // ! INCLUDE_UTIL_THREADLIBRARY_HPP
