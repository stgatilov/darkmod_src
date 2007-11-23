//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TEST_ENDPOINTFACTORIES_HPP
#define INCLUDE_RCF_TEST_ENDPOINTFACTORIES_HPP

#include <utility>

#include <boost/shared_ptr.hpp>

#include <RCF/TcpEndpoint.hpp>
#include <RCF/UdpEndpoint.hpp>
#include <RCF/util/AutoRun.hpp>
#include <RCF/util/PortNumbers.hpp>

namespace RCF {

    typedef std::pair<EndpointPtr, EndpointPtr> EndpointPair;

    class I_EndpointPairFactory
    {
    public:
        virtual ~I_EndpointPairFactory() {}
        virtual EndpointPair createEndpointPair() = 0;
        virtual EndpointPair createNonListeningEndpointPair() = 0;
    };

    typedef boost::shared_ptr<I_EndpointPairFactory> EndpointPairFactoryPtr;

    class TcpEndpointFactory : public I_EndpointPairFactory
    {
    public:
        EndpointPair createEndpointPair()
        {
            std::string ip = util::PortNumbers::getSingleton().getIp();
            int port = util::PortNumbers::getSingleton().getNext();
            return EndpointPair(
                EndpointPtr(new TcpEndpoint(port)),
                EndpointPtr(new TcpEndpoint(ip, port)));
        }
        EndpointPair createNonListeningEndpointPair()
        {
            return EndpointPair(
                EndpointPtr(new TcpEndpoint(-1)),
                EndpointPtr(new TcpEndpoint()));
        }
    };

    class UdpEndpointFactory : public I_EndpointPairFactory
    {
    public:
        EndpointPair createEndpointPair()
        {
            std::string ip = util::PortNumbers::getSingleton().getIp();
            int port = util::PortNumbers::getSingleton().getNext();
            return EndpointPair(
                EndpointPtr(new UdpEndpoint(port)),
                EndpointPtr(new UdpEndpoint(ip, port)));
        }
        EndpointPair createNonListeningEndpointPair()
        {
            // non listening endpoint makes no sense for UDP
            return createEndpointPair();
        }
    };

    inline void writeEndpointTypes(std::ostream &os, const I_Endpoint &serverEndpoint, const I_Endpoint &clientEndpoint)
    {
        os
            << "Server endpoint: " << typeid(serverEndpoint).name() << ", "
            << "Client endpoint: " << typeid(clientEndpoint).name()
            << std::endl;
    }

    typedef std::vector< boost::shared_ptr<RCF::I_EndpointPairFactory> > EndpointPairFactories;

    static EndpointPairFactories &getEndpointPairFactories()
    {
        static EndpointPairFactories endpointPairFactories;
        return endpointPairFactories;
    }

    AUTO_RUN( getEndpointPairFactories().push_back( RCF::EndpointPairFactoryPtr(new RCF::TcpEndpointFactory)));

#ifndef RCF_TEST_NO_UDP
    AUTO_RUN( getEndpointPairFactories().push_back( RCF::EndpointPairFactoryPtr(new RCF::UdpEndpointFactory)));
#endif

} // namespace RCF

#endif // ! INCLUDE_RCF_TEST_ENDPOINTFACTORIES_HPP
