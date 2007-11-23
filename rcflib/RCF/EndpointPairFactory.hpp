
//*****************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//*****************************************************************************

#ifndef _RCF_ENDPOINTPAIRFACTORY_HPP_
#define _RCF_ENDPOINTPAIRFACTORY_HPP_

#include <utility>

#include <boost/shared_ptr.hpp>

#include <RCF/Endpoint.hpp>
#include <RCF/util/PortNumbers.hpp>

namespace RCF {

    typedef std::pair< boost::shared_ptr<I_Endpoint>, boost::shared_ptr<I_Endpoint> > EndpointPair;

    class I_EndpointPairFactory
    {
    public:
        virtual ~I_EndpointPairFactory() {}
        virtual EndpointPair createEndpointPair() = 0;
    };

    typedef boost::shared_ptr<I_EndpointPairFactory> EndpointPairFactoryPtr;

    class TcpEndpointFactory : public I_EndpointPairFactory
    {
    public:
        EndpointPair createEndpointPair()
        {
            int port = util::Ports::getNext();
            return EndpointPair( 
                boost::shared_ptr<I_Endpoint>(new TcpEndpoint(port)), 
                boost::shared_ptr<I_Endpoint>(new TcpEndpoint("localhost", port)));
        }
    };

    class UdpEndpointFactory : public I_EndpointPairFactory
    {
    public:
        EndpointPair createEndpointPair()
        {
            int port = util::Ports::getNext();
            return EndpointPair(
                boost::shared_ptr<I_Endpoint>(new UdpEndpoint(port)), 
                boost::shared_ptr<I_Endpoint>(new UdpEndpoint("127.0.0.1", port)));
        }
    };

    extern std::vector< boost::shared_ptr<RCF::I_EndpointPairFactory> > endpointPairFactories;
    

} // namespace RCF

#endif // ! _RCF_ENDPOINTPAIRFACTORY_HPP_