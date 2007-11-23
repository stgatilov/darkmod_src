

//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#include <boost/config.hpp>

#ifndef RCF_CPP_WHICH_SECTION
#define RCF_CPP_WHICH_SECTION 0
#endif

#if RCF_CPP_WHICH_SECTION == 0 || RCF_CPP_WHICH_SECTION == 1

#include "AsyncFilter.cpp"
#include "ByteBuffer.cpp"
#include "ByteOrdering.cpp"
#include "CheckRtti.cpp"
#include "ClientStub.cpp"
#include "ClientTransport.cpp"
#include "CurrentSerializationProtocol.cpp"
#include "CurrentSession.cpp"
#include "Endpoint.cpp"
#include "EndpointBrokerService.cpp"
#include "EndpointServerService.cpp"
#include "Exception.cpp"
#include "FilterService.cpp"

#endif // RCF_CPP_WHICH_SECTION == 1

#if RCF_CPP_WHICH_SECTION == 0 || RCF_CPP_WHICH_SECTION == 2

#include "InitDeinit.cpp"
#include "IpAddress.cpp"
#include "IpServerTransport.cpp"
#include "Marshal.cpp"
#include "MethodInvocation.cpp"
#include "MulticastClientTransport.cpp"
#include "ObjectFactoryService.cpp"
#include "PublishingService.cpp"
#include "Random.cpp"
#include "RcfClient.cpp"
#include "RcfServer.cpp"
#include "RcfSession.cpp"
#include "SerializationProtocol.cpp"
#include "ServerInterfaces.cpp"
#include "ServerStub.cpp"
#include "ServerTask.cpp"
#include "ServerTransport.cpp"
#include "Service.cpp"
#include "StubEntry.cpp"
#include "StubFactory.cpp"
#include "SubscriptionService.cpp"
#include "TcpClientTransport.cpp"
#include "TcpEndpoint.cpp"

#endif // RCF_CPP_WHICH_SECTION == 2

#if RCF_CPP_WHICH_SECTION == 0 || RCF_CPP_WHICH_SECTION == 3

#include "TcpServerTransport.cpp"
#include "ThreadLibrary.cpp"
#include "ThreadLocalData.cpp"
#include "ThreadManager.cpp"
#include "TimedBsdSockets.cpp"
#include "Token.cpp"
#include "Tools.cpp"
#include "UdpClientTransport.cpp"
#include "UdpEndpoint.cpp"
#include "UdpServerTransport.cpp"
#include "UsingBsdSockets.cpp"

#include "Protocol/Protocol.cpp"

#ifdef RCF_USE_BOOST_ASIO
#include "TcpAsioServerTransport.cpp"
#endif

#ifdef BOOST_WINDOWS
#include "TcpIocpServerTransport.cpp"
#include "SspiFilter.cpp"
#endif

#ifdef RCF_USE_OPENSSL
#include "OpenSslEncryptionFilter.cpp"
#include "UsingOpenSsl.cpp"
#endif

#ifdef RCF_USE_ZLIB
#include "ZlibCompressionFilter.cpp"
#endif

#ifdef RCF_USE_SF_SERIALIZATION
#include "../SF/SF.cpp"
#endif

#endif // RCF_CPP_WHICH_SECTION == 3
