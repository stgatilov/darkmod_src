
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <RCF/TcpEndpoint.hpp>

#include <boost/config.hpp>

#include <RCF/InitDeinit.hpp>
#include <RCF/SerializationProtocol.hpp>

#ifdef RCF_USE_BOOST_ASIO

#include <RCF/TcpAsioServerTransport.hpp>

#elif defined(BOOST_WINDOWS)

#include <RCF/TcpIocpServerTransport.hpp>
#include <RCF/TcpClientTransport.hpp>

#else

#include <RCF/TcpServerTransport.hpp>
#include <RCF/TcpClientTransport.hpp>

#endif

#ifdef RCF_USE_SF_SERIALIZATION
#include <SF/Registry.hpp>
#endif

namespace RCF {

    TcpEndpoint::TcpEndpoint() :
        mIp(),
        mPort(RCF_DEFAULT_INIT)
    {}

    TcpEndpoint::TcpEndpoint(int port) :
        mIp("127.0.0.1"),
        mPort(port)
    {}

    TcpEndpoint::TcpEndpoint(const std::string &ip, int port) :
        mIp(ip),
        mPort(port)
    {}

    TcpEndpoint::TcpEndpoint(const TcpEndpoint &rhs) :
    	I_Endpoint(rhs),
        mIp(rhs.mIp),
        mPort(rhs.mPort)
    {}

    EndpointPtr TcpEndpoint::clone() const
    {
        return EndpointPtr(new TcpEndpoint(*this));
    }

    std::string TcpEndpoint::getIp() const
    {
        return mIp;
    }

    int TcpEndpoint::getPort() const
    {
        return mPort;
    }

#ifdef RCF_USE_BOOST_ASIO

    std::auto_ptr<I_ServerTransport> TcpEndpoint::createServerTransport() const
    {
        return std::auto_ptr<I_ServerTransport>(
            new RCF::TcpAsioServerTransport(mPort));
    }

    std::auto_ptr<I_ClientTransport> TcpEndpoint::createClientTransport() const
    {
        return std::auto_ptr<I_ClientTransport>(
            new RCF::TcpClientTransport(mIp, mPort));
    }

#elif defined(BOOST_WINDOWS)

    std::auto_ptr<I_ServerTransport> TcpEndpoint::createServerTransport() const
    {
        return std::auto_ptr<I_ServerTransport>(
            new RCF::TcpIocpServerTransport(mIp, mPort));
    }

    std::auto_ptr<I_ClientTransport> TcpEndpoint::createClientTransport() const
    {
        return std::auto_ptr<I_ClientTransport>(
            new RCF::TcpClientTransport(mIp, mPort));
    }

#else

    std::auto_ptr<I_ServerTransport> TcpEndpoint::createServerTransport() const
    {
        return std::auto_ptr<I_ServerTransport>(
            new RCF::TcpServerTransport(mPort));
    }

    std::auto_ptr<I_ClientTransport> TcpEndpoint::createClientTransport() const
    {
        return std::auto_ptr<I_ClientTransport>(
            new RCF::TcpClientTransport(mIp, mPort));
    }

#endif

    inline void initTcpEndpointSerialization()
    {
#ifdef RCF_USE_SF_SERIALIZATION
        SF::registerType( (TcpEndpoint *) 0, "RCF::TcpEndpoint");
        SF::registerBaseAndDerived( (I_Endpoint *) 0, (TcpEndpoint *) 0);
#endif
    }

    RCF_ON_INIT_NAMED( initTcpEndpointSerialization(), InitTcpEndpointSerialization );

} // namespace RCF

#ifdef RCF_USE_BOOST_SERIALIZATION
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>
BOOST_CLASS_EXPORT_GUID(RCF::TcpEndpoint, "RCF::TcpEndpoint")
BOOST_SERIALIZATION_SHARED_PTR(RCF::TcpEndpoint)
#endif
