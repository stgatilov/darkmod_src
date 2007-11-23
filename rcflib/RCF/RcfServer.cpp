
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/RcfServer.hpp>

#include <algorithm>

#include <boost/bind.hpp>

#include <RCF/AsyncFilter.hpp>
#include <RCF/CurrentSession.hpp>
#include <RCF/Endpoint.hpp>
#include <RCF/MethodInvocation.hpp>
#include <RCF/RcfClient.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/ServerTask.hpp>
#include <RCF/Service.hpp>
#include <RCF/StubEntry.hpp>
#include <RCF/Token.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    void repeatCycleServer(RcfServer &server, int timeoutMs)
    {
        RCF_TRACE("");
        while (!server.cycle(timeoutMs));
        RCF_TRACE("");
    }

    // RcfServer

    RcfServer::RcfServer() :
    	mServerThreadsStopFlag(RCF_DEFAULT_INIT),
        mOpened(RCF_DEFAULT_INIT),
        mStarted(RCF_DEFAULT_INIT),
        mStubMapMutex(WriterPriority),
        mServicesMutex(WriterPriority),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
        RCF_TRACE("");
    }

    RcfServer::RcfServer(const I_Endpoint &endpoint) :
    	mServerThreadsStopFlag(RCF_DEFAULT_INIT),
    	mOpened(RCF_DEFAULT_INIT),
        mStarted(RCF_DEFAULT_INIT),
        mStubMapMutex(WriterPriority),
        mServicesMutex(WriterPriority),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
        RCF_TRACE("");

        ServerTransportPtr serverTransportPtr(
            endpoint.createServerTransport().release());

        ServicePtr servicePtr(
            boost::dynamic_pointer_cast<I_Service>(serverTransportPtr) );

        addService(servicePtr);
    }

    RcfServer::RcfServer(const ServicePtr &servicePtr) :
        mServerThreadsStopFlag(RCF_DEFAULT_INIT),
        mOpened(RCF_DEFAULT_INIT),
        mStarted(RCF_DEFAULT_INIT),
        mStubMapMutex(WriterPriority),
        mServicesMutex(WriterPriority),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
        RCF_TRACE("");

        addService(servicePtr);
    }

    RcfServer::RcfServer(const ServerTransportPtr &serverTransportPtr) :
        mServerThreadsStopFlag(RCF_DEFAULT_INIT),
        mOpened(RCF_DEFAULT_INIT),
        mStarted(RCF_DEFAULT_INIT),
        mStubMapMutex(WriterPriority),
        mServicesMutex(WriterPriority),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
        RCF_TRACE("");

        addService( boost::dynamic_pointer_cast<I_Service>(serverTransportPtr) );
    }

    RcfServer::RcfServer(std::vector<ServerTransportPtr> serverTransports) :
        mServerThreadsStopFlag(RCF_DEFAULT_INIT),
        mOpened(RCF_DEFAULT_INIT),
        mStarted(RCF_DEFAULT_INIT),
        mStubMapMutex(WriterPriority),
        mServicesMutex(WriterPriority),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
        RCF_TRACE("");

        std::for_each(serverTransports.begin(), serverTransports.end(),
            boost::bind(&RcfServer::addServerTransport, this, _1));
    }

    RcfServer::RcfServer(std::vector<ServicePtr> services) :
    	mServerThreadsStopFlag(RCF_DEFAULT_INIT),
        mOpened(RCF_DEFAULT_INIT),
        mStarted(RCF_DEFAULT_INIT),
        mStubMapMutex(WriterPriority),
        mServicesMutex(WriterPriority),
        mRcfRuntimeVersion(gRcfRuntimeVersion)
    {
        RCF_TRACE("");

        std::for_each(services.begin(), services.end(),
            boost::bind(&RcfServer::addService, this, _1));
    }

    RcfServer::~RcfServer()
    {
        RCF_DTOR_BEGIN
            RCF_TRACE("");
            close();
        RCF_DTOR_END
    }

    bool RcfServer::addService(const ServicePtr &servicePtr)
    {
        RCF_TRACE("")(typeid(*servicePtr).name());

        bool ret = false;
        {
            WriteLock writeLock(mServicesMutex);
            if (
                std::find(
                    mServices.begin(),
                    mServices.end(),
                    servicePtr) == mServices.end())
            {
                mServices.push_back(servicePtr);
                ret = true;

                StubEntryLookupProviderPtr stubEntryLookupProviderPtr =
                    boost::dynamic_pointer_cast<I_StubEntryLookupProvider>(servicePtr);

                if (stubEntryLookupProviderPtr)
                {
                    mStubEntryLookupProviders.push_back(stubEntryLookupProviderPtr);
                }

                FilterFactoryLookupProviderPtr filterFactoryLookupProviderPtr =
                    boost::dynamic_pointer_cast<I_FilterFactoryLookupProvider>(servicePtr);

                if (filterFactoryLookupProviderPtr)
                {
                    mFilterFactoryLookupProviders.push_back(filterFactoryLookupProviderPtr);
                }

                ServerTransportPtr serverTransportPtr =
                    boost::dynamic_pointer_cast<I_ServerTransport>(servicePtr);

                if (serverTransportPtr)
                {
                    mServerTransports.push_back(serverTransportPtr);
                }

            }
        }
        if (ret)
        {
            servicePtr->onServiceAdded(*this);
        }
        {
            Lock lock(mStartedMutex);
            if (mStarted)
            {
                startService(servicePtr);
            }
        }
        return ret;
    }

    bool RcfServer::removeService(const ServicePtr &servicePtr)
    {
        RCF_TRACE("")(typeid(*servicePtr).name());

        bool found = false;
        {
            WriteLock writeLock(mServicesMutex);
            std::vector<ServicePtr>::iterator iter =
                std::find(mServices.begin(), mServices.end(), servicePtr);

            if (iter != mServices.end())
            {
                stopService(*iter);
                mServices.erase(iter);
                found = true;
            }
        }
        if (found)
        {
            servicePtr->onServiceRemoved(*this);
        }
        return found;
    }

    bool RcfServer::addServerTransport(const ServerTransportPtr &serverTransportPtr)
    {
        return addService(
            boost::dynamic_pointer_cast<I_Service>(serverTransportPtr));
    }

    bool RcfServer::removeServerTransport(const ServerTransportPtr &serverTransportPtr)
    {
        return removeService(
            boost::dynamic_pointer_cast<I_Service>(serverTransportPtr));
    }

    void RcfServer::open()
    {
        RCF_TRACE("");

        Lock lock(mOpenedMutex);
        if (!mOpened)
        {
            std::vector<ServicePtr> services;
            {
                ReadLock readLock(mServicesMutex);
                std::copy(
                    mServices.begin(),
                    mServices.end(),
                    std::back_inserter(services));
            }

            std::for_each(
                services.begin(),
                services.end(),
                boost::bind(&I_Service::onServerOpen, _1, boost::ref(*this)));

            mOpened = true;
        }
    }

    void RcfServer::start(bool spawnThreads)
    {
        RCF_TRACE("");

        Lock lock(mStartedMutex);
        if (!mStarted)
        {
            mServerThreadsStopFlag = false;

            // open the server
            open();

            // make a local copy of the service table
            std::vector<ServicePtr> services;
            {
                ReadLock readLock(mServicesMutex);
                std::copy(
                    mServices.begin(),
                    mServices.end(),
                    std::back_inserter(services));
            }

            // notify all services
            std::for_each(
                services.begin(),
                services.end(),
                boost::bind(&I_Service::onServerStart, _1, boost::ref(*this)));

            // spawn internal worker threads
            if (spawnThreads)
            {
                std::for_each(
                    services.begin(),
                    services.end(),
                    boost::bind(&RcfServer::startService, boost::cref(*this), _1));
            }

            mStarted = true;

            // call the start notification callback, if there is one
            invokeStartCallback();

            // notify anyone who was waiting on the stop event
            mStartEvent.notify_all();
        }
    }

    void RcfServer::addJoinFunctor(const JoinFunctor &joinFunctor)
    {
        if (joinFunctor)
        {
            mJoinFunctors.push_back(joinFunctor);
        }
    }

    void RcfServer::startInThisThread()
    {
        startInThisThread(JoinFunctor());
    }

    void RcfServer::startInThisThread(const JoinFunctor &joinFunctor)
    {
        RCF_TRACE("");

        start();

        // register the join functor
        mJoinFunctors.push_back(joinFunctor);

        // run all tasks sequentially in this thread
        repeatCycleServer(*this, 500);

    }

    bool RcfServer::cycle(int timeoutMs)
    {
        RCF_TRACE("")(timeoutMs);

        // sequentially run each task
        // only first task gets to use the timeout
        // if tasks are being dynamically added or removed, a given task might be cycled twice or not at all

        unsigned int i=0;
        while (true)
        {
            ServicePtr servicePtr;
            {
                ReadLock readLock(mServicesMutex);
                if (i < mServices.size())
                {
                    servicePtr = mServices[i];
                }
            }
            if (servicePtr)
            {
                unsigned int j=0;
                while (true)
                {
                    Task task;
                    bool ok = false;
                    {
                        ReadLock readLock(servicePtr->getTaskEntriesMutex());
                        TaskEntries &taskEntries = servicePtr->getTaskEntries();
                        if (j < taskEntries.size())
                        {
                            task = taskEntries[j].getTask();
                            ok = true;
                        }
                    }
                    if (ok)
                    {
                        task(
                            i == 0  && j == 0 ? timeoutMs : 0,
                            mServerThreadsStopFlag,
                            true); //JL
                        ++j;
                    }
                    else
                    {
                        break;
                    }
                }
                ++i;
            }
            else
            {
                break;
            }
        }

        return mServerThreadsStopFlag;
    }

    void RcfServer::startService(const ServicePtr &servicePtr) const
    {
        RCF_TRACE("")(typeid(*servicePtr));

        WriteLock writeLock(servicePtr->getTaskEntriesMutex());
        TaskEntries &taskEntries = servicePtr->getTaskEntries();
        std::for_each(
            taskEntries.begin(),
            taskEntries.end(),
            boost::bind(
                &TaskEntry::start,
                _1,
                boost::ref(mServerThreadsStopFlag)));
    }

    void RcfServer::stopService(const ServicePtr &servicePtr, bool wait) const
    {
        RCF_TRACE("")(typeid(*servicePtr))(wait);

        typedef void (TaskEntry::*Pfn)(bool);

        WriteLock writeLock(servicePtr->getTaskEntriesMutex());
        TaskEntries &taskEntries = servicePtr->getTaskEntries();
        std::for_each(
            taskEntries.rbegin(),
            taskEntries.rend(),
            boost::bind( (Pfn) &TaskEntry::stop, _1, wait));
    }

    void RcfServer::stop(bool wait)
    {
        RCF_TRACE("")(wait);

        Lock lock(mStartedMutex);
        if (mStarted)
        {
            // set stop flag
            mServerThreadsStopFlag = true;

            // make a local copy of the service table
            std::vector<ServicePtr> services;
            {
                ReadLock readLock(mServicesMutex);
                std::copy(
                    mServices.begin(),
                    mServices.end(),
                    std::back_inserter(services));
            }

            // notify and optionally join all internal worker threads
            typedef void (RcfServer::*Pfn)(const ServicePtr &, bool) const;
            std::for_each(
                services.rbegin(),
                services.rend(),
                boost::bind( (Pfn) &RcfServer::stopService, boost::cref(*this), _1, wait));

            if (wait)
            {
                // join all external worker threads
                std::for_each(
                    mJoinFunctors.rbegin(),
                    mJoinFunctors.rend(),
                    boost::bind(&JoinFunctor::operator(), _1));

                mJoinFunctors.clear();

                // notify all services
                std::for_each(
                    services.rbegin(),
                    services.rend(),
                    boost::bind(&I_Service::onServerStop, _1, boost::ref(*this)));

                // clear stop flag, since all the threads have been joined
                mServerThreadsStopFlag = false;

                mStarted = false;

                // notify anyone who was waiting on the stop event
                mStopEvent.notify_all();
            }
        }
    }

    void RcfServer::close()
    {
        RCF_TRACE("");

        Lock lock(mOpenedMutex);
        if (mOpened)
        {
            // stop the server
            stop();

            std::vector<ServicePtr> services;
            {
                ReadLock readLock(mServicesMutex);
                std::copy(
                    mServices.begin(),
                    mServices.end(),
                    std::back_inserter(services));
            }
            std::for_each(
                services.rbegin(),
                services.rend(),
                boost::bind(&I_Service::onServerClose, _1, boost::ref(*this)));

            // set status
            mOpened = false;
        }
    }

    void RcfServer::waitForStopEvent()
    {
        RCF_TRACE("");

        Lock lock(mStartedMutex);
        if (mStarted)
        {
            mStopEvent.wait(lock);
        }
    }

    void RcfServer::waitForStartEvent()
    {
        RCF_TRACE("");

        Lock lock(mStartedMutex);
        if (!mStarted)
        {
            mStartEvent.wait(lock);
        }
    }

    SessionPtr RcfServer::createSession()
    {
        RCF_TRACE("");
        return SessionPtr(new RcfSession());
    }

    void RcfServer::onReadCompleted(const SessionPtr &sessionPtr)
    {
        // 1. Deserialize request data
        // 2. Store request data in session
        // 3. Move session to corresponding queue

        RcfSessionPtr rcfSessionPtr =
            boost::static_pointer_cast<RcfSession>(sessionPtr);

        Lock lock(rcfSessionPtr->mStopCallInProgressMutex);
        if (!rcfSessionPtr->mStopCallInProgress)
        {

            ByteBuffer readByteBuffer =
                rcfSessionPtr->getProactorPtr()->getReadByteBuffer();

            RCF_TRACE("")(sessionPtr.get())(readByteBuffer.getLength());

            bool ok = rcfSessionPtr->mRequest.decodeRequest(
                readByteBuffer,
                rcfSessionPtr,
                *this);

            if (!ok)
            {
                // version mismatch (client is newer than we are)
                // send a control message back to client, with our runtime version

                boost::shared_ptr<std::vector<char> > vecPtr(
                    new std::vector<char>(4+1+1+4+4));

                // without RCF:: qualifiers, borland chooses not to generate any code at all...
                std::size_t pos = 4;
                RCF::encodeInt(Descriptor_Error, *vecPtr, pos);
                RCF::encodeInt(0, *vecPtr, pos);
                RCF::encodeInt(RcfError_VersionMismatch, *vecPtr, pos);
                RCF::encodeInt(mRcfRuntimeVersion, *vecPtr, pos);

                std::vector<ByteBuffer> byteBuffers;

                byteBuffers.push_back( ByteBuffer(
                    &(*vecPtr)[4],
                    pos-4,
                    4,
                    vecPtr));

                rcfSessionPtr->getProactorPtr()->postWrite(byteBuffers);
            }
            else
            {
                if (rcfSessionPtr->mRequest.getClose()) 
                {
                    rcfSessionPtr->getProactorPtr()->postClose();
                }
                else
                {
                    // TODO: downside of calling handleSession() now is that
                    // the stack might already be very deep. Might be better
                    // to unwind the stack first and then call handleSession().
                    handleSession(rcfSessionPtr);
                }
            }
        }
    }

    void RcfServer::onWriteCompleted(const SessionPtr &sessionPtr)
    {
        RCF_TRACE("")(sessionPtr.get());

        RcfSessionPtr rcfSessionPtr = boost::static_pointer_cast<RcfSession>(sessionPtr);
        std::vector<RcfSession::OnWriteCompletedCallback> onWriteCompletedCallbacks;
        rcfSessionPtr->extractOnWriteCompletedCallbacks(onWriteCompletedCallbacks);
        std::for_each(
            onWriteCompletedCallbacks.begin(),
            onWriteCompletedCallbacks.end(),
            boost::bind(
                &RcfSession::OnWriteCompletedCallback::operator(),
                _1,
                boost::ref(*rcfSessionPtr)));

        rcfSessionPtr->mIn.clear();
        rcfSessionPtr->mOut.clear();
        rcfSessionPtr->getProactorPtr()->postRead();
    }

    void RcfServer::sendSessionResponse(
        const RcfSessionPtr &sessionPtr,
        bool isException)
    {
        RcfSessionPtr rcfSessionPtr =
            boost::static_pointer_cast<RcfSession>(sessionPtr);

        rcfSessionPtr->mIn.clearByteBuffer();

        if (rcfSessionPtr->mRequest.getOneway())
        {
            rcfSessionPtr->mOut.extractByteBuffers();

            RCF2_TRACE("oneway call - suppressing response")
                (sessionPtr.get());

            onWriteCompleted(rcfSessionPtr);
        }
        else
        {
            ThreadLocalCached< std::vector<ByteBuffer> > tlcByteBuffers;
            std::vector<ByteBuffer> &byteBuffers = tlcByteBuffers.get();

            rcfSessionPtr->mOut.extractByteBuffers(byteBuffers);
            const std::vector<FilterPtr> &filters = rcfSessionPtr->mFilters;
            ThreadLocalCached< std::vector<ByteBuffer> > tlcEncodedByteBuffers;
            std::vector<ByteBuffer> &encodedByteBuffers = tlcEncodedByteBuffers.get();

            std::vector<FilterPtr> noFilters;
            rcfSessionPtr->mRequest.encodeResponse(
                byteBuffers,
                encodedByteBuffers,
                rcfSessionPtr->mFiltered ? filters : noFilters,
                isException);

            RCF2_TRACE("twoway call - sending response")
                (sessionPtr.get())
                (lengthByteBuffers(byteBuffers))
                (lengthByteBuffers(encodedByteBuffers));

            rcfSessionPtr->getProactorPtr()->postWrite(encodedByteBuffers);
        }
    }

    void RcfServer::closeSession(const RcfSessionPtr &sessionPtr)
    {
        sessionPtr->getProactorPtr()->postClose();
    }

    void RcfServer::serializeSessionExceptionResponse(const RcfSessionPtr &sessionPtr)
    {
        // TODO: test this with a GPF, at least on msvc
        RCF2_TRACE("")(sessionPtr.get());
        sessionPtr->mOut.reset(0);
        serialize(sessionPtr->mOut, RemoteException(RcfError_BadException));
    }

    class HandleSessionGuard
    {
    public:
        HandleSessionGuard(const RcfSessionPtr &sessionPtr)
        {
            setCurrentRcfSessionPtr(sessionPtr);
        }

        ~HandleSessionGuard()
        {
            RCF_DTOR_BEGIN
                typedef std::vector<RcfSession::OnWriteInitiatedCallback>
                    OnWriteInitiatedCallbacks;

                OnWriteInitiatedCallbacks onWriteInitiatedCallbacks;
                getCurrentRcfSessionPtr()->extractOnWriteInitiatedCallbacks(
                    onWriteInitiatedCallbacks);

                std::for_each(
                    onWriteInitiatedCallbacks.begin(),
                    onWriteInitiatedCallbacks.end(),
                    boost::bind(
                        &RcfSession::OnWriteInitiatedCallback::operator(),
                        _1,
                        boost::ref(*getCurrentRcfSessionPtr())));

                setCurrentRcfSessionPtr();
            RCF_DTOR_END
        }
    };

    void RcfServer::handleSession(const RcfSessionPtr &sessionPtr)
    {
        MethodInvocationRequest &request = sessionPtr->mRequest;

        RCF2_TRACE("")
            (sessionPtr.get())
            (request.getService())
            (request.getToken())
            (request.getSubInterface())
            (request.getFnId());

        HandleSessionGuard handleSessionGuard(sessionPtr);

        // locate the server stub entry to call
        StubEntryPtr stubEntryPtr = request.locateStubEntryPtr(*this);

        // make the call

        // NB: the following scopeguard's are apparently not triggered by Borland C++, when throwing non
        // std::exception derived exceptions.

        // C++ standard guarantees order of destruction of following scope guards

        using namespace boost::multi_index::detail;

        bool isException = true;

        scope_guard sendResponseGuard =
            make_obj_guard(
                *this,
                &RcfServer::sendSessionResponse,
                sessionPtr,
                boost::ref(isException));

        RCF_UNUSED_VARIABLE(sendResponseGuard);

        scope_guard serializeExceptionResponseGuard =
            make_obj_guard(
                *this,
                &RcfServer::serializeSessionExceptionResponse,
                sessionPtr) ;

        scope_guard closeSessionGuard =
            make_obj_guard(
                *this,
                &RcfServer::closeSession,
                sessionPtr);

        try
        {
            if (NULL == stubEntryPtr.get())
            {
                RCF_THROW(
                    Exception(RcfError_NoServerStub))
                    (request.getService())(request.getSubInterface())
                    (request.getFnId());
            }
            else
            {
                sessionPtr->setCachedStubEntryPtr(stubEntryPtr);

                stubEntryPtr->touch();
                scope_guard touchGuard =
                    make_obj_guard(*stubEntryPtr, &StubEntry::touch);
                RCF_UNUSED_VARIABLE(touchGuard);

                stubEntryPtr->getRcfClientPtr()->getServerStub().invoke(
                    request.getSubInterface(),
                    request.getFnId(),
                    sessionPtr->mIn,
                    sessionPtr->mOut);

                serializeExceptionResponseGuard.dismiss();
                closeSessionGuard.dismiss();
                isException = false;
            }
        }
        catch(const SerializationException &e)
        {
            RCF_TRACE(": Serialization exception")(typeid(e))(e);
            serializeExceptionResponseGuard.dismiss();
            closeSessionGuard.dismiss();
            sessionPtr->mOut.reset(0);
            serialize(
                sessionPtr->mOut,
                RemoteException(
                    e.getError(),
                    e.what(),
                    e.getContext(),
                    typeid(e).name()));
        }
        catch(const RemoteException &e)
        {
            RCF_TRACE(": User exception")(typeid(e))(e);
            serializeExceptionResponseGuard.dismiss();
            closeSessionGuard.dismiss();
            try
            {
                sessionPtr->mOut.reset(0);
                serialize(sessionPtr->mOut, e);
            }
            catch(const RCF::Exception &e)
            {
                sessionPtr->mOut.reset(0);
                serialize(
                    sessionPtr->mOut,
                    RemoteException(
                        RcfError_Serialization,
                        e.what(),
                        e.getContext(),
                        typeid(e).name()));
            }
            catch(const std::exception &e)
            {
                sessionPtr->mOut.reset(0);
                serialize(
                    sessionPtr->mOut,
                    RemoteException(
                        RcfError_Serialization,
                        e.what(),
                        "",
                        typeid(e).name()));
            }
        }
        catch(const Exception &e)
        {
            RCF_TRACE(": User exception")(typeid(e))(e);
            serializeExceptionResponseGuard.dismiss();
            closeSessionGuard.dismiss();
            sessionPtr->mOut.reset(0);
            serialize(
                sessionPtr->mOut,
                RemoteException(
                    e.getError(),
                    e.getSubSystemError(),
                    e.getSubSystem(),
                    e.what(),
                    e.getContext(),
                    typeid(e).name()));
        }
        catch(const std::exception &e)
        {
            RCF_TRACE(": User exception")(typeid(e))(e);
            serializeExceptionResponseGuard.dismiss();
            closeSessionGuard.dismiss();
            sessionPtr->mOut.reset(0);
            serialize(
                sessionPtr->mOut,
                RemoteException(
                    RcfError_UserModeException,
                    e.what(),
                    "",
                    typeid(e).name()));
        }
    }

    void RcfServer::cycleSessions(int timeoutMs, const volatile bool &stopFlag)
    {
        RCF_UNUSED_VARIABLE(timeoutMs);
        //RCF_UNUSED_VARIABLE(stopFlag);
        
        stopFlag == stopFlag;

        //RCF_TRACE("")(timeoutMs);

        //if (mThreadSpecificSessionQueuePtr.get() == NULL)
        //{
        //    mThreadSpecificSessionQueuePtr.reset(new SessionQueue);
        //}

        //while (!stopFlag && !mThreadSpecificSessionQueuePtr->empty())
        //{
        //    RcfSessionPtr sessionPtr = mThreadSpecificSessionQueuePtr->back();
        //    mThreadSpecificSessionQueuePtr->pop_back();
        //    handleSession(sessionPtr);
        //}
    }

    I_ServerTransport &RcfServer::getServerTransport()
    {
        return *getServerTransportPtr();
    }

    I_Service &RcfServer::getServerTransportService()
    {
        return dynamic_cast<I_Service &>(*getServerTransportPtr());
    }

    ServerTransportPtr RcfServer::getServerTransportPtr()
    {
        ReadLock readLock( mServicesMutex );
        RCF_ASSERT( ! mServerTransports.empty() );
        return mServerTransports[0];
    }

    bool RcfServer::bindShared(
        const std::string &name,
        const RcfClientPtr &rcfClientPtr)
    {
        RCF_ASSERT(rcfClientPtr.get());
        RCF_TRACE("")(name)(typeid(*rcfClientPtr));

        WriteLock writeLock(mStubMapMutex);
        mStubMap[name] = StubEntryPtr( new StubEntry(rcfClientPtr));
        return true;
    }

    FilterPtr RcfServer::createFilter(int filterId)
    {
        FilterFactoryPtr filterFactoryPtr =
            mFilterFactoryLookupProviders.empty() ?
                FilterFactoryPtr() :
                mFilterFactoryLookupProviders[0]->getFilterFactoryPtr(filterId);

        return filterFactoryPtr ?
            filterFactoryPtr->createFilter() :
            FilterPtr();
    }

    void RcfServer::setStartCallback(const StartCallback &startCallback)
    {
        mStartCallback = startCallback;
    }

    void RcfServer::invokeStartCallback()
    {
        if (mStartCallback)
        {
            mStartCallback(*this);
        }
    }

    bool RcfServer::getStopFlag() const
    {
        return mServerThreadsStopFlag;
    }

    int RcfServer::getRcfRuntimeVersion()
    {
        return mRcfRuntimeVersion;
    }

    void RcfServer::setRcfRuntimeVersion(int version)
    {
        mRcfRuntimeVersion = version;
    }

} // namespace RCF
