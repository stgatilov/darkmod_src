
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/SubscriptionService.hpp>

#include <boost/bind.hpp>

#include <typeinfo>

#include <RCF/ClientProgress.hpp>
#include <RCF/ClientTransport.hpp>
#include <RCF/RcfServer.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/ServerInterfaces.hpp>

namespace RCF {

    bool Subscription::isConnected()
    {
        return
            mClientTransportAutoPtr.get() &&
            mClientTransportAutoPtr->isConnected();
    }

    SubscriptionService::SubscriptionService() :
        mSubscriptionsMutex(WriterPriority),
        mRcfRuntimeVersion(RCF_DEFAULT_INIT)
    {}

    void vc6_boost_bind_helper(SubscriptionService::OnDisconnect onDisconnect, RcfSession &)
    {
        onDisconnect();
    }

    SubscriptionPtr SubscriptionService::beginSubscribeNamed(
        SubscriptionId whichSubscription,
        RcfClientPtr rcfClientPtr,
        ClientTransportAutoPtr clientTransportAutoPtr,
        OnDisconnect onDisconnect,
        const std::string &publisherName,
        ClientProgressPtr clientProgressPtr)
    {
        I_ServerTransportEx &serverTransportEx =
            dynamic_cast<I_ServerTransportEx &>(*mServerTransportPtr);

        RcfClient<I_RequestSubscription> client( clientTransportAutoPtr);
        client.getClientStub().setRcfRuntimeVersion( mRcfRuntimeVersion);

        if (clientProgressPtr)
        {
            client.getClientStub().setClientProgressPtr(clientProgressPtr);
        }
       
        bool ok = (RcfError_Ok == client.requestSubscription(Twoway, publisherName));
        if (ok)
        {
            SessionPtr sessionPtr =
                serverTransportEx.createServerSession(
                    client.getClientStub().releaseTransport());

            if (sessionPtr.get())
            {
                RcfSessionPtr rcfSessionPtr(
                    boost::static_pointer_cast<RcfSession>(sessionPtr));

                rcfSessionPtr->setDefaultStubEntryPtr( StubEntryPtr(
                    new StubEntry(rcfClientPtr)));
               
                if (onDisconnect)
                {
#if defined(_MSC_VER) && _MSC_VER < 1310

                    rcfSessionPtr->setOnDestroyCallback(
                        boost::bind(vc6_boost_bind_helper, onDisconnect, _1));

#else

                    rcfSessionPtr->setOnDestroyCallback(
                        boost::bind(onDisconnect));

#endif
                }

                ClientTransportAutoPtr apClientTransport(
                    serverTransportEx.createClientTransport(sessionPtr) );

                WriteLock lock(mSubscriptionsMutex);

                SubscriptionPtr subscriptionPtr(
                    new Subscription(apClientTransport, rcfSessionPtr));

                mSubscriptions[ whichSubscription ] = subscriptionPtr;

                return subscriptionPtr;
            }
        }
        return SubscriptionPtr();
    }

    SubscriptionPtr SubscriptionService::beginSubscribeNamed(
        SubscriptionId whichSubscription,
        RcfClientPtr rcfClientPtr,
        const I_Endpoint &publisherEndpoint,
        OnDisconnect onDisconnect,
        const std::string &publisherName,
        ClientProgressPtr clientProgressPtr)
    {
        I_ServerTransportEx &serverTransportEx =
            dynamic_cast<I_ServerTransportEx &>(*mServerTransportPtr);

        ClientTransportAutoPtr clientTransportAutoPtr(
            serverTransportEx.createClientTransport(publisherEndpoint) );

        return
            beginSubscribeNamed(
                whichSubscription,
                rcfClientPtr,
                clientTransportAutoPtr,
                onDisconnect,
                publisherName,
                clientProgressPtr);
    }

    bool SubscriptionService::endSubscribeNamed(SubscriptionId whichSubscription)
    {
        SubscriptionPtr subscriptionPtr;
        {
            WriteLock lock(mSubscriptionsMutex);
            subscriptionPtr = mSubscriptions[ whichSubscription ];
            mSubscriptions[ whichSubscription ].reset();
        }
        if (subscriptionPtr)
        {
            RcfSessionPtr rcfSessionPtr =
                subscriptionPtr->mRcfSessionWeakPtr.lock();

            if (rcfSessionPtr)
            {
                // block server threads from executing remote calls
                Lock lock(rcfSessionPtr->mStopCallInProgressMutex);
                rcfSessionPtr->mStopCallInProgress = true;

                // remove subscription binding
                rcfSessionPtr->setDefaultStubEntryPtr(StubEntryPtr());

                // clear the destroy callback
                // TODO: how do we know that we're not clearing someone else's callback?
                rcfSessionPtr->setOnDestroyCallback(RcfSession::OnDestroyCallback());
            }
        }
        return true;
    }

    void SubscriptionService::onServiceAdded(RcfServer &server)
    {
        mServerTransportPtr = server.getServerTransportPtr();
        mRcfRuntimeVersion = server.getRcfRuntimeVersion();
    }

    void SubscriptionService::onServiceRemoved(RcfServer &)
    {}

    void SubscriptionService::onServerClose(RcfServer &server)
    {
        RCF_UNUSED_VARIABLE(server);
        WriteLock writeLock(mSubscriptionsMutex);
        mSubscriptions.clear();
    }

    void SubscriptionService::setRcfRuntimeVersion(int rcfRuntimeVersion)
    {
        mRcfRuntimeVersion = rcfRuntimeVersion;
    }

    int SubscriptionService::getRcfRuntimeVersion()
    {
        return mRcfRuntimeVersion;
    }

    Subscription::Subscription(
        ClientTransportAutoPtr clientTransportAutoPtr,
        RcfSessionWeakPtr rcfSessionWeakPtr) :
            mClientTransportAutoPtr(clientTransportAutoPtr),
            mRcfSessionWeakPtr(rcfSessionWeakPtr)
    {}
   
} // namespace RCF
