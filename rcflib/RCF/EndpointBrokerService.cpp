
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/EndpointBrokerService.hpp>

#include <RCF/CurrentSession.hpp>
#include <RCF/RcfServer.hpp>

namespace RCF {

    EndpointBroker::EndpointBroker(
        ServerTransportPtr serverTransportPtr,
        const std::string &endpointName,
        const std::string &endpointClientPassword,
        const std::string &endpointServerPassword) :
            mEndpointName(endpointName),
            mEndpointServerPassword(endpointServerPassword),
            mEndpointClientPassword(endpointClientPassword),
            mServerTransportPtr(serverTransportPtr)
    {}

    boost::int32_t EndpointBroker::bindToEndpoint()
    {
        RCF_ASSERT( mMasterConnection.get() != NULL );
        if (mConnections.size() == 0)
        {
            if (mMasterConnection->getClientStub().isConnected())
            {
                try
                {
                    mMasterConnection->spawnConnections(Oneway, 3);
                    return RcfError_EndpointRetry;
                }
                catch(const RCF::Exception &e)
                {
                    RCF_TRACE("")(e);
                }
            }
            return RcfError_EndpointDown;
        }
        else
        {
            RCF_ASSERT(mConnections.size() > 0);
            while (!mConnections.empty())
            {
                RcfSessionPtr sessionPtr = mConnections.back();
                SessionPtr sessionPtr0 =
                    boost::static_pointer_cast<I_Session>(sessionPtr);
                mConnections.pop_back();
                RCF_ASSERT(sessionPtr.get());
                I_ServerTransport &serverTransport =
                    getCurrentRcfSessionPtr()->getProactorPtr()->getServerTransport();
                I_ServerTransportEx &serverTransportEx =
                    dynamic_cast<I_ServerTransportEx &>(serverTransport);
                if (serverTransportEx.reflect(getCurrentSessionPtr(), sessionPtr0))
                {
                    return RcfError_Ok;
                }
            }
            RCF_ASSERT( mConnections.empty());
            return bindToEndpoint(); // try again, this time try the master connection
        }
    }


    EndpointBrokerService::EndpointBrokerService() :
        mEndpointBrokersMutex(WriterPriority)
    {}

    void EndpointBrokerService::onServiceAdded(RcfServer &server)
    {
        // need to obtain a shared ptr to the transport, so it doesn't get deleted before we do
        mServerTransportPtr = server.getServerTransportPtr();
        server.bind( (I_EndpointBroker *) NULL, *this);
    }

    void EndpointBrokerService::onServiceRemoved(RcfServer &)
    {}

    void EndpointBrokerService::onServerClose(RcfServer &server)
    {
        RCF_UNUSED_VARIABLE(server);
        WriteLock writeLock(mEndpointBrokersMutex);
        mEndpointBrokers.clear();
    }

    // remotely invoked
    boost::int32_t EndpointBrokerService::openEndpoint(
        const std::string &endpointName,
        const std::string &endpointClientPassword,
        std::string &endpointServerPassword)
    {
        WriteLock writeLock(mEndpointBrokersMutex);

        if (mEndpointBrokers[endpointName].get() &&
            !mEndpointBrokers[endpointName]->mMasterConnection->getClientStub().isConnected())
        {
            mEndpointBrokers[endpointName].reset();
        }

        if (mEndpointBrokers[endpointName].get() &&
                mEndpointBrokers[endpointName]->mEndpointServerPassword ==
                endpointServerPassword)
        {
            mEndpointBrokers[endpointName].reset();
        }

        // TODO: check for zombie broker (e.g. all connections defunct)
        if (mEndpointBrokers[endpointName].get() == NULL)
        {
            I_ServerTransport &serverTransport =
                getCurrentRcfSessionPtr()->getProactorPtr()->getServerTransport();
            I_ServerTransportEx *pServerTransportEx =
                dynamic_cast<I_ServerTransportEx *>(&serverTransport);
            if (pServerTransportEx)
            {
                I_ServerTransportEx &serverTransportEx = *pServerTransportEx;
                mEndpointBrokers[endpointName].reset(
                    new EndpointBroker(
                        mServerTransportPtr,
                        endpointName,
                        endpointClientPassword,
                        endpointServerPassword));
                ClientTransportAutoPtr clientTransportAutoPtr(
                    serverTransportEx.createClientTransport(getCurrentSessionPtr()));
                boost::shared_ptr< RcfClient<I_EndpointServer> > clientPtr(
                    new RcfClient<I_EndpointServer>(clientTransportAutoPtr) );
                clientPtr->getClientStub().setRcfRuntimeVersion( 
                    getCurrentRcfSessionPtr()->getRcfRuntimeVersion());
                clientPtr->getClientStub().setTargetName("");
                clientPtr->getClientStub().setTargetToken(Token());
                mEndpointBrokers[endpointName]->mMasterConnection = clientPtr;
                //return true;
                return RcfError_Ok;
            }
        }

        // TODO: improve
        return RcfError_Unspecified;
    }

    // remotely invoked
    boost::int32_t EndpointBrokerService::closeEndpoint(
        const std::string &endpointName,
        const std::string &endpointServerPassword)
    {
        RCF_UNUSED_VARIABLE(endpointServerPassword);
        WriteLock writeLock(mEndpointBrokersMutex);

        if (mEndpointBrokers[endpointName].get())
        {
            // TODO: physically remove the entry, not just reset it
            mEndpointBrokers[endpointName].reset();
        }
        //return true;
        return RcfError_Ok;
    }

    // remotely invoked
    boost::int32_t EndpointBrokerService::establishEndpointConnection(
        const std::string &endpointName,
        const std::string &endpointServerPassword)
    {
        // TODO: read lock here and an extra mutex for each endpoint broker instead
        WriteLock writeLock(mEndpointBrokersMutex);

        if (mEndpointBrokers[endpointName].get() &&
            mEndpointBrokers[endpointName]->mEndpointServerPassword == endpointServerPassword)
        {
            mEndpointBrokers[endpointName]->mConnections.push_back(
                getCurrentRcfSessionPtr());
            //return true;
            return RcfError_Ok;
        }
        //return false;
        return RcfError_Unspecified;
    }

    // remotely invoked
    boost::int32_t EndpointBrokerService::bindToEndpoint(
        const std::string &endpointName,
        const std::string &endpointClientPassword)
    {
        // TODO: read lock here and an extra mutex for each endpoint broker instead
        WriteLock writeLock(mEndpointBrokersMutex);

        // TODO: don't use mEndpointBrokers[] until it is known that the endpoint broker exists

        if (NULL == mEndpointBrokers[endpointName].get())
        {
            return RcfError_UnknownEndpoint;
        }
        else if (mEndpointBrokers[endpointName]->mEndpointClientPassword != endpointClientPassword)
        {
            return RcfError_EndpointPassword;
        }
        else
        {
            // bind current session to endpoint
            int err = mEndpointBrokers[endpointName]->bindToEndpoint();
            if (err == RcfError_EndpointDown)
            {
                mEndpointBrokers[endpointName].reset();
            }
            return err;
        }
    }

} // namespace RCF
