
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_THREADLIBRARY_HPP
#define INCLUDE_RCF_THREADLIBRARY_HPP

#include <boost/shared_ptr.hpp>

#include <RCF/util/ThreadLibrary.hpp>

namespace RCF {

    typedef util::Mutex                         Mutex;
    typedef util::Lock                          Lock;

    typedef util::TryMutex                      TryMutex;
    typedef util::TryLock                       TryLock;

    typedef util::ReadWriteMutex                ReadWriteMutex;
    typedef util::ReadLock                      ReadLock;
    typedef util::WriteLock                     WriteLock;

    static const Platform::Threads::read_write_scheduling_policy ReaderPriority = Platform::Threads::reader_priority;
    static const Platform::Threads::read_write_scheduling_policy WriterPriority = Platform::Threads::writer_priority;

    typedef util::Thread                        Thread;

    typedef util::Condition                     Condition;

    template<typename T>
    struct ThreadSpecificPtr 
    {
        // vc6 was choking on the following line
        //typedef typename util::ThreadSpecificPtr<T>::Val Val;
        typedef typename Platform::Threads::thread_specific_ptr<T>::Val Val;
    };

    typedef boost::shared_ptr<Thread>           ThreadPtr;
    typedef boost::shared_ptr<ReadWriteMutex>   ReadWriteMutexPtr;
    typedef boost::shared_ptr<Mutex>            MutexPtr;
    typedef std::auto_ptr<Mutex>                MutexAutoPtr;
    typedef boost::shared_ptr<Lock>             LockPtr;

} // namespace RCF

#endif // ! INCLUDE_RCF_THREADLIBRARY_HPP
