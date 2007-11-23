
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_SERVERINTERFACES_HPP
#define INCLUDE_RCF_SERVERINTERFACES_HPP

#include <RCF/Idl.hpp>
#include <RCF/SerializationProtocol.hpp>
#include <RCF/TypeTraits.hpp>

#ifdef RCF_USE_SF_SERIALIZATION
#include <SF/string.hpp>
#include <SF/vector.hpp>
#endif

#ifdef RCF_USE_BOOST_SERIALIZATION
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#endif

RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(std::vector<boost::int32_t>)

namespace RCF {
   
    // I_ObjectFactory
    RCF_BEGIN(I_ObjectFactory, "")
        RCF_METHOD_R2(boost::int32_t, createObject, const std::string &, Token &)
        RCF_METHOD_R1(boost::int32_t, createSessionObject, const std::string &)
        RCF_METHOD_R1(boost::int32_t, deleteObject, const Token &)
        RCF_METHOD_R0(boost::int32_t, deleteSessionObject)
    RCF_END(I_ObjectFactory)
   
    // I_EndpointBroker
    RCF_BEGIN(I_EndpointBroker, "")
        RCF_METHOD_R3(boost::int32_t, openEndpoint, const std::string &, const std::string &, std::string &)
        RCF_METHOD_R2(boost::int32_t, closeEndpoint, const std::string &, const std::string &)
        RCF_METHOD_R2(boost::int32_t, establishEndpointConnection, const std::string &, const std::string &)
        RCF_METHOD_R2(boost::int32_t, bindToEndpoint, const std::string &, const std::string &)
    RCF_END(I_EndpointBroker)
   
    // I_EndpointServer
    RCF_BEGIN(I_EndpointServer, "")
        RCF_METHOD_V1(void, spawnConnections, boost::uint32_t)
    RCF_END(I_EndpointServer)
   
    // I_RequestSubscription
    RCF_BEGIN( I_RequestSubscription, "" )
        RCF_METHOD_R1(boost::int32_t, requestSubscription, const std::string &)
    RCF_END(I_RequestSubscription)

    // I_RequestTransportFilters
    RCF_BEGIN(I_RequestTransportFilters, "")
        RCF_METHOD_R1(boost::int32_t, requestTransportFilters, const std::vector<boost::int32_t> &)
        RCF_METHOD_R1(boost::int32_t, queryForTransportFilters, const std::vector<boost::int32_t> &)
    RCF_END(I_RequestTransportFilters)

} // namespace RCF

#endif // ! INCLUDE_RCF_SERVERINTERFACES_HPP
