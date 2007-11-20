
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/PublishingService.hpp>

#include <RCF/CurrentSession.hpp>
#include <RCF/MulticastClientTransport.hpp>
#include <RCF/RcfServer.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/ServerInterfaces.hpp>
#include <RCF/ServerTransport.hpp>

namespace RCF {

    PublishingService::PublishingService() :
        mPublishersMutex(WriterPriority)
    {}

    bool PublishingService::beginPublishNamed(
        const std::string &publisherName,
        RcfClientPtr rcfClientPtr)
    {
        rcfClientPtr->getClientStub().setTransport(ClientTransportAutoPtr(new MulticastClientTransport));
        rcfClientPtr->getClientStub().setDefaultCallingSemantics(Oneway);
        rcfClientPtr->getClientStub().setTargetName("");
        rcfClientPtr->getClientStub().setTargetToken( Token());
        PublisherPtr publisherPtr( new Publisher(publisherName, rcfClientPtr));
        WriteLock lock(mPublishersMutex);
        mPublishers[publisherName] = publisherPtr;
        return true;
    }

    I_RcfClient &PublishingService::publishNamed(const std::string &publisherName)
    {
        ReadLock lock(mPublishersMutex);
        if (mPublishers.find(publisherName) != mPublishers.end())
        {
            return *mPublishers[ publisherName ]->mMulticastRcfClientPtr;
        }
        RCF_THROW(Exception(RcfError_UnknownPublisher))(publisherName);
    }

    bool PublishingService::endPublishNamed(const std::string &publisherName)
    {
        WriteLock lock(mPublishersMutex);
        Publishers::iterator iter = mPublishers.find(publisherName);
        if (iter != mPublishers.end())
        {
            mPublishers.erase(iter);
        }
        return true;
    }

    // remotely accessible
    boost::int32_t PublishingService::requestSubscription(
        const std::string &subscriptionName)
    {
        std::string publisherName = subscriptionName;
        bool found = false;
        ReadLock lock(mPublishersMutex);
        if (mPublishers.find(publisherName) != mPublishers.end())
        {
            found = true;
        }
        lock.unlock();
        if (found)
        {
            I_ServerTransportEx &serverTransport =
                dynamic_cast<I_ServerTransportEx &>(
                    getCurrentRcfSessionPtr()->getProactorPtr()->getServerTransport());

            ClientTransportAutoPtr clientTransportAutoPtr(
                serverTransport.createClientTransport(getCurrentSessionPtr()));

            ClientTransportPtr clientTransportPtr(
                clientTransportAutoPtr.release());

            getCurrentRcfSessionPtr()->addOnWriteCompletedCallback(
                boost::bind(
                    &PublishingService::addSubscriberTransport,
                    this,
                    _1,
                    publisherName,
                    clientTransportPtr) );
        }
        return found ? RcfError_Ok : RcfError_Unspecified;
    }

    void PublishingService::onServiceAdded(RcfServer &server)
    {
        server.bind( (I_RequestSubscription *) NULL,*this);
    }

    void PublishingService::onServiceRemoved(RcfServer &)
    {}

    void PublishingService::onServerClose(RcfServer &server)
    {
        // need to do this now, rather than implicitly, when RcfServer is destroyed, because
        // the client transport objects have links to the server transport (close functor)
        RCF_UNUSED_VARIABLE(server);
        WriteLock writeLock(mPublishersMutex);
        this->mPublishers.clear();
    }

    void PublishingService::addSubscriberTransport(
        RcfSession &session,
        const std::string &publisherName,
        ClientTransportPtr clientTransportPtr)
    {
        RCF_UNUSED_VARIABLE(session);
        WriteLock lock(mPublishersMutex);
        if (mPublishers.find(publisherName) != mPublishers.end())
        {
            I_ClientTransport &clientTransport =
                mPublishers[ publisherName ]->mMulticastRcfClientPtr->
                    getClientStub().getTransport();

            MulticastClientTransport &multiCastClientTransport =
                dynamic_cast<MulticastClientTransport &>(clientTransport);

            multiCastClientTransport.addTransport(clientTransportPtr);
        }
    }
   
} // namespace RCF
