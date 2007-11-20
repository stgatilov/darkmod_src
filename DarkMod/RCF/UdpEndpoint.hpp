
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_UDPENDPOINT_HPP
#define INCLUDE_RCF_UDPENDPOINT_HPP

#include <memory>
#include <string>

#include <boost/shared_ptr.hpp>

#include <RCF/Endpoint.hpp>
#include <RCF/InitDeinit.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/TypeTraits.hpp>

#ifdef RCF_USE_SF_SERIALIZATION
#include <SF/SfNew.hpp>
#endif

#include <SF/SerializeParent.hpp>

namespace RCF {

    class I_ServerTransport;
    class I_ClientTransport;
    class I_SessionManager;

    /// Represents a UDP communications endpoint.
    class UdpEndpoint : public I_Endpoint
    {
    public:

        /// Default constructor.
        UdpEndpoint();

        /// Constructor.
        /// \param port UDP port number of the endpoint.
        UdpEndpoint(int port);
       
        /// Constructor.
        /// \param ip IP number of the endpoint.
        /// \param port UDP port number of the endpoint.
        UdpEndpoint(const std::string &ip, int port);
       
        /// Copy constructor.
        /// \param rhs UdpEndpoint object to construct this object from.
        UdpEndpoint(const UdpEndpoint &rhs);
       
        /// Creates a server transport.
        /// \return Auto pointer to server transport.
        std::auto_ptr<I_ServerTransport> createServerTransport() const;
       
        /// Creates a client transport.
        /// \return Auto pointer to client transport.
        std::auto_ptr<I_ClientTransport> createClientTransport() const;
       
        /// Clones this endpoint.
        /// \return Shared pointer to a new UdpEndpoint object.
        EndpointPtr clone() const;
       
        /// Returns IP number of this endpoint.
        /// \return IP number of this endpoint.
        std::string getIp() const;
       
        /// Returns port number of this endpoint.
        /// \return port number of this endpoint.
        int getPort() const;
       
        /// Serializes the UdpEndpoint object.
        template<typename Archive>
        void serialize(Archive &ar, const unsigned int)
        {
            serializeParent( (I_Endpoint*) 0, ar, *this);
            ar & mIp & mPort;
        }

    private:
        std::string mIp;
        int mPort;
    };

} // namespace RCF

RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(RCF::UdpEndpoint)

#endif // ! INCLUDE_RCF_UDPENDPOINT_HPP
