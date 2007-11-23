
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_ENDPOINTSERVERSERVICE_HPP
#define INCLUDE_RCF_ENDPOINTSERVERSERVICE_HPP

#include <list>
#include <map>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/ServerInterfaces.hpp>
#include <RCF/Service.hpp>

namespace RCF {

    class RcfServer;
    class I_Endpoint;
    class I_ServerTransport;
    class I_ClientTransport;

    //template<typename T>
    //class RcfClient;
    //class I_EndpointBroker;
    //template<> class RcfClient<I_EndpointBroker>;

    typedef std::auto_ptr<I_ClientTransport> ClientTransportAutoPtr;

    class EndpointServer
    {
    public:

        typedef int EndpointId;

        EndpointServer() :
            mEndpointId(RCF_DEFAULT_INIT)
        {}

        EndpointId getEndpointId()
        {
            return mEndpointId;
        }

        // remotely invoked (on the master connection)
        bool spawnConnections(unsigned int requestedConnections);

    private:

        friend class EndpointServerService;

        std::string mEndpointName;
        std::string mEndpointClientPassword;
        std::string mEndpointServerPassword;
        EndpointId mEndpointId;

        typedef RcfClient<I_EndpointBroker> Client;
        typedef boost::shared_ptr< Client > ClientPtr;
        std::list<ClientPtr> mClients;
    };

    typedef boost::shared_ptr<EndpointServer> EndpointServerPtr;
    typedef boost::weak_ptr<EndpointServer> EndpointServerWeakPtr;

    class EndpointServerService :
        public I_Service,
        boost::noncopyable
    {
    public:
        typedef EndpointServer::EndpointId EndpointId;
        EndpointServerService();
        void onServiceAdded(RcfServer &server);
        void onServiceRemoved(RcfServer &server);
        void onServerClose(RcfServer &server);
       
        EndpointId openEndpoint(
            const I_Endpoint &brokerEndpoint,
            const std::string &endpointName);
       
        EndpointId openEndpoint(
            ClientTransportAutoPtr clientTransportAutoPtr,
            const std::string &endpointName);
       
        void closeEndpoint(EndpointId endpointId);

    private:
        EndpointId getNextEndpointId();
        ServerTransportPtr mServerTransportPtr;
        ReadWriteMutex mEndpointServersMutex;
        std::map<EndpointId, EndpointServerPtr> mEndpointServers;
    };

    typedef boost::shared_ptr<EndpointServerService> EndpointServerServicePtr;

} // namespace RCF

#endif // ! INCLUDE_RCF_ENDPOINTSERVERSERVICE_HPP
