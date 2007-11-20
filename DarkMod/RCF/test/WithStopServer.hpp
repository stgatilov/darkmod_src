//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TEST_WITHSTOPSERVER_HPP
#define INCLUDE_RCF_TEST_WITHSTOPSERVER_HPP

#include <boost/bind.hpp>

#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    class WithStopServer
    {
    public:
        WithStopServer()
        {
            RCF::Thread t( boost::bind(&WithStopServer::setLock, this));
            t.join();
        }

        void stopServer()
        {
            mLockPtr->unlock();
        }

        void wait()
        {
            RCF::Lock lock(mMutex);
        }

    private:

        void setLock()
        {
            mLockPtr.reset( new RCF::Lock(mMutex));
        }

        RCF::Mutex mMutex;
        RCF::LockPtr mLockPtr;

    };

} // namespace RCF

#endif // ! INCLUDE_RCF_TEST_WITHSTOPSERVER_HPP

