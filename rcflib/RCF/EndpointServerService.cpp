
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/EndpointServerService.hpp>

#include <RCF/CurrentSession.hpp>
#include <RCF/RcfServer.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/ServerInterfaces.hpp>
#include <RCF/StubFactory.hpp>
#include <RCF/Tools.hpp>

namespace RCF {

    // remotely invoked (on the master connection)
    bool EndpointServer::spawnConnections(unsigned int requestedConnections)
    {
        if (requestedConnections > 10)
        {
            requestedConnections = 10;
        }

        I_ServerTransport &serverTransport =
            getCurrentRcfSessionPtr()->getProactorPtr()->getServerTransport();

        I_ServerTransportEx *pServerTransportEx =
            dynamic_cast<I_ServerTransportEx *>(&serverTransport);

        if (NULL == pServerTransportEx)
        {
            return false;
        }
        I_ServerTransportEx &serverTransportEx = *pServerTransportEx;
        for (unsigned int i=0; i<requestedConnections && mClients.size() < 50; ++i)
        {
            ClientTransportAutoPtr clientTransportAutoPtr(
                mClients.front()->getClientStub().getTransport().clone());
            Client client(clientTransportAutoPtr);
            // TODO: handle exceptions
            client.establishEndpointConnection(
                Oneway,
                mEndpointName,
                mEndpointServerPassword);

            SessionPtr sessionPtr(
                serverTransportEx.createServerSession(
                    client.getClientStub().releaseTransport()));

            ClientTransportAutoPtr apClientTransport(
                serverTransportEx.createClientTransport(sessionPtr));

            ClientPtr clientPtr( new Client(apClientTransport) );
            mClients.push_back(clientPtr);
        }
        return true;
    }

    EndpointServerService::EndpointServerService() :
        mEndpointServersMutex(WriterPriority)
    {}

    void EndpointServerService::onServiceAdded(RcfServer &server)
    {
        mServerTransportPtr = server.getServerTransportPtr();
    }

    void EndpointServerService::onServiceRemoved(RcfServer &)
    {
    }

    void EndpointServerService::onServerClose(RcfServer &)
    {
        WriteLock writeLock(mEndpointServersMutex);
        mEndpointServers.clear();
    }

    EndpointServerService::EndpointId EndpointServerService::openEndpoint(
        const I_Endpoint &brokerEndpoint,
        const std::string &endpointName)
    {
        I_ServerTransportEx &serverTransportEx =
            dynamic_cast<I_ServerTransportEx &>(*mServerTransportPtr);

        ClientTransportAutoPtr clientTransportAutoPtr(
            serverTransportEx.createClientTransport(brokerEndpoint));

        return openEndpoint(clientTransportAutoPtr, endpointName);
    }

    EndpointServerService::EndpointId EndpointServerService::openEndpoint(
        ClientTransportAutoPtr clientTransportAutoPtr,
        const std::string &endpointName)
    {
        EndpointId endpointId = 0;
        std::string endpointClientPassword = ""; // TODO
        std::string endpointServerPassword = ""; // TODO
        I_ServerTransportEx &serverTransportEx =
            dynamic_cast<I_ServerTransportEx &>(*mServerTransportPtr);

        EndpointServerPtr endpointServerPtr;
        {
            WriteLock lock(mEndpointServersMutex);

            // TODO: user configuration of the limit
            while (
                ++endpointId < 10 &&
                mEndpointServers.find(endpointId) != mEndpointServers.end());

            if (endpointId < 10)
            {
                endpointServerPtr.reset( new EndpointServer() );
                endpointServerPtr->mEndpointName = endpointName;
                endpointServerPtr->mEndpointId = endpointId;
                mEndpointServers[endpointServerPtr->getEndpointId()] = endpointServerPtr;
            }
        }
        if (endpointServerPtr)
        {
            RcfClient<I_EndpointBroker> client(clientTransportAutoPtr);
            bool ok = (RcfError_Ok == client.openEndpoint(
                Twoway,
                endpointName,
                endpointClientPassword,
                endpointServerPassword));

            if (ok)
            {
                SessionPtr sessionPtr0(
                    serverTransportEx.createServerSession(
                        client.getClientStub().releaseTransport()));

                RcfSessionPtr sessionPtr(
                    boost::static_pointer_cast<RcfSession>(sessionPtr0));

                clientTransportAutoPtr.reset(
                    serverTransportEx.createClientTransport(sessionPtr0).release());

                endpointServerPtr->mEndpointClientPassword = endpointClientPassword;
                endpointServerPtr->mEndpointServerPassword = endpointServerPassword;
                endpointServerPtr->mClients.push_back(
                    EndpointServer::ClientPtr(
                        new EndpointServer::Client(clientTransportAutoPtr) ) );

                // break the cycle session->endpointserverstub->clientConnection->session
                EndpointServerWeakPtr endpointServerWeakPtr(endpointServerPtr);
                boost::shared_ptr< I_Deref<EndpointServer> > derefPtr(
                    new DerefWeakPtr<EndpointServer>(endpointServerWeakPtr) );

                RcfClientPtr rcfClientPtr(
                    createServerStub(
                        (I_EndpointServer *) 0,
                        (EndpointServer *) 0,
                        derefPtr) );

                sessionPtr->setDefaultStubEntryPtr(
                    StubEntryPtr( new StubEntry(rcfClientPtr)));

                return endpointServerPtr->getEndpointId();
            }
            else
            {
                WriteLock writeLock(mEndpointServersMutex);
                mEndpointServers.erase( mEndpointServers.find(
                    endpointServerPtr->getEndpointId()) );
            }
        }
        return EndpointId();
    }

    void EndpointServerService::closeEndpoint(EndpointId endpointId)
    {
        WriteLock writeLock(mEndpointServersMutex);
        mEndpointServers.erase( mEndpointServers.find(endpointId) );
    }

} // namespace RCF
