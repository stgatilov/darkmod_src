
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_RCFSESSION_HPP
#define INCLUDE_RCF_RCFSESSION_HPP

#include <vector>

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/MethodInvocation.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/ServerTransport.hpp>
#include <RCF/StubEntry.hpp>

namespace RCF {

    class Filter;

    typedef boost::shared_ptr<Filter> FilterPtr;

    class RcfSession;

    typedef boost::shared_ptr<RcfSession> RcfSessionPtr;

    class RcfSession : public I_Session
    {
    public:
        RcfSession();
        ~RcfSession();

        typedef boost::function1<void, RcfSession&> OnWriteCompletedCallback;
        typedef boost::function1<void, RcfSession&> OnWriteInitiatedCallback;
        typedef boost::function1<void, RcfSession&> OnDestroyCallback;

        //*******************************
        // callback tables - synchronized

        // may well be called on a different thread than the one that executed the remote call
        void addOnWriteCompletedCallback(
            const OnWriteCompletedCallback &onWriteCompletedCallback);

        void extractOnWriteCompletedCallbacks(
            std::vector<OnWriteCompletedCallback> &onWriteCompletedCallbacks);

        // called on same thread that executed the remote call
        void addOnWriteInitiatedCallback(
            const OnWriteInitiatedCallback &onWriteInitiatedCallback);

        void extractOnWriteInitiatedCallbacks(
            std::vector<OnWriteInitiatedCallback> &onWriteInitiatedCallbacks);

        void setOnDestroyCallback(OnDestroyCallback onDestroyCallback);

        //*******************************

        const RCF::I_RemoteAddress &getRemoteAddress();
        bool hasDefaultServerStub();
        StubEntryPtr getDefaultStubEntryPtr();
        void setDefaultStubEntryPtr(const StubEntryPtr &stubEntryPtr);
        void setCachedStubEntryPtr(const StubEntryPtr &stubEntryPtr);

        // protocol-opaque serialization
        SerializationProtocolIn mIn;
        SerializationProtocolOut mOut;

        // payload filters
        std::vector<FilterPtr> mFilters;
        bool mFiltered;

        /// Enables pointer tracking for outbound SF serialization on this session.
        /// \parameter Whether to enable or not.
        void enableSfSerializationPointerTracking(bool enable);

        MethodInvocationRequest mRequest;

        int getRcfRuntimeVersion();
        void setRcfRuntimeVersion(int version);

        void setUserData(boost::any userData);
        boost::any getUserData();

    private:

        StubEntryPtr mDefaultStubEntryPtr;
        StubEntryPtr mCachedStubEntryPtr;

        Mutex mMutex;
        std::vector<OnWriteCompletedCallback> mOnWriteCompletedCallbacks;
        std::vector<OnWriteInitiatedCallback> mOnWriteInitiatedCallbacks;
        OnDestroyCallback mOnDestroyCallback;

        int mRcfRuntimeVersion;

        boost::any mUserData;

    public:
        Mutex mStopCallInProgressMutex;
        bool mStopCallInProgress;

    public:
        const std::vector<FilterPtr> &getMessageFilters();
        const std::vector<FilterPtr> &getTransportFilters();
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_RCFSESSION_HPP
