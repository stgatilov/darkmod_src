
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/ObjectFactoryService.hpp>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/RcfServer.hpp>
#include <RCF/ServerInterfaces.hpp>
#include <RCF/StubEntry.hpp>
#include <RCF/StubFactory.hpp>

namespace RCF {

#if defined(_MSC_VER) && _MSC_VER <= 1200
#define for if (0) {} else for
#endif

    ObjectFactoryService::TokenFactory::TokenFactory(int tokenCount) :
        mMutex(WriterPriority)
    {
        for (int i=0; i<tokenCount; i++)
        {
            mTokenSpace.push_back( Token(i+1) );
        }

#if defined(_MSC_VER) && _MSC_VER < 1310 && !(defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION))

        //for (std::size_t i = mTokenSpace.size() - 1; i >= 0; --i)
        //{
        //    mAvailableTokens.push_back(mTokenSpace[i]);
        //}

        std::for_each(
            mTokenSpace.rbegin(),
            mTokenSpace.rend(),
            boost::bind(
                &std::vector<Token>::push_back,
                boost::ref(mAvailableTokens),
                _1));

#else

        mAvailableTokens.assign( mTokenSpace.rbegin(), mTokenSpace.rend() );

#endif

    }

#if defined(_MSC_VER) && _MSC_VER <= 1200
#undef for
#endif

    bool ObjectFactoryService::TokenFactory::requestToken(Token &token)
    {
        WriteLock writeLock(mMutex);
        RCF_UNUSED_VARIABLE(writeLock);
        if (mAvailableTokens.empty())
        {
            RCF_TRACE("no more tokens available")
                (mAvailableTokens.size())(mTokenSpace.size());
            return false;
        }
        else
        {
            Token myToken = mAvailableTokens.back();
            mAvailableTokens.pop_back();
            token = myToken;
            return true;
        }
    }

    void ObjectFactoryService::TokenFactory::returnToken(const Token &token)
    {
        WriteLock writeLock(mMutex);
        RCF_UNUSED_VARIABLE(writeLock);
        mAvailableTokens.push_back(token);
    }

    const std::vector<Token> &ObjectFactoryService::TokenFactory::getTokenSpace()
    {
        return mTokenSpace;
    }

    std::size_t ObjectFactoryService::TokenFactory::getAvailableTokenCount()
    {
        ReadLock readLock( mMutex );
        RCF_UNUSED_VARIABLE(readLock);
        return mAvailableTokens.size();
    }

    ObjectFactoryService::ObjectFactoryService(
        unsigned int numberOfTokens,
        unsigned int clientStubTimeoutS,
        unsigned int cleanupIntervalS,
        float cleanupThreshold) :
        	mTokenFactory(numberOfTokens),
            mClientStubTimeoutS(clientStubTimeoutS),
            mCleanupIntervalS(cleanupIntervalS),
            mCleanupThreshold(cleanupThreshold),
            mStubMapMutex(WriterPriority),
            mStubFactoryMapMutex(WriterPriority),
            mStopFlag(RCF_DEFAULT_INIT)
    {
        RCF_ASSERT(0.0 <= cleanupThreshold && mCleanupThreshold <= 1.0);

        // up-front initialization, before threads get into the picture
        typedef std::vector<Token>::const_iterator Iter;
        for (
            Iter iter = mTokenFactory.getTokenSpace().begin();
            iter != mTokenFactory.getTokenSpace().end();
            ++iter)
        {
            mStubMap[*iter].first.reset(new Mutex());
        }

    }

    // remotely accessible
    boost::int32_t ObjectFactoryService::createObject(
        const std::string &objectName,
        Token &token)
    {
        RCF_TRACE("")(objectName);

        // TODO: seems unnecessary to be triggering a sweep here
        std::size_t nAvail = mTokenFactory.getAvailableTokenCount();
        std::size_t nTotal = mTokenFactory.getTokenSpace().size();
        float used = float(nTotal - nAvail) / float(nTotal);
        if (used > mCleanupThreshold)
        {
            mCleanupThresholdCondition.notify_one();
        }

        boost::int32_t ret = RcfError_Ok;


        StubFactoryPtr stubFactoryPtr( getStubFactory( objectName));
        if (stubFactoryPtr.get())
        {
            // TODO: exception safety
            RcfClientPtr rcfClientPtr( stubFactoryPtr->makeServerStub());
            Token myToken;
            bool ok = mTokenFactory.requestToken(myToken);
            if (ok)
            {
                WriteLock writeLock(mStubMapMutex);
                RCF_UNUSED_VARIABLE(writeLock);
                RCF_ASSERT(mStubMap.find(myToken) != mStubMap.end())(token);
                StubEntryPtr stubEntryPtr( new StubEntry(rcfClientPtr));
                mStubMap[myToken].second = stubEntryPtr;
                getCurrentRcfSessionPtr()->setCachedStubEntryPtr(stubEntryPtr);
                token = myToken;
            }
            else
            {
                ret = RcfError_TokenRequestFailed;
            }
        }
        else
        {
            ret = RcfError_ObjectFactoryNotFound;
        }

        return ret;
    }

    // remotely accessible
    boost::int32_t ObjectFactoryService::createSessionObject(
        const std::string &objectName)
    {
        StubFactoryPtr stubFactoryPtr( getStubFactory( objectName));
        if (stubFactoryPtr.get())
        {
            RcfClientPtr rcfClientPtr( stubFactoryPtr->makeServerStub());
            getCurrentRcfSessionPtr()->setDefaultStubEntryPtr(
                StubEntryPtr( new StubEntry(rcfClientPtr)));
            return RcfError_Ok;
        }
        return RcfError_Unspecified;
    }

    // remotely accessible
    boost::int32_t ObjectFactoryService::deleteObject(const Token &token)
    {
        WriteLock writeLock(mStubMapMutex);
        RCF_UNUSED_VARIABLE(writeLock);

        if (mStubMap.find(token) == mStubMap.end())
        {
            return RcfError_Unspecified;
        }
        else
        {
            mStubMap.erase(token);
            mTokenFactory.returnToken(token);
            RCF_TRACE("Token returned")(token);
            return RcfError_Ok;
        }
    }

    // remotely accessible
    boost::int32_t ObjectFactoryService::deleteSessionObject()
    {
        getCurrentRcfSessionPtr()->setDefaultStubEntryPtr(StubEntryPtr());
        return RcfError_Ok;
    }

    StubEntryPtr ObjectFactoryService::getStubEntryPtr(const Token &token)
    {
        ReadLock readLock(mStubMapMutex);
        //RCF_ASSERT(mStubMap.find(token) != mStubMap.end())(token);
        RCF_VERIFY(
            mStubMap.find(token) != mStubMap.end(),
            Exception(RcfError_DynamicObjectNotFound))
            (token);

        Lock lock(*mStubMap[token].first);
        return StubEntryPtr(mStubMap[token].second);
    }
/*
    void ObjectFactoryService::removeStubEntryPtr(const Token &token)
    {
        ReadLock readLock(mStubMapMutex);
        RCF_ASSERT(mStubMap.find(token) != mStubMap.end())(token);
        Lock lock(*mStubMap[token].first);
        mStubMap[token].second = StubEntryPtr();
    }
*/
    void ObjectFactoryService::onServiceAdded(RcfServer &server)
    {
        server.bind((I_ObjectFactory *) NULL, *this);
        {
            WriteLock writeLock(getTaskEntriesMutex());
            getTaskEntries().clear();
            getTaskEntries().push_back(
                TaskEntry(
                boost::bind(&ObjectFactoryService::cycleCleanup, this, _1, _2),
                boost::bind(&ObjectFactoryService::stopCleanup, this),
                "RCF OFS cleanup"));
        }
        mStopFlag = false;
    }

    void ObjectFactoryService::onServiceRemoved(RcfServer &server)
    {
        server.unbind( (I_ObjectFactory *) NULL);
    }

    void ObjectFactoryService::onServerStart(RcfServer &)
    {
    }

    void ObjectFactoryService::onServerStop(RcfServer &)
    {
        mStopFlag = false;
    }

    void ObjectFactoryService::stopCleanup()
    {
        mStopFlag = true;
        Lock lock(mCleanupThresholdMutex);
        mCleanupThresholdCondition.notify_one();
    }

    bool ObjectFactoryService::cycleCleanup(
        int timeoutMs,
        const volatile bool &stopFlag)
    {
        if (timeoutMs == 0)
        {
            cleanupStubMap(mClientStubTimeoutS);
        }
        else
        {
            Lock lock(mCleanupThresholdMutex);
            if (!stopFlag && !mStopFlag)
            {
                unsigned int cleanupIntervalMs = 1000*mCleanupIntervalS;
                mCleanupThresholdCondition.timed_wait(lock, cleanupIntervalMs);
                if (!stopFlag && !mStopFlag)
                {
                    cleanupStubMap(mClientStubTimeoutS);
                }
                else
                {
                    return true;
                }
            }
        }
        return stopFlag || mStopFlag;
    }

    bool ObjectFactoryService::insertStubFactory(
        const std::string &objectName,
        const std::string &desc,
        StubFactoryPtr stubFactoryPtr)
    {
        RCF_UNUSED_VARIABLE(desc);
        WriteLock writeLock(mStubFactoryMapMutex);
        mStubFactoryMap[ objectName ] = stubFactoryPtr;
        return true;
    }

    bool ObjectFactoryService::removeStubFactory(const std::string &objectName)
    {
        WriteLock writeLock(mStubFactoryMapMutex);
        mStubFactoryMap.erase(mStubFactoryMap.find(objectName));
        return true;
    }

    StubFactoryPtr ObjectFactoryService::getStubFactory(
        const std::string &objectName)
    {
        ReadLock readLock(mStubFactoryMapMutex);
        return mStubFactoryMap.find(objectName)  != mStubFactoryMap.end() ?
            mStubFactoryMap[objectName] :
            StubFactoryPtr();
    }

    void ObjectFactoryService::cleanupStubMap(unsigned int timeoutS)
    {
        // Clean up the stub map
        RCF_TRACE("");
        std::size_t nAvail = mTokenFactory.getAvailableTokenCount();
        std::size_t nTotal = mTokenFactory.getTokenSpace().size();
        float used = float(nTotal - nAvail) / float(nTotal);
        if (used > mCleanupThreshold)
        {
            typedef std::vector<Token>::const_iterator Iter;
            for (
                Iter iter = mTokenFactory.getTokenSpace().begin();
                iter != mTokenFactory.getTokenSpace().end();
                ++iter)
            {
                Token token = *iter;

                bool removeStub = false;
                {
                    ReadLock readLock(mStubMapMutex);
                    RCF_ASSERT(mStubMap.find(token) != mStubMap.end())(token);
                    Lock lock(*mStubMap[token].first);
                    StubEntryPtr &stubEntryPtr = mStubMap[token].second;
                    if (
                        stubEntryPtr.get() &&
                        stubEntryPtr.unique() &&
                        stubEntryPtr->getElapsedTimeS() > timeoutS)
                    {
                        removeStub = true;
                        stubEntryPtr.reset();
                    }
                }
                if (removeStub)
                {
                    mTokenFactory.returnToken(token);
                    RCF_TRACE("Token returned")(token);
                }
            }
        }
    }

    namespace {

        void reinstateClientTransport(
            ClientStub &clientStub,
            I_RcfClient &factory)
        {
            clientStub.setTransport(factory.getClientStub().releaseTransport());
        }

    }

    void ClientStub::createRemoteObject(
        const std::string &objectName_)
    {
        const std::string &objectName = objectName_.empty() ? mInterfaceName : objectName_;
        unsigned int timeoutMs = getRemoteCallTimeoutMs();
        connect();
        RcfClient<I_ObjectFactory> factory(*this);
        factory.getClientStub().setTransport( releaseTransport());
        factory.getClientStub().setTargetToken( Token());
        // TODO: should only be using the remainder of the timeout
        factory.getClientStub().setRemoteCallTimeoutMs(timeoutMs);
        using namespace boost::multi_index::detail;
        scope_guard guard = make_guard(
            reinstateClientTransport,
            boost::ref(*this),
            boost::ref(factory));
        RCF_UNUSED_VARIABLE(guard);
        RCF::Token token;
        boost::int32_t ret = factory.createObject(RCF::Twoway, objectName, token);
        if (ret == RcfError_Ok)
        {
            setTargetToken(token);
        }
        else
        {
            setTargetToken(Token());

            // dtor issues with borland
#ifdef __BORLANDC__
            setTransport(factory.getClientStub().releaseTransport());
            guard.dismiss();
#endif

            RCF_THROW( RemoteException(ret));
        }
    }

    void ClientStub::createRemoteSessionObject(
        const std::string &objectName_)
    {
        const std::string &objectName = objectName_.empty() ? mInterfaceName : objectName_;
        unsigned int timeoutMs = getRemoteCallTimeoutMs();
        connect();
        RcfClient<I_ObjectFactory> factory(*this);
        factory.getClientStub().setTransport( releaseTransport());
        factory.getClientStub().setTargetToken( Token());
        // TODO: should only be using the remainder of the timeout
        factory.getClientStub().setRemoteCallTimeoutMs(timeoutMs);
        using namespace boost::multi_index::detail;
        scope_guard guard = make_guard(
            reinstateClientTransport,
            boost::ref(*this),
            boost::ref(factory));
        RCF_UNUSED_VARIABLE(guard);

        boost::int32_t ret = factory.createSessionObject(RCF::Twoway, objectName);
        if (ret == RcfError_Ok)
        {
            setTargetName("");
            setTargetToken(Token());
        }
        else
        {
            RCF_THROW( RemoteException(ret));
        }
    }

    void ClientStub::deleteRemoteSessionObject()
    {
        RcfClient<I_ObjectFactory> factory(*this);
        factory.getClientStub().setTransport( releaseTransport());
        factory.getClientStub().setTargetToken( Token());
        using namespace boost::multi_index::detail;
        scope_guard guard = make_guard(
            reinstateClientTransport,
            boost::ref(*this),
            boost::ref(factory));
        RCF_UNUSED_VARIABLE(guard);

        boost::int32_t ret = factory.deleteSessionObject(RCF::Twoway);
        RCF_VERIFY(ret == RcfError_Ok, RCF::RemoteException(ret));
    }

    void ClientStub::deleteRemoteObject()
    {
        Token token = getTargetToken();
        if (token != Token())
        {
            RcfClient<I_ObjectFactory> factory(*this);
            factory.getClientStub().setTransport( releaseTransport());
            factory.getClientStub().setTargetToken( Token());
            using namespace boost::multi_index::detail;
            scope_guard guard = make_guard(
                reinstateClientTransport,
                boost::ref(*this),
                boost::ref(factory));
            RCF_UNUSED_VARIABLE(guard);

            boost::int32_t ret = factory.deleteObject(RCF::Twoway, token);
            RCF_VERIFY(ret == RcfError_Ok, RCF::RemoteException(ret));
        }
    }

} // namespace RCF
