
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/RcfSession.hpp>
#include <RCF/SerializationProtocol.hpp>

#include <boost/bind.hpp>

namespace RCF {

    RcfSession::RcfSession() :
        mFiltered(RCF_DEFAULT_INIT),
        mRcfRuntimeVersion(gRcfRuntimeVersion),
        mStopCallInProgress(RCF_DEFAULT_INIT)
    {}

    RcfSession::~RcfSession()
    {
        // no locks here, relying on dtor thread safety of reference counted objects
        if (mOnDestroyCallback)
        {
            mOnDestroyCallback(*this);
        }
    }

    void RcfSession::setOnDestroyCallback(OnDestroyCallback onDestroyCallback)
    {
        Lock lock(mMutex);
        mOnDestroyCallback = onDestroyCallback;
    }

#ifdef RCF_USE_SF_SERIALIZATION

    void RcfSession::enableSfSerializationPointerTracking(bool enable)
    {
        mOut.mOutProtocol1.setCustomizationCallback(
            boost::bind(enableSfPointerTracking_1, _1, enable) );

        //mOut.mOutProtocol2.setCustomizationCallback(
        //    boost::bind(enableSfPointerTracking_2, _1, enable) );
    }

#else

    void RcfSession::enableSfSerializationPointerTracking(bool enable)
    {}

#endif

    void RcfSession::addOnWriteCompletedCallback(
        const OnWriteCompletedCallback &onWriteCompletedCallback)
    {
        Lock lock(mMutex);
        mOnWriteCompletedCallbacks.push_back(onWriteCompletedCallback);
    }

    void RcfSession::addOnWriteInitiatedCallback(
        const OnWriteInitiatedCallback &onWriteInitiatedCallback)
    {
        Lock lock(mMutex);
        mOnWriteInitiatedCallbacks.push_back(onWriteInitiatedCallback);
    }

    void RcfSession::extractOnWriteCompletedCallbacks(
        std::vector<OnWriteCompletedCallback> &onWriteCompletedCallbacks)
    {
        Lock lock(mMutex);
        onWriteCompletedCallbacks.clear();
        onWriteCompletedCallbacks.swap( mOnWriteCompletedCallbacks );
    }

    void RcfSession::extractOnWriteInitiatedCallbacks(
        std::vector<OnWriteInitiatedCallback> &onWriteInitiatedCallbacks)
    {
        Lock lock(mMutex);
        onWriteInitiatedCallbacks.clear();
        onWriteInitiatedCallbacks.swap( mOnWriteInitiatedCallbacks );
    }

    const RCF::I_RemoteAddress &RcfSession::getRemoteAddress()
    {
        return getProactorPtr()->getRemoteAddress();
    }

    bool RcfSession::hasDefaultServerStub()
    {
        Lock lock(mMutex);
        return mDefaultStubEntryPtr;
    }

    StubEntryPtr RcfSession::getDefaultStubEntryPtr()
    {
        Lock lock(mMutex);
        return mDefaultStubEntryPtr;
    }

    void RcfSession::setDefaultStubEntryPtr(const StubEntryPtr &stubEntryPtr)
    {
        Lock lock(mMutex);
        mDefaultStubEntryPtr = stubEntryPtr;
    }

    void RcfSession::setCachedStubEntryPtr(const StubEntryPtr &stubEntryPtr)
    {
        mCachedStubEntryPtr = stubEntryPtr;
    }

    const std::vector<FilterPtr> &RcfSession::getMessageFilters()
    {
        return mFilters;
    }

    const std::vector<FilterPtr> &RcfSession::getTransportFilters()
    {
        return getProactorPtr()->getTransportFilters();
    }

    int RcfSession::getRcfRuntimeVersion()
    {
        return mRcfRuntimeVersion;
    }

    void RcfSession::setRcfRuntimeVersion(int version)
    {
        mRcfRuntimeVersion = version;
    }

    void RcfSession::setUserData(boost::any userData)
    {
        mUserData = userData;
    }

    boost::any RcfSession::getUserData()
    {
        return mUserData;
    }

} // namespace RCF
