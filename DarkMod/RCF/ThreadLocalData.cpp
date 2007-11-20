
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ThreadLocalData.hpp>

#include <RCF/ByteBuffer.hpp>
#include <RCF/InitDeinit.hpp>

namespace RCF {

    class ThreadLocalData
    {
    public:
        ObjectCache     mObjectCache;
        ClientStubPtr   mCurrentClientStubPtr;
        RcfSessionPtr   mCurrentRcfSessionPtr;
        ThreadInfoPtr   mThreadInfoPtr;
        SessionPtr      mUdpSessionPtr;

        void clear()
        {
            mObjectCache.clear();
            mCurrentClientStubPtr.reset();
            mCurrentRcfSessionPtr.reset();
            mThreadInfoPtr.reset();
            mUdpSessionPtr.reset();
        }
    };

    typedef ThreadSpecificPtr<ThreadLocalData>::Val ThreadLocalDataPtr;

    ThreadLocalDataPtr *pThreadLocalDataPtr = NULL;

    ThreadLocalData &getThreadLocalData()
    {
        if (NULL == pThreadLocalDataPtr->get())
        {
            pThreadLocalDataPtr->reset( new ThreadLocalData());
        }
        return *(*pThreadLocalDataPtr);
    }

    // Solaris 10 on x86 crashes when trying to delete the thread specific pointer
#if defined(sun) || defined(__sun) || defined(__sun__)

    RCF_ON_INIT_NAMED(if (!pThreadLocalDataPtr) pThreadLocalDataPtr = new ThreadLocalDataPtr; , ThreadLocalDataInit)
    //RCF_ON_DEINIT_NAMED( (*pThreadLocalDataPtr)->clear(); , ThreadLocalDataDeinit)

#else

    RCF_ON_INIT_NAMED(pThreadLocalDataPtr = new ThreadLocalDataPtr;, ThreadLocalDataInit)
    RCF_ON_DEINIT_NAMED( delete pThreadLocalDataPtr; pThreadLocalDataPtr = NULL; , ThreadLocalDataDeinit)

#endif

    // access to the various thread local entities

    ObjectCache &getThreadLocalObjectCache()
    {
        return getThreadLocalData().mObjectCache;
    }

    ClientStubPtr getCurrentClientStubPtr()
    {
        return getThreadLocalData().mCurrentClientStubPtr;
    }

    void setCurrentClientStubPtr(ClientStubPtr clientStubPtr)
    {
        getThreadLocalData().mCurrentClientStubPtr = clientStubPtr;
    }

    RcfSessionPtr getCurrentRcfSessionPtr()
    {
        return getThreadLocalData().mCurrentRcfSessionPtr;
    }

    void setCurrentRcfSessionPtr(const RcfSessionPtr &rcfSessionPtr)
    {
        getThreadLocalData().mCurrentRcfSessionPtr = rcfSessionPtr;
    }

    ThreadInfoPtr getThreadInfoPtr()
    {
        return getThreadLocalData().mThreadInfoPtr;
    }

    void setThreadInfoPtr(const ThreadInfoPtr &threadInfoPtr)
    {
        getThreadLocalData().mThreadInfoPtr = threadInfoPtr;
    }

    SessionPtr getCurrentUdpSessionPtr()
    {
        return getThreadLocalData().mUdpSessionPtr;
    }

    void setCurrentUdpSessionPtr(SessionPtr sessionPtr)
    {
        getThreadLocalData().mUdpSessionPtr = sessionPtr;
    }

} // namespace RCF
