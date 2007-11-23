
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_MULTICASTCLIENTTRANSPORT_HPP
#define INCLUDE_RCF_MULTICASTCLIENTTRANSPORT_HPP

#include <list>
#include <memory>
#include <string>

#include <boost/shared_ptr.hpp>

#include <RCF/ClientTransport.hpp>
#include <RCF/ThreadLibrary.hpp>

namespace RCF {

    /// Special purpose client transport for sending messages in parallel on multiple sub-transports.
    class MulticastClientTransport : public I_ClientTransport
    {
    public:
        std::auto_ptr<I_ClientTransport> clone() const;
        EndpointPtr getEndpointPtr() const;
        int send(const std::vector<ByteBuffer> &data, unsigned int timeoutMs);
        int receive(ByteBuffer &byteBuffer, unsigned int timeoutMs);
        bool isConnected();
        void connect(unsigned int timeoutMs);
        void disconnect(unsigned int timeoutMs);
        void addTransport(const ClientTransportPtr &clientTransportPtr);
        void setTransportFilters(const std::vector<FilterPtr> &filters);
        void getTransportFilters(std::vector<FilterPtr> &filters);

    private:
        Mutex mMutex;
        typedef std::list< ClientTransportPtr > ClientTransportPtrList;
        ClientTransportPtrList mClientTransports;
    };

} // namespace RCF

#endif // ! INCLUDE_RCF_MULTICASTCLIENTTRANSPORT_HPP
