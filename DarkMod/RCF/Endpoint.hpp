
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_ENDPOINT_HPP
#define INCLUDE_RCF_ENDPOINT_HPP

#include <memory>
#include <string>

#include <boost/shared_ptr.hpp>

#include <RCF/SerializationProtocol.hpp>

#ifdef RCF_USE_SF_SERIALIZATION
#include <SF/SfNew.hpp>
#endif

namespace RCF {

    class I_ServerTransport;
    class I_ClientTransport;
    class I_SessionManager;

    class I_Endpoint;
    typedef boost::shared_ptr<I_Endpoint> EndpointPtr;

    /// Represents an abstraction of a communications endpoint.
    /// In essence, I_Endpoint is a factory for creating compatible pairs of server and client transports.
    class I_Endpoint
    {
    public:
        /// Virtual destructor.
        virtual ~I_Endpoint()
        {}

        /// Creates a server transport corresponding to the endpoint.
        /// \return Auto pointer to a server transport.
        virtual std::auto_ptr<I_ServerTransport> createServerTransport() const = 0;

        /// Creates a client transport corresponding to the endpoint.
        /// \return Auto pointer to a client transport.
        virtual std::auto_ptr<I_ClientTransport> createClientTransport() const = 0;

        /// Clones the endpoint
        /// \return Shared pointer to a clone (deep copy) of this endpoint.
        virtual EndpointPtr clone() const = 0;

        template<typename Archive>
        void serialize(Archive &, const unsigned int)
        {}
    };

    // serialization

    //template<typename Archive>
    //inline void serialize(Archive &, I_Endpoint &, const unsigned int)
    //{}

} // namespace RCF

/*namespace SF {
    template<typename Archive>
    inline void serialize(Archive &, RCF::I_Endpoint &, const unsigned int)
    {}
}*/

RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(RCF::I_Endpoint)

#ifdef RCF_USE_SF_SERIALIZATION
namespace SF {
    SF_NO_CTOR(RCF::I_Endpoint)
}
#endif

#ifdef RCF_USE_BOOST_SERIALIZATION
#include <boost/serialization/is_abstract.hpp>
#include <boost/serialization/shared_ptr.hpp>
BOOST_IS_ABSTRACT(RCF::I_Endpoint)
BOOST_SERIALIZATION_SHARED_PTR(RCF::I_Endpoint)
#endif

#endif // ! INCLUDE_RCF_ENDPOINT_HPP
