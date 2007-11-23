
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/UdpEndpoint.hpp>

#include <RCF/InitDeinit.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/UdpClientTransport.hpp>
#include <RCF/UdpServerTransport.hpp>

#ifdef RCF_USE_SF_SERIALIZATION
#include <SF/Registry.hpp>
#endif

namespace RCF {

    UdpEndpoint::UdpEndpoint() :
        mIp(),
        mPort(RCF_DEFAULT_INIT)
    {}

    UdpEndpoint::UdpEndpoint(int port) :
        mPort(port)
    {}

    UdpEndpoint::UdpEndpoint(const std::string &ip, int port) :
        mIp(ip),
        mPort(port)
    {}

    UdpEndpoint::UdpEndpoint(const UdpEndpoint &rhs) :
    	I_Endpoint(rhs),
        mIp(rhs.mIp),
        mPort(rhs.mPort)
    {}

    EndpointPtr UdpEndpoint::clone() const
    {
        return EndpointPtr(new UdpEndpoint(*this));
    }

    std::string UdpEndpoint::getIp() const
    {
        return mIp;
    }

    int UdpEndpoint::getPort() const
    {
        return mPort;
    }

    std::auto_ptr<I_ServerTransport> UdpEndpoint::createServerTransport() const
    {
        return std::auto_ptr<I_ServerTransport>(
            new UdpServerTransport(mPort));
    }

    std::auto_ptr<I_ClientTransport> UdpEndpoint::createClientTransport() const
    {
        return std::auto_ptr<I_ClientTransport>(
            new UdpClientTransport(mIp, mPort));
    }

    inline void initUdpEndpointSerialization()
    {
#ifdef RCF_USE_SF_SERIALIZATION
        SF::registerType( (UdpEndpoint *) 0, "RCF::UdpEndpoint");
        SF::registerBaseAndDerived( (I_Endpoint *) 0, (UdpEndpoint *) 0);
#endif
    }

    RCF_ON_INIT_NAMED( initUdpEndpointSerialization(), InitUdpEndpointSerialization );

} // namespace RCF

#ifdef RCF_USE_BOOST_SERIALIZATION
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>
BOOST_CLASS_EXPORT_GUID(RCF::UdpEndpoint, "RCF::UdpEndpoint")
BOOST_SERIALIZATION_SHARED_PTR(RCF::UdpEndpoint)
#endif
